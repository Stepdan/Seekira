#pragma once

#include "reader_types.hpp"

#include <core/base/interfaces/event_handler_list.hpp>

#include <core/exception/assert.hpp>

namespace step::video::ff {

class IReaderEventObserver
{
public:
    virtual ~IReaderEventObserver() = default;

    virtual void on_reader_state_changed(ReaderState) { STEP_UNDEFINED("on_reader_state_changed is undefined!"); }
};

class IReaderEventSource
{
public:
    virtual ~IReaderEventSource() = default;

    virtual void register_observer(IReaderEventObserver* observer) = 0;
    virtual void unregister_observer(IReaderEventObserver* observer) = 0;
};

}  // namespace step::video::ff