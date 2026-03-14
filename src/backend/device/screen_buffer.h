/*
 * screen_buffer.h — u8g2 framebuffer to QImage converter
 *
 * The M1 uses a 128x64 monochrome display driven by u8g2 in full
 * framebuffer mode. The buffer is 1024 bytes in u8g2's tile layout:
 *
 *   - 16 tile columns (128 pixels / 8)
 *   - 8 tile rows (64 pixels / 8)
 *   - Each tile is 8 bytes (one byte per pixel column, 8 vertical pixels)
 *   - Within each byte, LSB is the topmost pixel
 *
 * Layout:  buf[tile_row * 128 + tile_col * 8 + x_within_tile]
 *          Each byte: bit 0 = top pixel, bit 7 = bottom pixel
 */

#ifndef SCREEN_BUFFER_H
#define SCREEN_BUFFER_H

#include <QImage>
#include <QByteArray>
#include <QColor>
#include <cstdint>

class ScreenBuffer {
public:
    static constexpr int WIDTH  = 128;
    static constexpr int HEIGHT = 64;
    static constexpr int FB_SIZE = WIDTH * HEIGHT / 8;  // 1024 bytes
    static constexpr int SCALE  = 4;  // upscale factor
    static constexpr int DISPLAY_WIDTH  = WIDTH * SCALE;   // 512
    static constexpr int DISPLAY_HEIGHT = HEIGHT * SCALE;   // 256

    ScreenBuffer();

    /**
     * Update the internal buffer from raw u8g2 framebuffer data.
     * @param data  1024 bytes of u8g2 tile-format framebuffer
     * @return true if data was valid (exactly 1024 bytes)
     */
    bool update(const QByteArray &data);

    /**
     * Get the current frame as a QImage, upscaled to display size.
     * Uses green-on-black color scheme to match LCD appearance.
     */
    QImage toImage() const;

    /**
     * Get the current frame at native 128x64 resolution.
     */
    QImage toNativeImage() const;

    /**
     * Check if a pixel is set at the given coordinates.
     */
    bool pixel(int x, int y) const;

    /**
     * Set foreground (pixel on) and background colors.
     */
    void setColors(const QColor &fg, const QColor &bg);

private:
    uint8_t m_buffer[FB_SIZE] = {};
    QColor  m_fgColor{0x00, 0xFF, 0x66};  // green LCD phosphor
    QColor  m_bgColor{0x0A, 0x0A, 0x0A};  // near-black
};

#endif // SCREEN_BUFFER_H
