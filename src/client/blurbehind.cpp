/*
    SPDX-FileCopyrightText: 2023 Aleix Pol Gonzalez <aleix.pol_gonzalez@mercerdes-benz.com>

    SPDX-License-Identifier: LGPL-2.1-or-later
*/

#include "blurbehind.h"
#include <QQuickItemGrabResult>
#include <QQuickWindow>
#include <QGuiApplication>
#include <QPainter>
#include <qpa/qplatformnativeinterface.h>
#include "shm.h"
#include "kwinblurngclientlogging.h"

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
        destroy();
    }

    void setMask(QImage &&mask) {
        if (m_mask == mask) {
            return;
        }

        m_mask = std::move(mask);
        m_maskBuffer = Shm::instance()->createBuffer(m_mask);
        if (m_maskBuffer) {
            set_mask(m_maskBuffer->object());
        } else {
            qCWarning(KWINBLURNG_CLIENT) << "Failed to create mask";
        }
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

    QRectF m_geo;
    QImage m_mask;
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

    BlurSurface *surface(QWindow *window)
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

private:
    QHash<QWindow *, BlurSurface *> m_surfaces;
};

BlurBehind::BlurBehind(QQuickItem *parent)
    : QQuickItem(parent)
{
}

BlurBehind::~BlurBehind()
{
}

void BlurBehind::refresh()
{
    if (!m_completed || !window() || !window()->isVisible()) {
        m_mask.reset();
        return;
    }

    if (!isVisible() || !m_activated || width() <= 0 || height() <= 0) {
        m_mask.reset();
        return;
    }

    bool anyVisibleChild = false;
    for (auto x : childItems()) {
        anyVisibleChild |= x->isVisible() && x->opacity() > 0.01;
    }
    if (!anyVisibleChild) {
        m_mask.reset();
        return;
    }

    QSharedPointer<QQuickItemGrabResult> grab = grabToImage();
    if (!grab) {
        return;
    }

    connect(grab.data(), &QQuickItemGrabResult::ready, this, [this, grab]() {
        if (!window()) {
            m_mask.reset();
            return;
        }
        auto image = grab->image().convertedTo(QImage::Format_Alpha8);
        if (!m_mask) {
            m_mask = std::make_unique<BlurMask>(BlurManager::instance()->get_blur_mask());
        }
        m_mask->setMask(std::move(image));
        m_mask->setGeometry({mapToGlobal({0, 0}), QSizeF{width(), height()}});
        m_mask->done();
        m_mask->setSurface(BlurManager::instance()->surface(window()));
    });

}

void BlurBehind::itemChange(ItemChange change, const ItemChangeData &value)
{
    QQuickItem::itemChange(change, value);
    refresh();
}

void BlurBehind::geometryChange(const QRectF &newGeometry, const QRectF &oldGeometry)
{
    Q_UNUSED(newGeometry);
    Q_UNUSED(oldGeometry);
    QQuickItem::geometryChange(newGeometry, oldGeometry);
    // TODO full refresh just when the size changes?
    refresh();
}

void BlurBehind::componentComplete()
{
    m_completed = true;
    QQuickItem::componentComplete();
    refresh();

    connect(this, &QQuickItem::visibleChanged, this, &BlurBehind::refresh);
    connect(window(), &QWindow::visibilityChanged, this, &BlurBehind::refresh);
}

#include "blurbehind.moc"
