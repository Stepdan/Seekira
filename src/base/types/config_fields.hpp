#pragma once

#include <string>

namespace step {

// Global config fields
class CFG_FLD
{
    /* TODO string_view*/
public:
    /* Common */
    static const std::string ID;
    static const std::string NAME;

    static const std::string WIDTH;
    static const std::string HEIGHT;

    static const std::string SETTINGS;
    static const std::string MODE;

    static const std::string SERIAL_ID;

    /* Coordinates */
    static const std::string X;
    static const std::string Y;
    static const std::string Z;
    static const std::string W;

    /* Pose */
    static const std::string P;
    static const std::string Q;
    static const std::string POSE;
    static const std::string ANGLE;
    static const std::string ROTATION;
    static const std::string TRANSLATION;

    /* Camera */
    static const std::string FX;
    static const std::string FY;
    static const std::string CX;
    static const std::string CY;

    static const std::string INTRINSICS;
    static const std::string EXTRINSICS;
    static const std::string DISTORTION;
    static const std::string RESOLUTION;

    static const std::string EXPOSURE;
    static const std::string FRAME_RATE;
    static const std::string RECONNECT_TIMEOUT;

    /* Abstract task */
    static const std::string TASK_SETTINGS_ID;

    /* Frame */
    static const std::string PIXEL_FORMAT;

    /* Frame pipeline */
    static const std::string PIPELINE;
    static const std::string NODE;
    static const std::string NODES;
    static const std::string LINK;
    static const std::string LINKS;
    static const std::string SYNC_MODE;
};

}  // namespace step