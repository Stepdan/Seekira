#pragma once

#include "pipeline_branch.hpp"
#include "pipeline_settings.hpp"

#include <graph/graph.hpp>

#include <log/log.hpp>

namespace step::pipeline {

template <typename TData>
class Pipeline : public graph::BaseGraph, public IPipelineBranchEventObserver<TData>, public ISerializable
{
public:
    Pipeline(const ObjectPtrJSON& config)
    {
        graph::GraphSettings graph_settings;
        graph_settings.one_parent = true;
        set_settings(graph_settings);

        deserialize(config);
    }

    virtual ~Pipeline() { reset(); }

    void reset()
    {
        m_need_stop.store(true);
        if (m_worker.joinable())
            m_worker.join();

        m_is_running.store(false);

        m_root.reset();
    }

    bool is_running() const { return m_is_running; }

    IdType get_root_id() const
    {
        STEP_ASSERT(m_root, "Pipeline {} can'r provide root id: empty root!", m_settings.name);
        return m_root->get_id();
    }

protected:
    bool add_process_data(const std::shared_ptr<PipelineData<TData>>& data)
    {
        // TODO Single method instead of lazy start?
        if (!m_is_running)
        {
            m_need_stop.store(false);
            STEP_LOG(L_INFO, "Start pipeline {}", m_settings.name);
            m_worker = std::thread(&Pipeline::run, this);
            m_is_running.store(true);
        }

        bool result;
        {
            std::scoped_lock lock(m_branches_data_guard);
            result = add_process_data(get_root_id(), data);
        }
        return result;
    }

private:
    bool add_process_data(const IdType& id, const std::shared_ptr<PipelineData<TData>>& data)
    {
        // NO LOCK THERE
        auto& branch_data = m_branches_data[id];
        if (branch_data.status != BranchStatus::Finished)
        {
            return false;
        }

        branch_data = {clone_pipeline_data_shared(data), BranchStatus::Ready};
        return true;
    }

    void run()
    {
        switch (m_settings.sync_policy)
        {
            case SyncPolicy::ParallelWait:
            case SyncPolicy::ParallelNoWait:
                run_parallel();
                break;
            case SyncPolicy::Sync:
                STEP_UNDEFINED("Sync policy for pipeline is undefined");
                break;
            default:
                STEP_UNDEFINED("Undefined sync policy for pipeline");
                break;
        }
    }

    void run_parallel()
    {
        const auto& root_id = get_root_id();
        bool is_ready;

        while (!m_need_stop)
        {
            std::scoped_lock lock(m_branches_data_guard);

            // Check for exceptions
            {
                for (auto& [id, branch] : m_branches)
                {
                    try
                    {
                        if (auto eptr = branch->get_exception(); eptr)
                        {
                            STEP_LOG(L_WARN, "Pipeline {}: Handle exception from branch {}", m_settings.name, id);
                            std::rethrow_exception(eptr);
                        }
                    }
                    catch (const std::exception& e)
                    {
                        STEP_LOG(L_ERROR, "Pipeline {}: Branch {} exception: {}", m_settings.name, id, e.what());
                        m_branches_data[id] = {nullptr, BranchStatus::NeedStop};
                        branch->reset_exception();

                        // TODO Exception event handling
                    }
                }
            }

            bool ready_for_wait_iteration = true;
            // For ParallelWait we ensure that all branches are stopped and input are ready
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

                // Stop branch if it has been marked as 'NeedStop'
                if (branch_data.status == BranchStatus::NeedStop)
                {
                    m_branches[id]->stop();
                    branch_data.status = BranchStatus::Finished;
                    STEP_LOG(L_INFO, "Pipeline {} stop branch {}", m_settings.name, id);
                    continue;
                }

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
                auto process_data = clone_pipeline_data_shared(branch_data.data_to_process);
                branch_data = {nullptr, BranchStatus::Running};

                m_branches[id]->run(process_data);
            }
        }

        // In case of stopping - stop and wait for all branches
        for (const auto& [id, branch] : m_branches)
        {
            STEP_LOG(L_INFO, "Pipeline {}: Stopping branch {} due pipeline stop", m_settings.name, id);
            branch->stop();
            m_branches_data[id] = {nullptr, BranchStatus::Finished};
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
            branch->add(std::dynamic_pointer_cast<PipelineGraphNode<TData>>(node));
            if (node->get_children_ids().empty())
                continue;

            processed_id = node->get_children_ids().front();

            while (!m_branches.contains(processed_id))
            {
                auto node = get_node(processed_id);
                branch->add(std::dynamic_pointer_cast<PipelineGraphNode<TData>>(node));
                if (node->get_children_ids().empty())
                    break;

                processed_id = node->get_children_ids().front();
            }
        }
    }

    // IPipelineListEventObserver
private:
    void on_branch_finished(std::pair<IdType, IdType> branch_ids, std::shared_ptr<PipelineData<TData>> data) override
    {
        STEP_LOG(L_INFO, "On branch finished [{} -> ... -> {}]", branch_ids.first, branch_ids.second);

        const auto& children_ids = get_node(branch_ids.second)->get_children_ids();
        std::scoped_lock lock(m_branches_data_guard);
        m_branches_data[branch_ids.first] = {nullptr, BranchStatus::NeedStop};
        for (const auto& child_id : children_ids)
            add_process_data(child_id, data);
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
        NeedStop    = 3, // Finished but stop requires, data is empty
        Finished    = 4, // Is stopped, no data for processing
        /* clang-format on */
    };

    template <typename TProcessData>
    struct BranchProcessData
    {
        TProcessData data_to_process;
        BranchStatus status{BranchStatus::Finished};
    };
    std::mutex m_branches_data_guard;
    robin_hood::unordered_map<IdType, BranchProcessData<std::shared_ptr<PipelineData<TData>>>> m_branches_data;
    robin_hood::unordered_map<IdType, std::shared_ptr<PipelineBranch<TData>>> m_branches;
};

}  // namespace step::pipeline