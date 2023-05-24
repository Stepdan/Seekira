#pragma once

namespace step::video::ff {

enum class ReadingMode
{
    Undefined,
    Continuously,
    ByRequest,
};

enum class ReaderState
{
    Undefined,
    Ready,
    ReadingContiniously,
    ReadingByRequest,
    TryToSeek,
    SuccessfulSeek,
    InvalidSeek,
    EndOfFile,
    Error,
    Destruction,
};

}  // namespace step::video::ff