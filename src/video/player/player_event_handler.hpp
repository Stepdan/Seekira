#pragma once

#include "player_types.hpp"

#include <core/base/interfaces/event_handler_list.hpp>
#include <core/threading/thread_pool_execute_policy.hpp>

#include <core/exception/assert.hpp>

namespace step::video::ff {

class IPlayerEventObserver
{
public:
    virtual ~IPlayerEventObserver() = default;

    virtual void on_media_status_changed(MediaStatus) { STEP_UNDEFINED("on_media_status_changed is undefined!"); }
    virtual void on_playing_state_changed(PlayingState) { STEP_UNDEFINED("on_playing_state_changed is undefined!"); }
};

class IPlayerEventSource
{
public:
    virtual ~IPlayerEventSource() = default;

    virtual void register_observer(IPlayerEventObserver* observer) = 0;
    virtual void unregister_observer(IPlayerEventObserver* observer) = 0;
};

}  // namespace step::video::ff