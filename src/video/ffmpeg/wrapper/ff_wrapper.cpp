#include "ff_wrapper.hpp"

namespace step::video::ff {

FFWrapper::FFWrapper() = default;
FFWrapper::~FFWrapper() = default;

void FFWrapper::open_file(const std::string& filename) { m_demuxer.load(filename); }

void FFWrapper::run() { m_demuxer.run_worker(); }

void FFWrapper::stop() { m_demuxer.stop_worker(); }

bool FFWrapper::is_running() const { return m_demuxer.is_running(); }

void FFWrapper::register_observer(IFrameSourceObserver* observer) { m_demuxer.register_observer(observer); }

void FFWrapper::unregister_observer(IFrameSourceObserver* observer) { m_demuxer.unregister_observer(observer); }

}  // namespace step::video::ff