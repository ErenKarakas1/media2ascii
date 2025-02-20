#include "ascii_lib.hpp"
#include "cmdline.hpp"

#include <chrono>
#include <filesystem>
#include <thread>

constexpr int OUTPUT_WIDTH = 600;
constexpr double MIN_FPS = 1.0; // Guaranteed minimum fps

int main(int argc, char** argv) {
    double max_fps = 144.0; // Screen refresh rate
    std::filesystem::path video_path;

    utils::cmd::add_option(
        {.name = "max-fps", .description = "Set maximum frames per second", .value = "fps", .default_value = 144});
    utils::cmd::add_option({.alt = 'h', .name = "help", .description = "Show this help message"});
    utils::cmd::add_positional("FILE");

    if (argc < 2) {
        std::cerr << "Not enough arguments" << '\n';
        utils::cmd::print_help(argv[0]);
        return 1;
    }

    utils::cmd::shift(argc, argv); // Skip program name

    while (argc > 0) {
        const std::string_view arg = utils::cmd::shift(argc, argv);

        if (arg == "-h" || arg == "--help") {
            utils::cmd::print_help(argv[0]);
            return 0;
        }
        if (arg == "--max-fps") {
            const auto fps_str = utils::cmd::shift(argc, argv);
            double fps = 0;
            if (std::from_chars(fps_str.data(), fps_str.data() + fps_str.size(), fps).ec != std::errc()) {
                std::cerr << "Invalid fps value: " << fps_str << '\n';
                return 1;
            }
            max_fps = std::max(fps, MIN_FPS);
        } else {
            video_path = static_cast<std::filesystem::path>(arg);
        }
    }

    if (video_path.empty()) {
        std::cerr << "No video file provided" << '\n';
        utils::cmd::print_help(argv[0]);
        return 1;
    }

    if (!std::filesystem::exists(video_path)) {
        std::cerr << "File not found: " << video_path << '\n';
        return 1;
    }

    AVFormatContext* format_context = avformat_alloc_context();

    if (avformat_open_input(&format_context, video_path.c_str(), nullptr, nullptr) != 0) {
        std::cerr << "Error opening the video file" << '\n';
        return 1;
    }

    if (avformat_find_stream_info(format_context, nullptr) < 0) {
        std::cerr << "Error finding the stream info" << '\n';
        return 1;
    }

    int video_stream_index = -1;

    for (unsigned int i = 0; i < format_context->nb_streams; ++i) {
        if (format_context->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO) {
            video_stream_index = i;
            break;
        }
    }

    if (video_stream_index == -1) {
        std::cerr << "Error finding the video stream" << '\n';
        return 1;
    }

    AVStream* video_stream = format_context->streams[video_stream_index];
    const AVRational frame_rate = av_guess_frame_rate(format_context, video_stream, nullptr);
    const double fps = av_q2d(frame_rate);

    const double target_fps = std::clamp(fps, MIN_FPS, max_fps);

    const AVCodecParameters* codec_parameters = video_stream->codecpar;
    const AVCodec* codec = avcodec_find_decoder(codec_parameters->codec_id);

    if (codec_parameters == nullptr || codec == nullptr) {
        std::cerr << "Error finding the decoder information" << '\n';
        return 1;
    }

    AVCodecContext* codec_context = avcodec_alloc_context3(codec);

    if (avcodec_parameters_to_context(codec_context, codec_parameters) < 0) {
        std::cerr << "Error converting codec parameters to codec context" << '\n';
        return 1;
    }

    if (avcodec_open2(codec_context, codec, nullptr) < 0) {
        std::cerr << "Error opening the codec" << '\n';
        return 1;
    }

    const float aspect_ratio = static_cast<float>(codec_context->height) / static_cast<float>(codec_context->width);
    const int output_height = static_cast<int>(OUTPUT_WIDTH * aspect_ratio * 0.45);

    SwsContext* sws_context =
        sws_getContext(codec_context->width, codec_context->height, codec_context->pix_fmt, OUTPUT_WIDTH, output_height,
                       AV_PIX_FMT_RGB24, SWS_BILINEAR, nullptr, nullptr, nullptr);

    if (sws_context == nullptr) {
        std::cerr << "Error creating the sws context" << '\n';
        return 1;
    }

    AVFrame* frame = av_frame_alloc();
    AVFrame* rgb_frame = av_frame_alloc();
    AVPacket* packet = av_packet_alloc();

    if (frame == nullptr || rgb_frame == nullptr || packet == nullptr) {
        std::cerr << "Error allocating the frames and packet" << '\n';
        return 1;
    }

    const int rgb_frame_size = av_image_get_buffer_size(AV_PIX_FMT_RGB24, OUTPUT_WIDTH, output_height, 1);
    const auto* rgb_framebuffer = static_cast<unsigned char*>(av_malloc(rgb_frame_size * sizeof(unsigned char)));

    av_image_fill_arrays(rgb_frame->data, rgb_frame->linesize, rgb_framebuffer, AV_PIX_FMT_RGB24, OUTPUT_WIDTH,
                         output_height, 1);

    std::vector<AsciiArt::ColoredPixel> asciiArt(static_cast<size_t>(OUTPUT_WIDTH * output_height));

    const auto frame_delay = std::chrono::milliseconds(static_cast<int>(1000 / target_fps));
    auto last_frame_time = std::chrono::steady_clock::now();

    while (av_read_frame(format_context, packet) >= 0) {
        if (packet->stream_index == video_stream_index) {
            if (avcodec_send_packet(codec_context, packet) < 0) {
                std::cerr << "Error sending a packet to the decoder" << '\n';
                break;
            }

            while (avcodec_receive_frame(codec_context, frame) >= 0) {
                sws_scale(sws_context, frame->data, frame->linesize, 0, codec_context->height, rgb_frame->data,
                          rgb_frame->linesize);

                asciiArt = AsciiArt::frame_to_ascii(rgb_frame, OUTPUT_WIDTH, output_height, 3);

                std::cout << "\033[H"; // Move cursor to top-left
                print_ascii_frame(asciiArt, OUTPUT_WIDTH, output_height);
                std::cout.flush();

                const auto current_time = std::chrono::steady_clock::now();
                const auto elapsed_time =
                    std::chrono::duration_cast<std::chrono::milliseconds>(current_time - last_frame_time);

                if (elapsed_time < frame_delay) {
                    std::this_thread::sleep_for(frame_delay - elapsed_time);
                }

                last_frame_time = std::chrono::steady_clock::now();
            }
        }
        av_packet_unref(packet);
    }

    sws_freeContext(sws_context);
    av_frame_free(&rgb_frame);
    av_frame_free(&frame);
    av_packet_free(&packet);
    avcodec_free_context(&codec_context);
    avformat_close_input(&format_context);

    return 0;
}
