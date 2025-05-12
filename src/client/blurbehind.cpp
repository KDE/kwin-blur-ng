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
        , m_fullThing(window->size(), QImage::Format_Alpha8)
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

    void addMask(BlurBehind *b, QImage &&mask) {
        Q_ASSERT(b->window() == m_window);
        auto it = m_masks.find(b);
        if (it != m_masks.end()) {
            *it = std::move(mask);
        } else {
            m_masks.insert(b, std::move(mask));
            connect(b, &QObject::destroyed, this, [this, b] () {
                removeMask(b);
            });
        }
        refresh();
    }
    void removeMask(BlurBehind *b) {
        const int removed = m_masks.remove(b);
        if (removed > 0) {
            refresh();
        }
    }

Q_SIGNALS:
    void forgetSurface(QWindow *window);

private:
    void refresh()
    {
        if (m_masks.isEmpty()) {
            unset_mask();
            return;
        }
        m_fullThing.fill(Qt::transparent);

        QPainter p(&m_fullThing);
        for (const auto [item, mask] : std::as_const(m_masks).asKeyValueRange()) {
            const QPointF pos = item->mapToGlobal({0, 0});
            p.drawImage({pos, mask.size()}, mask, QRect());
        }

        m_maskBuffer = Shm::instance()->createBuffer(m_fullThing);
        if (m_maskBuffer) {
            // auto wlr = createRegion(region);
            set_mask(m_maskBuffer->object());
        } else {
            qCWarning(KWINBLURNG_CLIENT) << "Failed to create mask";
        }
    }

    friend class BlurManager;
    QImage m_fullThing;
    QWindow *const m_window;
    QHash<BlurBehind *, QImage> m_masks;
    std::unique_ptr<ShmBuffer> m_maskBuffer;
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

    void setBlur(QWindow *window, BlurBehind *item, QImage &&mask) {
        Q_ASSERT(isInitialized());
        auto surface = surfaceForWindow(window);
        if (!surface) {
            qCWarning(KWINBLURNG_CLIENT) << "Cannot set mask to null surface" << window << surface;
            resetBlur(item);
            return;
        }

        auto &blurSurface = m_surfaces[window];
        if (!blurSurface) {
            blurSurface = new BlurSurface(window, get_blur(surface), this);
            connect(blurSurface, &BlurSurface::forgetSurface, this, [this] (QWindow *window) {
                delete m_surfaces.take(window);
            });
        }
        blurSurface->addMask(item, std::move(mask));
    }

    void resetBlur(BlurBehind *b)
    {
        Q_ASSERT(b);
        auto it = m_surfaces.find(b->window());
        if (it == m_surfaces.end()) {
            return;
        }
        (*it)->removeMask(b);
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
    BlurManager::instance()->resetBlur(this);
}

void BlurBehind::refresh()
{
    // TODO: Should probably combine and centralise all the blur regions on the window
    if (!m_completed || !window() || !window()->isVisible()) {
        return;
    }

    if (!isVisible() || !m_activated || width() <= 0 || height() <= 0) {
        BlurManager::instance()->resetBlur(this);
        return;
    }

    QSharedPointer<QQuickItemGrabResult> grab = grabToImage();
    if (!grab) {
        return;
    }
    connect(grab.data(), &QQuickItemGrabResult::ready, this, [this, grab]() {
        auto image = grab->image().convertedTo(QImage::Format_Alpha8);
        BlurManager::instance()->setBlur(window(), this, std::move(image));
    });

}

void BlurBehind::itemChange(ItemChange change, const ItemChangeData &value)
{
    Q_UNUSED(change);
    Q_UNUSED(value);
    QQuickItem::itemChange(change, value);
    refresh();
}

void BlurBehind::geometryChange(const QRectF &newGeometry, const QRectF &oldGeometry)
{
    Q_UNUSED(newGeometry);
    Q_UNUSED(oldGeometry);
    QQuickItem::geometryChange(newGeometry, oldGeometry);
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
