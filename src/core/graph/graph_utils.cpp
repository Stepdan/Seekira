#include "graph.hpp"

namespace step::graph {

std::vector<BaseGraph::IdType> BaseGraph::get_nodes_without_parents() const
{
    std::vector<BaseGraph::IdType> no_parents_nodes;
    for (const auto& [id, node] : m_nodes)
    {
        if (!node->has_parents())
            no_parents_nodes.push_back(id);
    }
    return no_parents_nodes;
}

std::vector<BaseGraph::IdType> BaseGraph::get_nodes_with_children() const
{
    std::vector<BaseGraph::IdType> nodes;
    for (const auto& [id, node] : m_nodes)
    {
        if (node->get_children_ids().size() > 1)
            nodes.push_back(id);
    }
    return nodes;
}

}  // namespace step::graph