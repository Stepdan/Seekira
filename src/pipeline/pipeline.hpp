#pragma once

#include "pipeline_branch.hpp"
#include "pipeline_settings.hpp"

#include <graph/graph.hpp>

#include <log/log.hpp>

namespace step::pipeline {

template <typename TData>
class Pipeline : public graph::BaseGraph,
                 public utils::ThreadWorker<PipelineDataTypePtr<TData>>,
                 public IPipelineBranchEventObserver<TData>,
                 public ISerializable
{
protected:
    using ThreadWorkerDataType = PipelineDataTypePtr<TData>;
    using ThreadWorkerType = step::utils::ThreadWorker<ThreadWorkerDataType>;

public:
    Pipeline(const ObjectPtrJSON& config)
    {
        graph::GraphSettings graph_settings;
        graph_settings.one_parent = true;
        set_settings(graph_settings);

        deserialize(config);
    }

    virtual ~Pipeline() { stop(); }

    virtual void stop() override
    {
        STEP_LOG(L_INFO, "Stopping pipeline {}", m_settings.name);
        ThreadWorkerType::stop();
        for (auto& [id, branch] : m_branches)
            branch->stop();
        STEP_LOG(L_INFO, "Pipeline {} has been stopped", m_settings.name);
    }

    IdType get_root_id() const
    {
        STEP_ASSERT(m_root, "Pipeline {} can'r provide root id: empty root!", m_settings.name);
        return m_root->get_id();
    }

private:
    void process_data(ThreadWorkerDataType&& data) override
    {
        auto process_data = std::move(data);
        const auto& root_id = get_root_id();
        bool is_ready;

        std::scoped_lock lock(m_branches_data_guard);

        // Process input data
        auto& branch_data = m_branches_data[root_id];
        // Skip data if input branch still processing
        if (branch_data.status != BranchStatus::Finished)
            return;

        branch_data = {process_data, BranchStatus::Ready};

        // Check for exceptions
        for (auto& [id, branch] : m_branches)
        {
            if (!branch->has_exceptions())
                continue;

            STEP_LOG(L_ERROR, "Pipeline {}: Handle exceptions from branch {}", m_settings.name, id);
            for (auto branch_exception : branch->get_exceptions())
            {
                try
                {
                    std::rethrow_exception(branch_exception);
                }
                catch (const std::exception& e)
                {
                    STEP_LOG(L_ERROR, "Pipeline {}: Branch {} exception: {}", m_settings.name, id, e.what());
                    m_branches_data[id] = {nullptr, BranchStatus::Finished};
                    ThreadWorkerType::add_exception(std::current_exception());
                }
            }
            branch->reset_exceptions();
        }
        // TODO exception handling
        ThreadWorkerType::reset_exceptions();

        // For ParallelWait we ensure that all branches are stopped and input are ready
        bool ready_for_wait_iteration = true;
        if (m_settings.sync_policy == SyncPolicy::ParallelWait)
        {
            ready_for_wait_iteration = std::ranges::all_of(m_branches_data, [&root_id](const auto& item) {
                return item.first == root_id ? item.second.status == BranchStatus::Ready
                                             : item.second.status == BranchStatus::Finished;
            });
        }

        for (auto& [id, branch_data] : m_branches_data)
        {
            // Skip if branch is running now
            if (branch_data.status == BranchStatus::Running)
                continue;

            is_ready = branch_data.data_to_process && branch_data.status == BranchStatus::Ready;

            if (m_settings.sync_policy == SyncPolicy::ParallelWait)
            {
                // For ParallelWait we should ensure that we are processing input with all stopped other branches
                is_ready = is_ready && (root_id != id || root_id == id && ready_for_wait_iteration);
            }

            // Skip if smth is not ready
            if (!is_ready)
                continue;

            STEP_ASSERT(branch_data.data_to_process && branch_data.status == BranchStatus::Ready,
                        "Invalid pipeline process data for start!");

            STEP_LOG(L_INFO, "Pipeline {} start branch {}", m_settings.name, id);

            // Copy processing data and clear it from storage
            //    to skip next data before finishing
            m_branches[id]->add_data(std::move(branch_data.data_to_process));
            branch_data = {nullptr, BranchStatus::Running};
        }
    }

    // Data parsing
private:
    void deserialize(const ObjectPtrJSON& config)
    {
        STEP_LOG(L_INFO, "Start pipeline deserializing");
        m_root.reset();
        auto pipeline_json = json::get_object(config, CFG_FLD::PIPELINE);

        auto settings_json = json::get_object(pipeline_json, CFG_FLD::SETTINGS);
        m_settings = PipelineSettings(settings_json);
        const auto& pipeline_name = m_settings.name;
        STEP_LOG(L_INFO, "Pipeline name: {}", pipeline_name);

        auto nodes_collection = json::get_array(pipeline_json, CFG_FLD::NODES);
        json::for_each_in_array<ObjectPtrJSON>(nodes_collection, [this](const ObjectPtrJSON& node_cfg) {
            auto node = std::make_shared<PipelineGraphNode<TData>>(node_cfg);
            add_node(node->get_id(), node);
        });

        auto links_collection = json::get_array(pipeline_json, CFG_FLD::LINKS);
        json::for_each_in_array<ArrayPtrJSON>(links_collection, [this, &pipeline_name](const ArrayPtrJSON& link_cfg) {
            STEP_ASSERT(link_cfg->size() == 2, "Pipeline {}: Invalid link", pipeline_name);
            std::vector<std::string> ids;
            json::for_each_in_array<std::string>(link_cfg, [&ids](const std::string& id) { ids.push_back(id); });

            add_edge(ids.front(), ids.back());
        });

        m_root = std::dynamic_pointer_cast<PipelineGraphNode<TData>>(get_node("input_node"));

        init_branches();
    }

    void init_branches()
    {
        for (const auto& id : get_nodes_without_parents())
            m_branches[id] = std::make_shared<PipelineBranch<TData>>();

        for (const auto& parent_id : get_nodes_with_children())
        {
            for (const auto& child_id : get_node(parent_id)->get_children_ids())
            {
                m_branches[child_id] = std::make_shared<PipelineBranch<TData>>();
            }
        }

        for (auto& [id, branch] : m_branches)
        {
            branch->register_observer(this);
            m_branches_data[id] = {nullptr, BranchStatus::Finished};

            auto list_front_id = id;
            auto processed_id = id;

            auto node = get_node(processed_id);
            branch->add_node(std::dynamic_pointer_cast<PipelineGraphNode<TData>>(node));
            if (node->get_children_ids().empty())
                continue;

            processed_id = node->get_children_ids().front();

            while (!m_branches.contains(processed_id))
            {
                auto node = get_node(processed_id);
                branch->add_node(std::dynamic_pointer_cast<PipelineGraphNode<TData>>(node));
                if (node->get_children_ids().empty())
                    break;

                processed_id = node->get_children_ids().front();
            }
        }
    }

    // IPipelineListEventObserver
private:
    void on_branch_finished(std::pair<IdType, IdType> branch_ids, PipelineDataTypePtr<TData> data) override
    {
        STEP_LOG(L_INFO, "On branch finished [{} -> ... -> {}]", branch_ids.first, branch_ids.second);

        const auto& children_ids = get_node(branch_ids.second)->get_children_ids();
        std::scoped_lock lock(m_branches_data_guard);
        m_branches_data[branch_ids.first] = {nullptr, BranchStatus::Finished};
        for (const auto& child_id : children_ids)
        {
            auto& branch_data = m_branches_data[child_id];
            if (branch_data.status == BranchStatus::Finished)
                branch_data = {clone_pipeline_data_shared(data), BranchStatus::Ready};
        }
    }

private:
    PipelineSettings m_settings;
    PipelineGraphNode<TData>::Ptr m_root;

    std::atomic_bool m_is_running{false};
    std::atomic_bool m_need_stop{false};
    std::thread m_worker;

    enum class BranchStatus
    {
        /* clang-format off */
        Ready       = 1, // Is stopped and has data for processing
        Running     = 2, // Is running, data is empty
        //NeedStop    = 3, // Finished but stop requires, data is empty
        Finished    = 4, // Is stopped, no data for processing
        /* clang-format on */
    };

    template <typename TProcessData>
    struct BranchProcessData
    {
        TProcessData data_to_process;
        BranchStatus status{BranchStatus::Finished};
    };
    mutable std::mutex m_branches_data_guard;
    robin_hood::unordered_map<IdType, BranchProcessData<std::shared_ptr<PipelineData<TData>>>> m_branches_data;
    robin_hood::unordered_map<IdType, std::shared_ptr<PipelineBranch<TData>>> m_branches;
};

}  // namespace step::pipeline