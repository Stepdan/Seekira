#pragma once

#include "node.hpp"
#include "graph_settings.hpp"

#include <robin_hood.h>

#include <memory>
#include <vector>

namespace step::graph {

class BaseGraph
{
public:
    using IdType = BaseGraphNode::IdType;

public:
    virtual ~BaseGraph() = default;

    void set_settings(const GraphSettings& settings) { m_settings = settings; }

    void add_node(const IdType& id, BaseGraphNode::Ptr node);
    void add_edge(const IdType& from, const IdType& to);

    /*
        TODO
    void remove_node(const IdType& id);
    void remove_edge(const IdType& from, const IdType& to);
    */

    std::vector<IdType> get_all_ids() const noexcept;
    BaseGraphNode::Ptr get_node(const IdType& id) const;
    std::vector<BaseGraphNode::Ptr> get_parents(const IdType& id) const;
    std::vector<BaseGraphNode::Ptr> get_children(const IdType& id) const;

    bool is_valid() const;

    // utils
public:
    std::vector<IdType> get_nodes_without_parents() const;
    std::vector<IdType> get_nodes_with_children() const;
    robin_hood::unordered_map<IdType, std::vector<IdType>> get_lists() const;

private:
    GraphSettings m_settings;
    mutable robin_hood::unordered_map<IdType, BaseGraphNode::Ptr> m_nodes;
    robin_hood::unordered_map<IdType, robin_hood::unordered_set<IdType>> m_adj_mat;
};

}  // namespace step::graph