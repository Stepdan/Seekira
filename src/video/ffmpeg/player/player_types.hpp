#pragma once

namespace step::video::ff {

enum class MediaStatus
{
    Undefined,     // The status of the media cannot be determined.
    NoMedia,       // There is no current media. The player is in the StoppedState.
    LoadingMedia,  // The current media is being loaded. The player may be in any state.
    LoadedMedia,   // The current media has been loaded. The player is in the StoppedState.
    StalledMedia,  // Playback of the current media has stalled due to insufficient buffering or some other temporary interruption. The player is in the PlayingState or PausedState.
    BufferingMedia,  // The player is buffering data but has enough data buffered for playback to continue for the immediate future. The player is in the PlayingState or PausedState.
    BufferedMedia,  // The player has fully buffered the current media. The player is in the PlayingState or PausedState.
    EndOfMedia,     // Playback has reached the end of the current media. The player is in the StoppedState.
    InvalidMedia,   // The current media cannot be played. The player is in the StoppedState.
};

enum class PlayingState
{
    Undefined,
    Paused,
    Playing,
    Seeking,
    Stopped,
};

}  // namespace step::video::ff