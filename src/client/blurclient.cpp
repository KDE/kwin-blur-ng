/*
    SPDX-FileCopyrightText: 2025 Aleix Pol Gonzalez <aleix.pol_gonzalez@mercerdes-benz.com>

    SPDX-License-Identifier: LGPL-2.1-or-later
*/

#include "blurclient.h"
#include <QGuiApplication>

inline wl_surface *surfaceForWindow(QWindow *window)
{
    if (!window) {
        return nullptr;
    }

    QPlatformNativeInterface *native = qGuiApp->platformNativeInterface();
    if (!native) {
        return nullptr;
    }
    window->create();
    return reinterpret_cast<wl_surface *>(native->nativeResourceForWindow(QByteArrayLiteral("surface"), window));
}

BlurSurface* BlurManager::surface(QWindow* window)
{
    Q_ASSERT(isInitialized());
    auto surface = surfaceForWindow(window);
    if (!surface) {
        qCWarning(KWINBLURNG_CLIENT) << "Cannot set mask to null surface" << window << surface;
        return nullptr;
    }
    auto &blurSurface = m_surfaces[window];
    if (!blurSurface) {
        blurSurface = new BlurSurface(window, get_blur(surface), this);
        connect(blurSurface, &BlurSurface::forgetSurface, this, [this] (QWindow *window) {
            delete m_surfaces.take(window);
        });
    }
    return blurSurface;
}
