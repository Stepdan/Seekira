#pragma once

namespace step::video::ff {

enum class ReaderState
{
    Undefined,
    Reading,
    TryToSeek,
    SuccessfulSeek,
    InvalidSeek,
    EndOfFile,
    Error,
    Destruction,
};

}  // namespace step::video::ff