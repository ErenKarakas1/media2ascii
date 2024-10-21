#include "ascii_lib.hpp"

#include <algorithm>
#include <format>
#include <ostream>
#include <sstream>

namespace AsciiArt {

std::string colorCode(const int colorIndex) {
    return std::format("{}{}m", COLOR_PREFIX, colorIndex);
}

ColoredPixel pixelToAscii(const unsigned char r, const unsigned char g, const unsigned char b) {
    // Convert to grayscale and then to ASCII
    const int pixel = (2126 * r + 7152 * g + 722 * b) / 10000;
    const char ascii = ASCII_CHARS[static_cast<size_t>(pixel / 255.0F * (ASCII_CHARS.length() - 1))];

    const int colorIndex = 16 + (36 * (r / 51)) + (6 * (g / 51)) + (b / 51);
    return {ascii, colorIndex};
}

ColoredPixel pixelToAscii(const unsigned char pixel) {
    return pixelToAscii(pixel, pixel, pixel);
}

std::vector<ColoredPixel> imageToAscii(const unsigned char *image, const int w, const int h, const int channels,
                                       const int outputW, const int outputH) {
    // Resize the image
    std::vector<unsigned char> resizedImg(static_cast<size_t>(outputW * outputH * channels));
    stbir_resize_uint8_linear(image, w, h, 0, resizedImg.data(), outputW, outputH, 0,
                              static_cast<stbir_pixel_layout>(channels));

    std::vector<ColoredPixel> asciiArt(static_cast<size_t>(outputW * outputH));

    if (channels >= 3) {
        for (int i = 0; i < outputW * outputH; ++i) {
            const int curr = i * channels;
            asciiArt[i] = pixelToAscii(resizedImg[curr], resizedImg[curr + 1], resizedImg[curr + 2]);
        }
    } else {
        std::ranges::transform(resizedImg, asciiArt.begin(), [](const unsigned char pixel) {
            return pixelToAscii(pixel);
        });
    }

    return asciiArt;
}

std::vector<ColoredPixel> frameToAscii(const AVFrame *frame, const int w, const int h, const int channels) {
    std::vector<ColoredPixel> asciiArt(static_cast<size_t>(w * h));

    for (int y = 0; y < h; ++y) {
        for (int x = 0; x < w; ++x) {
            const int pixelIndex = y * frame->linesize[0] + x * channels;
            asciiArt[y * w + x] = pixelToAscii(frame->data[0][pixelIndex], frame->data[0][pixelIndex + 1],
                                               frame->data[0][pixelIndex + 2]);
        }
    }

    return asciiArt;
}

void printAsciiFrame(const std::vector<ColoredPixel> &asciiArt, const int w, const int h) {
    std::ostringstream oss;

    for (int y = 0; y < h; ++y) {
        for (int x = 0; x < w; ++x) {
            const auto [ascii, colorIndex] = asciiArt[y * w + x];
            oss << colorCode(colorIndex) << ascii;
        }
        oss << ANSI_RESET << '\n';
    }

    std::cout << oss.str();
}

} // namespace AsciiArt
