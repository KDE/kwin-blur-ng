/*
    SPDX-FileCopyrightText: 2015 Martin Gräßlin <mgraesslin@kde.org>
    SPDX-FileCopyrightText: 2015 Marco Martin <mart@kde.org>
    SPDX-FileCopyrightText: 2020 David Edmundson <davidedmundson@kde.org>
    SPDX-FileCopyrightText: 2025 Aleix Pol Gonzalez <aleix.pol_gonzalez@mbition.io>

    SPDX-License-Identifier: LGPL-2.1-only OR LGPL-3.0-only OR LicenseRef-KDE-Accepted-LGPL
*/
#pragma once

#include "kwin_export.h"

#include <QObject>
#include <memory>

struct wl_resource;

namespace KWin
{
class BlurNGSurfaceInterface;
class BlurNGManagerInterfacePrivate;
class BlurNGSurfaceInterfacePrivate;
class BlurNGMaskInterfacePrivate;
class Display;
class GLTexture;
class GraphicsBufferRef;
class SurfaceInterface;

class BlurNGManagerInterface : public QObject
{
    Q_OBJECT
public:
    explicit BlurNGManagerInterface(Display *display, QObject *parent = nullptr);
    ~BlurNGManagerInterface() override;

    BlurNGSurfaceInterface *surface(SurfaceInterface *surface) const;
    void remove();

Q_SIGNALS:
    void blurChanged(SurfaceInterface *s);

private:
    std::unique_ptr<BlurNGManagerInterfacePrivate> d;
};

/**
 * @brief Represents the Resource for the mbition_blur_surface_v1 interface.
 *
 * Lifespan matches the underlying client resource
 */
class BlurNGSurfaceInterface : public QObject
{
    Q_OBJECT
public:
    ~BlurNGSurfaceInterface() override;

    std::shared_ptr<GLTexture> mask() const;
    QRegion region() const;

Q_SIGNALS:
    void blurChanged(SurfaceInterface *s);

private:
    explicit BlurNGSurfaceInterface(wl_resource *resource, SurfaceInterface *s);
    friend class BlurNGManagerInterface;
    friend class BlurNGManagerInterfacePrivate;

    std::unique_ptr<BlurNGSurfaceInterfacePrivate> d;
};

/**
 * @brief Represents the Resource for the mbition_blur_mask_v1 interface.
 *
 * Lifespan matches the underlying client resource
 */
class BlurNGMaskInterface : public QObject
{
    Q_OBJECT
public:
    ~BlurNGMaskInterface() override;

    QRect geometry() const;
    GraphicsBufferRef buffer() const;

Q_SIGNALS:
    void aboutToBeDestroyed();
    void maskChanged();

private:
    explicit BlurNGMaskInterface(wl_resource *resource);
    friend class BlurNGManagerInterface;
    friend class BlurNGManagerInterfacePrivate;
    friend class BlurNGSurfaceInterfacePrivate;

    std::unique_ptr<BlurNGMaskInterfacePrivate> d;
};

}
