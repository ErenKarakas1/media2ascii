#ifndef ASCII_LIB_HPP
#define ASCII_LIB_HPP

#include "ffmpeg.hpp"
#include "stb_image.h"
#include "stb_image_resize2.h"

#include <iostream>
#include <string>
#include <string_view>
#include <vector>

namespace AsciiArt {

struct ColoredPixel {
    char ascii;
    int colorIndex;
};

constexpr std::string_view ASCII_CHARS = ".:;=ox+*?SXE$O8NZHMW#BQ@";
constexpr std::string_view COLOR_PREFIX = "\033[38;5;";
constexpr std::string_view ANSI_RESET = "\033[0m";

std::string color_code(int colorIndex);

ColoredPixel pixel_to_ascii(unsigned char r, unsigned char g, unsigned char b);
ColoredPixel pixel_to_ascii(unsigned char pixel);

std::vector<ColoredPixel> image_to_ascii(const unsigned char* image, int w, int h, int channels, int outputW,
                                         int outputH);
std::vector<ColoredPixel> frame_to_ascii(const AVFrame* frame, int w, int h, int channels);

void print_ascii_frame(const std::vector<ColoredPixel>& asciiArt, int w, int h);

} // namespace AsciiArt

#endif // ASCII_LIB_HPP
