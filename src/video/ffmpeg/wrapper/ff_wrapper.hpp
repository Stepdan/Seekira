#pragma once

#include <video/ffmpeg/decoding/ff_parser.hpp>

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
    ParserFF m_parser;
};

}  // namespace step::video::ff