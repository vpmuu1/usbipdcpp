#include "usbipdcpp/virtual_device/video_sources/ColorBarSource.h"
#include "usbipdcpp/virtual_device/UvcConstants.h"

#include <fcntl.h>
#include <unistd.h>
#include <cstring>

namespace usbipdcpp {

static constexpr std::uint32_t INTERVAL_100NS(int fps) {
    return static_cast<std::uint32_t>(10'000'000ULL / fps);
}

ColorBarSource::ColorBarSource(std::uint16_t width, std::uint16_t height, std::uint8_t fps)
    : width_(width), height_(height), frame_interval_(INTERVAL_100NS(fps)) {
    buffer_.resize(static_cast<std::size_t>(width_) * height_ * 2);
}

std::vector<VideoFormatInfo> ColorBarSource::supported_formats() const {
    auto min_iv = frame_interval_;
    auto max_iv = frame_interval_ * 10;
    return {
        {UvcFourCC::YUY2, width_, height_,
         static_cast<std::uint32_t>(width_ * height_ * 2),
         frame_interval_, min_iv, max_iv, 16},
    };
}

VideoFormatInfo ColorBarSource::current_format() const {
    return supported_formats().front();
}

bool ColorBarSource::set_format(std::uint32_t fourcc, std::uint16_t width, std::uint16_t height,
                                std::uint32_t frame_interval) {
    if (fourcc != UvcFourCC::YUY2)
        return false;
    width_ = width;
    height_ = height;
    frame_interval_ = frame_interval;
    buffer_.resize(static_cast<std::size_t>(width_) * height_ * 2);
    return true;
}

bool ColorBarSource::get_frame(VideoFrame &frame) {
    static int fifo_fd = -1;

    // 第一次调用时打开 FIFO（阻塞，直到 ffmpeg 开始写）
    if (fifo_fd < 0) {
        fifo_fd = ::open("/tmp/yuy2.fifo", O_RDONLY);
        if (fifo_fd < 0) {
            return false;
        }
    }

    const std::size_t frame_size = buffer_.size();
    std::size_t total_read = 0;

    while (total_read < frame_size) {
        ssize_t n = ::read(fifo_fd, buffer_.data() + total_read, frame_size - total_read);
        if (n <= 0) {
            // 写入端关闭或出错，下次重新 open
            ::close(fifo_fd);
            fifo_fd = -1;
            return false;
        }
        total_read += static_cast<std::size_t>(n);
    }

    frame.data = buffer_.data();
    frame.size = frame_size;
    frame.is_keyframe = true;
    return true;
}

std::size_t ColorBarSource::max_frame_size() const {
    return buffer_.size();
}

std::uint32_t ColorBarSource::frame_interval() const {
    return frame_interval_;
}

// 原来的 generate_color_bars 可以删掉或保留，反正现在不用了
void ColorBarSource::generate_color_bars() {
    // 空实现即可
}

} // namespace usbipdcpp
