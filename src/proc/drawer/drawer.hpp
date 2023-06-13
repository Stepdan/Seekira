#pragma once

#include <video/frame/interfaces/frame.hpp>

#include "settings_drawer.hpp"

#include <opencv2/core/types.hpp>

namespace step::proc {

class Drawer
{
public:
    Drawer(const SettingsDrawer& settings = SettingsDrawer()) : m_settings(settings) {}

    void set_settings(const SettingsDrawer& settings) { m_settings = settings; }

    template <typename T>
    void draw(video::Frame& frame, const T& obj);

private:
    cv::Scalar get_cv_color(const ColorRGB& color) { return cv::Scalar(color.red, color.green, color.blue); }

public:
    SettingsDrawer m_settings;
};

}  // namespace step::proc