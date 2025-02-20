#include "ascii_lib.hpp"

#include <algorithm>
#include <format>
#include <ostream>
#include <sstream>

namespace AsciiArt {

std::string color_code(const int colorIndex) {
    return std::format("{}{}m", COLOR_PREFIX, colorIndex);
}

ColoredPixel pixel_to_ascii(const unsigned char r, const unsigned char g, const unsigned char b) {
    // Convert to grayscale and then to ASCII
    const float rf = static_cast<float>(r) / 255.0F;
    const float gf = static_cast<float>(g) / 255.0F;
    const float bf = static_cast<float>(b) / 255.0F;

    const float pixel = 0.2126F * rf + 0.7152F * gf + 0.0722F * bf;
    const char ascii = ASCII_CHARS[static_cast<size_t>(pixel) * (ASCII_CHARS.length() - 1)];

    const int colorIndex = 16 + (36 * (r / 51)) + (6 * (g / 51)) + (b / 51);
    return {.ascii = ascii, .colorIndex = colorIndex};
}

ColoredPixel pixel_to_ascii(const unsigned char pixel) {
    return pixel_to_ascii(pixel, pixel, pixel);
}

std::vector<ColoredPixel> image_to_ascii(const unsigned char* image, const int w, const int h, const int channels,
                                         const int outputW, const int outputH) {
    // Resize the image
    std::vector<unsigned char> resizedImg(static_cast<size_t>(outputW * outputH * channels));
    stbir_resize_uint8_linear(image, w, h, 0, resizedImg.data(), outputW, outputH, 0,
                              static_cast<stbir_pixel_layout>(channels));

    std::vector<ColoredPixel> asciiArt(static_cast<size_t>(outputW * outputH));

    if (channels >= 3) {
        for (int i = 0; i < outputW * outputH; ++i) {
            const int curr = i * channels;
            asciiArt[i] = pixel_to_ascii(resizedImg[curr], resizedImg[curr + 1], resizedImg[curr + 2]);
        }
    } else {
        std::ranges::transform(resizedImg, asciiArt.begin(), [](const unsigned char pixel) {
            return pixel_to_ascii(pixel);
        });
    }

    return asciiArt;
}

std::vector<ColoredPixel> frame_to_ascii(const AVFrame* frame, const int w, const int h, const int channels) {
    std::vector<ColoredPixel> asciiArt(static_cast<size_t>(w * h));

    for (int y = 0; y < h; ++y) {
        for (int x = 0; x < w; ++x) {
            const int pixelIndex = y * frame->linesize[0] + x * channels;
            asciiArt[y * w + x] = pixel_to_ascii(frame->data[0][pixelIndex], frame->data[0][pixelIndex + 1],
                                                 frame->data[0][pixelIndex + 2]);
        }
    }

    return asciiArt;
}

void print_ascii_frame(const std::vector<ColoredPixel>& asciiArt, const int w, const int h) {
    std::ostringstream oss;
    oss.str().reserve(w * h * 13);

    thread_local static auto buffer = std::string(w * 13, ' ');

    for (int y = 0; y < h; ++y) {
        buffer.clear();
        for (int x = 0; x < w; ++x) {
            const auto [ascii, colorIndex] = asciiArt[y * w + x];
            // oss << color_code(colorIndex) << ascii;
            buffer += color_code(colorIndex);
            buffer.push_back(ascii);
        }
        oss << buffer << ANSI_RESET << '\n';
    }

    std::cout << oss.str();
}

} // namespace AsciiArt
