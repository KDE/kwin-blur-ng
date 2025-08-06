/*
    SPDX-FileCopyrightText: 2025 Aleix Pol Gonzalez <aleix.pol_gonzalez@mercerdes-benz.com>

    SPDX-License-Identifier: LGPL-2.1-or-later
*/

#pragma once

#include <QHash>
#include <QPointer>
#include <QWaylandClientExtensionTemplate>
#include <QWindow>
#include <qpa/qplatformnativeinterface.h>
#include "qwayland-mbition-blur-v1.h"
#include "kwinblurngclientlogging.h"
#include "shm.h"

class BlurSurface : public QObject, public QtWayland::mbition_blur_surface_v1
{
    Q_OBJECT
public:
    BlurSurface(QWindow *window, struct ::mbition_blur_surface_v1 *object, QObject *parent)
        : QObject(parent)
        , QtWayland::mbition_blur_surface_v1(object)
        , m_window(window)
    {
        connect(window, &QWindow::visibilityChanged, this, [this] {
            if (m_window->visibility() != QWindow::Hidden) {
                return;
            }
            Q_EMIT forgetSurface(m_window);
        });
    }

    ~BlurSurface() override
    {
        destroy();
    }

Q_SIGNALS:
    void forgetSurface(QWindow *window);

private:

    friend class BlurManager;
    QWindow *const m_window;
};

class BlurMask : public QtWayland::mbition_blur_mask_v1
{
public:
    BlurMask(struct ::mbition_blur_mask_v1 *object)
        : QtWayland::mbition_blur_mask_v1(object)
    {
        Q_ASSERT(object);
    }
    ~BlurMask()
    {
        qDebug() << "destroy mask!";
        destroy();
    }

    void setMask(const QImage &mask) {
        if (m_mask == mask) {
            return;
        }

        m_mask = mask;
        m_dirty = true;
    }
    void setIntensity(qreal intensity) {
        m_intensity = intensity;
        m_dirty = true;
    }
    void setGeometry(const QRectF& geo) {
        if (geo == m_geo)
            return;
        m_geo = geo;
        set_geometry(geo.x(), geo.y(), geo.width(), geo.height());
    }

    void setSurface(BlurSurface *surface)
    {
        Q_ASSERT(isInitialized());
        if (m_surface == surface) {
            return;
        }

        m_surface = surface;
        if (surface) {
            Q_ASSERT(object());
            m_surface->add_mask(object());
        }
    }

    void sendMask();
    void sendDone() {
        if (m_dirty) {
            sendMask();
        }
        done();
    }

    qreal m_intensity = 1;
    QRectF m_geo;
    QImage m_mask;
    bool m_dirty = true;
    std::unique_ptr<ShmBuffer> m_maskBuffer;
    QPointer<BlurSurface> m_surface;
};

class BlurManager : public QWaylandClientExtensionTemplate<BlurManager>, public QtWayland::mbition_blur_manager_v1
{
public:
    BlurManager()
        : QWaylandClientExtensionTemplate<BlurManager>(1)
    {
        initialize();
    }

    static BlurManager *instance() {
        static BlurManager m;
        return &m;
    }

    BlurSurface *surface(QWindow *window);

private:
    QHash<QWindow *, BlurSurface *> m_surfaces;
};
