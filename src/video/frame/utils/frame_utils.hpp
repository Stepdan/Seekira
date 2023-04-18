#pragma once

#include <video/frame/interfaces/frame.hpp>

#include <filesystem>

// IO utils
namespace step::video::utils {

/*
    Allowed PixFmt is:
        GRAY
        BGR
        BGRA
*/
Frame open_file(const std::string& path, PixFmt fmt = PixFmt::BGR);

void save_to_file(Frame& frame, const std::filesystem::path& path);

}  // namespace step::video::utils

// Colorspace utils
namespace step::video::utils {

void convert_colorspace(Frame& frame, PixFmt dst_format);

}  // namespace step::video::utils