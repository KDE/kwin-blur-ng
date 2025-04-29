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
            Q_EMIT forgetMask(m_window);
        });
    }

    ~BlurSurface() override
    {
        destroy();
    }

    void addMask(BlurBehind *b, QImage &&mask) {
        Q_ASSERT(b->window() == m_window);
        auto it = std::find_if(m_masks.begin(), m_masks.end(), [b] (const auto &m) {
            return m.m_item == b;
        });
        if (it != m_masks.end()) {
            it->m_mask = std::move(mask);
        } else {
            m_masks += {b, std::move(mask)};
        }
        refresh();
    }
    void removeMask(BlurBehind *b) {
        auto it = std::find_if(m_masks.begin(), m_masks.end(), [b] (const auto &m) {
            return m.m_item == b;
        });
        if (it != m_masks.end()) {
            m_masks.erase(it);
            refresh();
        }
    }

Q_SIGNALS:
    void forgetMask(QWindow *window);

private:
    void refresh()
    {
        if (m_masks.isEmpty()) {
            Q_EMIT forgetMask(m_window);
            return;
        }
        QImage fullThing(m_window->size(), QImage::Format_Alpha8);
        fullThing.fill(Qt::transparent);

        for (const Mask &m : std::as_const(m_masks)) {
            QPainter p(&fullThing);
            const QPointF pos = m.m_item->mapToGlobal({0, 0});
            p.drawImage({pos, m.m_mask.size()}, m.m_mask, QRect());
        }

        // static int i = 0;
        // fullThing.save(QStringLiteral("/tmp/bananator%1.png").arg(++i));
        setMask(std::move(fullThing));
    }

    void setMask(QImage &&mask) {
        if (mask.isNull()) {
            Q_EMIT forgetMask(m_window);
        }
        m_maskBuffer = Shm::instance()->createBuffer(std::move(mask));
        if (m_maskBuffer) {
            // auto wlr = createRegion(region);
            set_mask(m_maskBuffer->object());
        } else {
            qCWarning(KWINBLURNG_CLIENT) << "Failed to create mask";
        }
    }

    QWindow *const m_window;
    struct Mask {
        BlurBehind *m_item;
        QImage m_mask;
    };
    QVector<Mask> m_masks;
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
        Q_ASSERT(window);
        auto surface = surfaceForWindow(window);
        if (!window) {
            qCWarning(KWINBLURNG_CLIENT) << "Cannot set mask to null surface" << window << surface;
            resetBlur(item);
            return;
        }

        auto &blurSurface = m_surfaces[window];
        if (!blurSurface) {
            blurSurface = new BlurSurface(window, get_blur(surface), this);
            connect(blurSurface, &BlurSurface::forgetMask, this, [this] (QWindow *window) {
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
