#include "ascii_lib.hpp"

#include <filesystem>

static constexpr int OUTPUT_WIDTH = 600;

int main(int argc, char** argv) {
    if (argc < 2) {
        std::cerr << "Usage: " << argv[0] << " FILE" << '\n';
        return 1;
    }

    const std::filesystem::path image_path(argv[1]);

    if (!std::filesystem::exists(image_path)) {
        std::cerr << "File not found: " << image_path << '\n';
        return 1;
    }

    int width, height, channels;
    unsigned char* img = stbi_load(image_path.c_str(), &width, &height, &channels, 0);

    if (img == nullptr) {
        std::cout << "Error in loading the image" << '\n';
        return 1;
    }

    const double aspect_ratio = static_cast<double>(height) / width;
    const int output_height = static_cast<int>(OUTPUT_WIDTH * aspect_ratio * 0.45);

    const auto ascii_art = AsciiArt::image_to_ascii(img, width, height, channels, OUTPUT_WIDTH, output_height);

    print_ascii_frame(ascii_art, OUTPUT_WIDTH, output_height);

    stbi_image_free(img);

    return 0;
}
