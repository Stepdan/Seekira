#include "ff_wrapper.hpp"

namespace step::video::ff {

FFWrapper::FFWrapper() = default;
FFWrapper::~FFWrapper() = default;

void FFWrapper::open_file(const std::string& filename) { m_parser.load(filename); }

void FFWrapper::run() { m_parser.run_worker(); }

void FFWrapper::stop() { m_parser.stop_worker(); }

bool FFWrapper::is_running() const { return m_parser.is_running(); }

void FFWrapper::register_observer(IFrameSourceObserver* observer) { m_parser.register_observer(observer); }

void FFWrapper::unregister_observer(IFrameSourceObserver* observer) { m_parser.unregister_observer(observer); }

}  // namespace step::video::ff