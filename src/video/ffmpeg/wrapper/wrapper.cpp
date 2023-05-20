#include "wrapper.hpp"

namespace step::video::ff {

FFWrapper::FFWrapper() = default;
FFWrapper::~FFWrapper() = default;

void FFWrapper::open_file(const std::string& filename)
{
    m_parser = std::make_shared<ParserFF>();
    m_parser->open_file(filename);

    m_demuxer = std::make_shared<DemuxerQueue>(m_parser);
    m_stream_reader = std::make_shared<StreamReader>(m_demuxer);
}

void FFWrapper::run() {}

void FFWrapper::stop() {}

bool FFWrapper::is_running() const { return false; }

void FFWrapper::register_observer(IFrameSourceObserver* observer) {}

void FFWrapper::unregister_observer(IFrameSourceObserver* observer) {}

}  // namespace step::video::ff