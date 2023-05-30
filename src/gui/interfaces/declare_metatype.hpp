#pragma once

#include "enums.hpp"

#include <video/frame/interfaces/frame.hpp>

#include <QMetaType>

//Q_DECLARE_METATYPE(step::gui::Enums);
Q_DECLARE_METATYPE(step::gui::Enums::PlayerDirection);
Q_DECLARE_METATYPE(std::shared_ptr<step::video::Frame>);