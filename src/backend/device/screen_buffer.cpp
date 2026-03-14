/*
 * screen_buffer.cpp — u8g2 framebuffer to QImage converter
 */

#include "screen_buffer.h"

#include <cstring>

ScreenBuffer::ScreenBuffer()
{
    std::memset(m_buffer, 0, FB_SIZE);
}

bool ScreenBuffer::update(const QByteArray &data)
{
    if (data.size() != FB_SIZE) {
        return false;
    }
    std::memcpy(m_buffer, data.constData(), FB_SIZE);
    return true;
}

bool ScreenBuffer::pixel(int x, int y) const
{
    if (x < 0 || x >= WIDTH || y < 0 || y >= HEIGHT) {
        return false;
    }

    /*
     * u8g2 full-buffer tile layout:
     *
     * The buffer is organized as 8 tile rows, each 128 bytes wide.
     * tile_row = y / 8
     * bit_position = y % 8
     * byte_index = tile_row * 128 + x
     * LSB = topmost pixel in the tile
     */
    int tile_row = y / 8;
    int bit_pos  = y % 8;
    int byte_idx = tile_row * WIDTH + x;

    return (m_buffer[byte_idx] >> bit_pos) & 1;
}

QImage ScreenBuffer::toNativeImage() const
{
    QImage img(WIDTH, HEIGHT, QImage::Format_RGB32);

    /*
     * The firmware normalizes the framebuffer to logical display
     * orientation before sending (flips 180° when in U8G2_R2 mode).
     * So the PC always receives correctly-oriented data and can
     * render directly — no rotation logic needed here.
     */
    for (int y = 0; y < HEIGHT; ++y) {
        auto *line = reinterpret_cast<QRgb*>(img.scanLine(y));
        for (int x = 0; x < WIDTH; ++x) {
            bool on = pixel(x, y);
            line[x] = on ? m_fgColor.rgb() : m_bgColor.rgb();
        }
    }

    return img;
}

QImage ScreenBuffer::toImage() const
{
    QImage native = toNativeImage();

    // Upscale using nearest-neighbor for crisp pixel look
    return native.scaled(DISPLAY_WIDTH, DISPLAY_HEIGHT,
                         Qt::IgnoreAspectRatio,
                         Qt::FastTransformation);
}

void ScreenBuffer::setColors(const QColor &fg, const QColor &bg)
{
    m_fgColor = fg;
    m_bgColor = bg;
}
