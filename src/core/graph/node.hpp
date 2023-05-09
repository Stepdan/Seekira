#pragma once

#include <memory>
#include <string>
#include <vector>

namespace step::graph {

class BaseGraph;

class BaseGraphNode
{
public:
    using IdType = std::string;
    using Ptr = std::shared_ptr<BaseGraphNode>;

public:
    virtual ~BaseGraphNode() = default;

    const IdType& get_id() const noexcept { return m_id; }
    const std::vector<IdType>& get_parents_ids() const noexcept { return m_parents; }
    const std::vector<IdType>& get_children_ids() const noexcept { return m_children; }

    bool has_parents() const noexcept { return !m_parents.empty(); }
    bool has_children() const noexcept { return !m_children.empty(); }

protected:
    IdType m_id;

private:
    friend class BaseGraph;
    std::vector<IdType> m_parents;
    std::vector<IdType> m_children;
};

}  // namespace step::graph