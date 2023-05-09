#pragma once

#include "pipeline_branch.hpp"
#include "pipeline_settings.hpp"

#include <core/threading/thread_pool.hpp>

#include <core/graph/graph.hpp>

#include <core/log/log.hpp>

namespace step::pipeline {

template <typename TData>
class Pipeline : public graph::BaseGraph,
                 public threading::ThreadPool<IdType, PipelineDataTypePtr<TData>, PipelineDataTypePtr<TData>>,
                 public ISerializable
{
protected:
    using ThreadPoolDataType = PipelineDataTypePtr<TData>;
    using ThreadPoolResultDataType = ThreadPoolDataType;
    using ThreadPoolType = step::threading::ThreadPool<IdType, PipelineDataTypePtr<TData>, PipelineDataTypePtr<TData>>;

public:
    Pipeline(const ObjectPtrJSON& config)
    {
        graph::GraphSettings graph_settings;
        graph_settings.one_parent = true;
        set_settings(graph_settings);

        deserialize(config);
    }

    virtual ~Pipeline()
    {
        STEP_LOG(L_TRACE, "Pipeline {} destruction", m_settings.name);
        ThreadPoolType::stop();
        // Stop (thread_pool_stop_impl) will be called from ThreadPool::stop during ThreadPool destruction
    }

    IdType get_root_id() const
    {
        STEP_ASSERT(m_root, "Pipeline {} can'r provide root id: empty root!", m_settings.name);
        return m_root->get_id();
    }

    bool add_process_data(PipelineDataTypePtr<TData>&& data)
    {
        if (!ThreadPoolType::is_running())
            ThreadPoolType::run();

        return add_process_data(get_root_id(), std::move(data));
    }

private:
    bool add_process_data(const IdType& id, PipelineDataTypePtr<TData>&& data)
    {
        std::scoped_lock lock(m_branches_data_guard);
        auto& branch_data = m_branches_data[id];
        if (branch_data.status != BranchStatus::Finished)
            return false;

        branch_data = {std::move(data), BranchStatus::Ready};
        return true;
    }

    void thread_pool_stop_impl() override
    {
        // NO LOCK THERE
        // Under thread_pool mutex m_stop_guard locking.
        // All threads already joined and stopped
        STEP_LOG(L_INFO, "Stopping pipeline {}", m_settings.name);

        for (auto& [id, branch_data] : m_branches_data)
            branch_data = {nullptr, BranchStatus::Finished};

        STEP_LOG(L_INFO, "Pipeline {} has been stopped", m_settings.name);
    }

    void thread_pool_iteration() override
    {
        const auto& root_id = get_root_id();
        bool is_ready;

        std::scoped_lock lock(m_branches_data_guard);

        // Check for exceptions
        for (auto& [id, branch] : ThreadPoolType::m_threads)
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
                }
            }
            // TODO exception handling
            branch->reset_exceptions();
        }

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

            ThreadPoolType::m_threads[id]->add_data(std::move(branch_data.data_to_process));
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
        std::set<IdType> branch_ids;
        for (const auto& id : get_nodes_without_parents())
            branch_ids.insert(id);

        for (const auto& parent_id : get_nodes_with_children())
            for (const auto& child_id : get_node(parent_id)->get_children_ids())
                branch_ids.insert(child_id);

        for (const auto& id : branch_ids)
        {
            auto list_front_id = id;
            auto processed_id = id;
            auto node = get_node(processed_id);

            ThreadPoolType::add_thread_worker(
                std::make_shared<PipelineBranch<TData>>(std::dynamic_pointer_cast<PipelineGraphNode<TData>>(node)));

            auto& branch = ThreadPoolType::m_threads[processed_id];
            branch->register_observer(this);
            m_branches_data[id] = {nullptr, BranchStatus::Finished};

            if (node->get_children_ids().empty())
                continue;

            processed_id = node->get_children_ids().front();

            while (!branch_ids.contains(processed_id))
            {
                auto node = get_node(processed_id);
                std::dynamic_pointer_cast<PipelineBranch<TData>>(branch)->add_node(
                    std::dynamic_pointer_cast<PipelineGraphNode<TData>>(node));
                if (node->get_children_ids().empty())
                    break;

                processed_id = node->get_children_ids().front();
            }
        }
    }

    // IThreadWorkerEventObserver
private:
    void on_finished(const IdType& id, ThreadPoolResultDataType data) override
    {
        STEP_LOG(L_INFO, "on_finished branch {}", id);
        if (ThreadPoolType::need_stop())
        {
            STEP_LOG(L_INFO, "Skip on_finished branch {} due stopping", id);
            return;
        }

        const auto branch_last_id =
            std::dynamic_pointer_cast<PipelineBranch<TData>>(ThreadPoolType::m_threads[id])->get_last_id();

        const auto& children_ids = get_node(branch_last_id)->get_children_ids();

        std::scoped_lock lock(m_branches_data_guard);
        m_branches_data[id] = {nullptr, BranchStatus::Finished};
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

    /*
        For thread pool iterations
    */
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
        TProcessData data_to_process{nullptr};
        BranchStatus status{BranchStatus::Finished};
    };
    mutable std::mutex m_branches_data_guard;
    robin_hood::unordered_map<IdType, BranchProcessData<std::shared_ptr<PipelineData<TData>>>> m_branches_data;
};

}  // namespace step::pipeline