/*
 * screen_image_provider.cpp — QQuickImageProvider for M1 screen buffer
 */

#include "screen_image_provider.h"
#include "m1_device.h"

ScreenImageProvider::ScreenImageProvider()
    : QQuickImageProvider(QQuickImageProvider::Image)
{
}

QImage ScreenImageProvider::requestImage(const QString &id, QSize *size,
                                          const QSize &requestedSize)
{
    Q_UNUSED(id);
    Q_UNUSED(requestedSize);

    if (!m_device) {
        QImage blank(512, 256, QImage::Format_RGB32);
        blank.fill(Qt::black);
        if (size) *size = blank.size();
        return blank;
    }

    QImage img = m_device->screenImage();
    if (size) *size = img.size();
    return img;
}
