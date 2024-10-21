#ifndef ASCII_LIB_HPP
#define ASCII_LIB_HPP

#include "stb_image.h"
#include "stb_image_resize2.h"
#include "ffmpeg.hpp"

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

std::string colorCode(int colorIndex);

ColoredPixel pixelToAscii(unsigned char r, unsigned char g, unsigned char b);
ColoredPixel pixelToAscii(unsigned char pixel);

std::vector<ColoredPixel> imageToAscii(const unsigned char *image, int w, int h, int channels, int outputW, int outputH);
std::vector<ColoredPixel> frameToAscii(const AVFrame *frame, int w, int h, int channels);

void printAsciiFrame(const std::vector<ColoredPixel> &asciiArt, int w, int h);

}  // namespace AsciiArt

#endif // ASCII_LIB_HPP
