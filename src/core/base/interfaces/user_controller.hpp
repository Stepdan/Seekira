#pragma once

#include <core/exception/assert.hpp>

#include <vector>

namespace step {

template <typename IUserT>
class IUserController
{
public:
    using Ptr = IUserT*;

public:
    IUserController() {}
    virtual ~IUserController() = default;

    virtual void add_user(Ptr user)
    {
        STEP_ASSERT(user, "Can't add user: nullptr!");
        m_users.push_back(user);
        add_controller_data_to_user(user);
    }

protected:
    virtual void add_controller_data_to_user(Ptr user) = 0;

protected:
    std::vector<Ptr> m_users;
};

}  // namespace step