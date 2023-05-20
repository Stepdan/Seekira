#pragma once

#include <video/frame/interfaces/frame_interfaces.hpp>
#include <video/ffmpeg/decoding/stream_reader.hpp>

namespace step::video::ff {

class FFWrapper : public IFrameSource
{
public:
    FFWrapper();
    ~FFWrapper();

    void open_file(const std::string& filename);

    void run();
    void stop();

    bool is_running() const;

    // IFrameSource
public:
    void register_observer(IFrameSourceObserver* observer) override;
    void unregister_observer(IFrameSourceObserver* observer) override;

private:
    std::shared_ptr<ParserFF> m_parser;
    std::shared_ptr<IDemuxer> m_demuxer;
    std::shared_ptr<IStreamReader> m_stream_reader;
};

}  // namespace step::video::ff