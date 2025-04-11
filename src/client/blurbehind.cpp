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
static wl_region *createRegion(const QRegion &region)
{
    QPlatformNativeInterface *native = qGuiApp->platformNativeInterface();
    if (!native) {
        return nullptr;
    }
    auto compositor = reinterpret_cast<wl_compositor *>(native->nativeResourceForIntegration(QByteArrayLiteral("compositor")));
    if (!compositor) {
        return nullptr;
    }
    auto wl_region = wl_compositor_create_region(compositor);
    for (const auto &rect : region) {
        wl_region_add(wl_region, rect.x(), rect.y(), rect.width(), rect.height());
    }
    return wl_region;
}

class BlurSurface : public QObject, public QtWayland::mbition_blur_surface_v1
{
public:
    BlurSurface(struct ::mbition_blur_surface_v1 *object, QObject *parent)
        : QObject(parent)
        , QtWayland::mbition_blur_surface_v1(object)
    {
    }

    ~BlurSurface() override
    {
        destroy();
    }

    void setMask(QImage &&mask) {
        Q_ASSERT(!mask.isNull());
        m_maskBuffer = Shm::instance()->createBuffer(std::move(mask));
        Q_ASSERT(m_maskBuffer);

        // auto wlr = createRegion(region);
        set_mask(m_maskBuffer->object());
    }

private:
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

    void setBlur(QWindow *window, QImage &&mask) {
        Q_ASSERT(isInitialized());
        Q_ASSERT(window);
        auto surface = surfaceForWindow(window);
        if (!window) {
            qWarning() << "Cannot set mask to null surface" << window << surface;
            resetBlur(window);
            return;
        }
        auto &blurSurface = m_surfaces[window];
        if (!blurSurface) {
            blurSurface = new BlurSurface(get_blur(surface), this);
        }
        blurSurface->setMask(std::move(mask));
    }

    void resetBlur(QWindow *window)
    {
        Q_ASSERT(window);
        delete m_surfaces.take(window);
    }

private:
    QHash<QWindow *, BlurSurface *> m_surfaces;
};

BlurBehind::BlurBehind(QQuickItem *parent)
    : QQuickItem(parent)
{
}

BlurBehind::~BlurBehind() = default;

void BlurBehind::refresh()
{
    // TODO: Should probably combine and centralise all the blur regions on the window
    if (!m_completed || !window()) {
        return;
    }

    if (!isVisible() || !m_activated) {
        BlurManager::instance()->resetBlur(window());
        return;
    }

    QSharedPointer<QQuickItemGrabResult> grab = grabToImage();
    if (!grab) {
        return;
    }
    connect(grab.data(), &QQuickItemGrabResult::ready, this, [this, grab]() {
        auto image = grab->image().convertedTo(QImage::Format_Alpha8);
        // We normalise against the item's opacity
        // if (opacity() != 1) {
        //     for (int y = 0, h = image.height(); y < h; ++y) {
        //         auto line = image.scanLine(y);
        //         for (int x = 0, w = image.width(); x < w; ++x) {
        //             auto nx = line[x] / opacity();
        //             qDebug() << "wwwwwww" << line[x] << "opacity" << opacity() << "=" << nx;
        //             line[x] = nx;
        //         }
        //     }
        // }
        // qDebug() << "waaaaaa" << opacity() << image.save(QStringLiteral("/tmp/potatolo.png"));

        // TODO optimise
        QImage fullThing(window()->size(), QImage::Format_Alpha8);
        fullThing.fill(Qt::transparent);
        const QPointF pos = mapToGlobal({0, 0});
        {
            QPainter p(&fullThing);
            p.drawImage({pos, image.size()}, image, QRect());
        }

        // static int i = 0;
        // fullThing.save(QStringLiteral("/tmp/ppp-%1.png").arg(i++));

        // TODO only borders?
        // QRegion region = QRect({0, 0}, fullThing.size());
        // for (int y = pos.y(); y < pos.y() + height(); ++y) {
        //     for (int x = pos.x(); x < pos.x() + width(); ++x) {
        //         if (qAlpha(fullThing.pixel(x, y)) == 0) { // Check if the pixel is transparent
        //             region -= QRegion(x, y, 1, 1);
        //         }
        //     }
        // }

        BlurManager::instance()->setBlur(window(), std::move(fullThing));
    });

}

void BlurBehind::itemChange(ItemChange change, const ItemChangeData &value)
{
    Q_UNUSED(change);
    Q_UNUSED(value);
    refresh();
}

void BlurBehind::geometryChange(const QRectF &newGeometry, const QRectF &oldGeometry)
{
    Q_UNUSED(newGeometry);
    Q_UNUSED(oldGeometry);
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
