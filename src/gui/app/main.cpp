#include <gui/utils/qimage_utils.hpp>

int main(int argc, char* argv[])
{
    step::video::Frame frame;
    auto qimage = step::gui::utils::frame_to_qimage(frame);

    return 0;
}