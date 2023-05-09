#include "graph.hpp"

#include <core/exception/assert.hpp>

#include <core/log/log.hpp>

namespace step::graph {

void BaseGraph::add_node(const IdType& id, BaseGraphNode::Ptr node)
{
    STEP_ASSERT(!m_nodes.contains(id), "Graph already contains node with id {}", id);
    node->m_id = id;
    m_nodes[id] = std::move(node);
}

void BaseGraph::add_edge(const IdType& from, const IdType& to)
{
    STEP_ASSERT(from != to, "Graph edge has equal from/to: {}", from);
    STEP_ASSERT(m_nodes.contains(from), "Graph doesn't contains parent node: {}", from);
    STEP_ASSERT(m_nodes.contains(to), "Graph doesn't contains child node: {}", to);

    auto& node_from = m_nodes[from];
    STEP_ASSERT(std::ranges::none_of(node_from->m_children, [&to](const IdType& item) { return item == to; }),
                "Graph node {} already contains child {}", from, to);

    auto& node_to = m_nodes[to];
    STEP_ASSERT(std::ranges::none_of(node_to->m_parents, [&from](const IdType& item) { return item == from; }),
                "Graph node {} already contains parent {}", to, from);

    node_from->m_children.push_back(to);
    node_to->m_parents.push_back(from);
    m_adj_mat[from].insert(to);
}

std::vector<BaseGraph::IdType> BaseGraph::get_all_ids() const noexcept
{
    std::vector<BaseGraph::IdType> ids;
    std::transform(m_nodes.cbegin(), m_nodes.cend(), std::back_inserter(ids),
                   [this](const auto& item) { return item.first; });

    return ids;
}

BaseGraphNode::Ptr BaseGraph::get_node(const IdType& id) const
{
    STEP_ASSERT(m_nodes.contains(id), "Graph doesn't contains node {}", id);
    return m_nodes[id];
}

std::vector<BaseGraphNode::Ptr> BaseGraph::get_parents(const IdType& id) const
{
    STEP_ASSERT(m_nodes.contains(id), "Graph doesn't contains node {}", id);

    const auto& node = m_nodes[id];
    std::vector<BaseGraphNode::Ptr> parents;
    std::transform(node->m_parents.cbegin(), node->m_parents.cend(), std::back_inserter(parents),
                   [this](const IdType& parent_id) { return get_node(parent_id); });
    return parents;
}

std::vector<BaseGraphNode::Ptr> BaseGraph::get_children(const IdType& id) const
{
    STEP_ASSERT(m_nodes.contains(id), "Graph doesn't contains node {}", id);

    const auto& node = m_nodes[id];
    std::vector<BaseGraphNode::Ptr> children;
    std::transform(node->m_children.cbegin(), node->m_children.cend(), std::back_inserter(children),
                   [this](const IdType& child_id) { return get_node(child_id); });
    return children;
}

bool BaseGraph::is_valid() const
{
    // find nodes without edges
    for (const auto& [id, node] : m_nodes)
    {
        if (!node->has_parents() || !node->has_children())
        {
            STEP_LOG(L_ERROR, "Graph node {} doesn't have parent or child!", id);
            return false;
        }

        if (m_settings.one_parent && node->get_parents_ids().size() != 1)
        {
            STEP_LOG(L_ERROR, "Graph node {} has more than 1 parent!", id);
            return false;
        }
    }

    // find edges with non-existing node
    for (const auto& [from, to_set] : m_adj_mat)
    {
        if (!m_nodes.contains(from))
        {
            STEP_LOG(L_ERROR, "Graph has edge with non-existing parent node {}!", from);
            return false;
        }

        for (const auto& to : to_set)
        {
            if (!m_nodes.contains(to))
            {
                STEP_LOG(L_ERROR, "Graph has edge with non-existing child node {}!", to);
                return false;
            }
        }
    }

    return true;
}

}  // namespace step::graph