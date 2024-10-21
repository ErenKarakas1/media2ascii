#include "ascii_lib.hpp"

#include <filesystem>
#include <thread>
#include <chrono>

static constexpr int OUTPUT_WIDTH = 600;
static constexpr double MAX_FPS = 144.0;    // Screen refresh rate
static constexpr double MIN_FPS = 1.0;      // Guaranteed minimum fps

int main(int argc, char **argv) {
    if (argc < 2) {
        std::cerr << "Usage: " << argv[0] << " <video file>" << '\n';
        return 1;
    }

    const std::filesystem::path videoPath(argv[1]);

    if (!std::filesystem::exists(videoPath)) {
        std::cerr << "File not found: " << videoPath << '\n';
        return 1;
    }

    AVFormatContext *formatContext = avformat_alloc_context();

    if (avformat_open_input(&formatContext, videoPath.c_str(), nullptr, nullptr) != 0) {
        std::cerr << "Error opening the video file" << '\n';
        return 1;
    }

    if (avformat_find_stream_info(formatContext, nullptr) < 0) {
        std::cerr << "Error finding the stream info" << '\n';
        return 1;
    }

    int videoStreamIndex = -1;

    for (unsigned int i = 0; i < formatContext->nb_streams; ++i) {
        if (formatContext->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO) {
            videoStreamIndex = i;
            break;
        }
    }

    if (videoStreamIndex == -1) {
        std::cerr << "Error finding the video stream" << '\n';
        return 1;
    }

    AVStream *videoStream = formatContext->streams[videoStreamIndex];
    const AVRational frameRate = av_guess_frame_rate(formatContext, videoStream, nullptr);
    const double fps = av_q2d(frameRate);

    const double targetFps = std::min(std::max(fps, MIN_FPS), MAX_FPS);

    const AVCodecParameters *codecParameters = videoStream->codecpar;
    const AVCodec *codec = avcodec_find_decoder(codecParameters->codec_id);

    if (codecParameters == nullptr || codec == nullptr) {
        std::cerr << "Error finding the decoder information" << '\n';
        return 1;
    }

    AVCodecContext *codecContext = avcodec_alloc_context3(codec);

    if (avcodec_parameters_to_context(codecContext, codecParameters) < 0) {
        std::cerr << "Error converting codec parameters to codec context" << '\n';
        return 1;
    }

    if (avcodec_open2(codecContext, codec, nullptr) < 0) {
        std::cerr << "Error opening the codec" << '\n';
        return 1;
    }

    const double aspectRatio = static_cast<double>(codecContext->height) / codecContext->width;
    const int outputHeight = static_cast<int>(OUTPUT_WIDTH * aspectRatio * 0.45);

    SwsContext *swsContext =
        sws_getContext(codecContext->width, codecContext->height, codecContext->pix_fmt, OUTPUT_WIDTH, outputHeight,
                       AV_PIX_FMT_RGB24, SWS_BILINEAR, nullptr, nullptr, nullptr);

    if (swsContext == nullptr) {
        std::cerr << "Error creating the sws context" << '\n';
        return 1;
    }

    AVFrame *frame = av_frame_alloc();
    AVFrame *rgbFrame = av_frame_alloc();
    AVPacket *packet = av_packet_alloc();

    if (frame == nullptr || rgbFrame == nullptr || packet == nullptr) {
        std::cerr << "Error allocating the frames and packet" << '\n';
        return 1;
    }

    const int rgbFrameSize = av_image_get_buffer_size(AV_PIX_FMT_RGB24, OUTPUT_WIDTH, outputHeight, 1);
    const auto *rgbFrameBuffer = static_cast<unsigned char *>(av_malloc(rgbFrameSize * sizeof(unsigned char)));

    av_image_fill_arrays(rgbFrame->data, rgbFrame->linesize, rgbFrameBuffer, AV_PIX_FMT_RGB24, OUTPUT_WIDTH, outputHeight, 1);

    std::vector<AsciiArt::ColoredPixel> asciiArt(static_cast<size_t>(OUTPUT_WIDTH * outputHeight));

    const auto frameDelay = std::chrono::milliseconds(static_cast<int>(1000 / targetFps));
    auto lastFrameTime = std::chrono::steady_clock::now();

    while (av_read_frame(formatContext, packet) >= 0) {
        if (packet->stream_index == videoStreamIndex) {
            if (avcodec_send_packet(codecContext, packet) < 0) {
                std::cerr << "Error sending a packet to the decoder" << '\n';
                break;
            }

            while (avcodec_receive_frame(codecContext, frame) >= 0) {
                sws_scale(swsContext, frame->data, frame->linesize, 0, codecContext->height, rgbFrame->data,
                        rgbFrame->linesize);

                asciiArt = AsciiArt::frameToAscii(rgbFrame, OUTPUT_WIDTH, outputHeight, 3);

                std::cout << "\033[H"; // Move cursor to top-left
                AsciiArt::printAsciiFrame(asciiArt, OUTPUT_WIDTH, outputHeight);
                std::cout.flush();

                const auto currentTime = std::chrono::steady_clock::now();
                const auto elapsedTime = std::chrono::duration_cast<std::chrono::milliseconds>(currentTime - lastFrameTime);

                if (elapsedTime < frameDelay) {
                    std::this_thread::sleep_for(frameDelay - elapsedTime);
                }

                lastFrameTime = std::chrono::steady_clock::now();
            }
        }
        av_packet_unref(packet);
    }

    sws_freeContext(swsContext);
    av_frame_free(&rgbFrame);
    av_frame_free(&frame);
    av_packet_free(&packet);
    avcodec_free_context(&codecContext);
    avformat_close_input(&formatContext);

    return 0;
}
