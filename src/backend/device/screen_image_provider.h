/*
 * screen_image_provider.h — QQuickImageProvider for M1 screen buffer
 *
 * Bridges the ScreenBuffer (QImage) to QML's Image element via
 * the "image://screen/" URL scheme.
 */

#ifndef SCREEN_IMAGE_PROVIDER_H
#define SCREEN_IMAGE_PROVIDER_H

#include <QQuickImageProvider>

class M1Device;

class ScreenImageProvider : public QQuickImageProvider {
public:
    ScreenImageProvider();

    void setDevice(M1Device *device) { m_device = device; }

    QImage requestImage(const QString &id, QSize *size,
                        const QSize &requestedSize) override;

private:
    M1Device *m_device = nullptr;
};

#endif // SCREEN_IMAGE_PROVIDER_H
