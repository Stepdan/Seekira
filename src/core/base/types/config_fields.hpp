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
    static const std::string PATH;
    static const std::string MODEL_PATH;

    static const std::string WIDTH;
    static const std::string HEIGHT;

    static const std::string SETTINGS;
    static const std::string CONFIG;
    static const std::string MODE;
    static const std::string TYPE;

    static const std::string SERIAL_ID;
    static const std::string CREATE_ID;
    static const std::string CONNECTION_ID;

    static const std::string CACHE_SIZE;

    static const std::string DEVICE;

    static const std::string INTERPOLATION;

    /* Coordinates */
    static const std::string X;
    static const std::string Y;
    static const std::string Z;
    static const std::string W;
    static const std::string X0;
    static const std::string Y0;
    static const std::string X1;
    static const std::string Y1;
    static const std::string POINT;
    static const std::string POINT0;
    static const std::string POINT1;
    static const std::string RECT;

    /* Color */
    static const std::string COLOR_RGB;
    static const std::string RED;
    static const std::string GREEN;
    static const std::string BLUE;

    static const std::string FACE_COLOR;
    static const std::string VALID_FACE_COLOR;
    static const std::string INVALID_FACE_COLOR;

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
    static const std::string CYCLIC;

    /* Abstract task */
    static const std::string TASK_SETTINGS_ID;

    /* Frame */
    static const std::string PIXEL_FORMAT;
    static const std::string FRAME_SIZE;

    /* Frame pipeline */
    static const std::string PIPELINE;
    static const std::string NODE;
    static const std::string NODES;
    static const std::string LINK;
    static const std::string LINKS;
    static const std::string SYNC_MODE;

    /* Video processing */
    static const std::string VIDEO_PROCESSOR;

    /* Face engine */
    static const std::string FACE;
    static const std::string FACES;
    static const std::string FACE_DETECTION_RESULT;
    static const std::string FACE_ENGINE_CONNECTION_ID;
    static const std::string FACE_ENGINE_INIT;
    static const std::string FACE_ENGINE_INIT_SAVE_FRAMES;
    static const std::string FACE_ENGINE_CONTROLLER;

    /* Resizer */
    static const std::string RESIZER_SIZE_MODE;

    /* ReaderFF */
    static const std::string READER_FF_SETTINGS;

    static const std::string DRAWER_SETTINGS;
};

}  // namespace step