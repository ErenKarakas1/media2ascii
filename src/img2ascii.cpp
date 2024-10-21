#include "ascii_lib.hpp"

#include <filesystem>

static constexpr int OUTPUT_WIDTH = 600;

int main(int argc, char **argv) {
    if (argc < 2) {
        std::cerr << "Usage: " << argv[0] << " <image file>" << '\n';
        return 1;
    }

    const std::filesystem::path imagePath(argv[1]);

    if (!std::filesystem::exists(imagePath)) {
        std::cerr << "File not found: " << imagePath << '\n';
        return 1;
    }

    int width, height, channels;
    unsigned char *img = stbi_load(imagePath.c_str(), &width, &height, &channels, 0);

    if (img == nullptr) {
        std::cout << "Error in loading the image" << '\n';
        return 1;
    }

    const double aspectRatio = static_cast<double>(height) / width;
    const int outputHeight = static_cast<int>(OUTPUT_WIDTH * aspectRatio * 0.45);

    const auto asciiArt = AsciiArt::imageToAscii(img, width, height, channels, OUTPUT_WIDTH, outputHeight);

    AsciiArt::printAsciiFrame(asciiArt, OUTPUT_WIDTH, outputHeight);

    stbi_image_free(img);

    return 0;
}
