/*
    SPDX-FileCopyrightText: 2015 Martin Gräßlin <mgraesslin@kde.org>
    SPDX-FileCopyrightText: 2015 Marco Martin <mart@kde.org>
    SPDX-FileCopyrightText: 2020 David Edmundson <davidedmundson@kde.org>

    SPDX-License-Identifier: LGPL-2.1-only OR LGPL-3.0-only OR LicenseRef-KDE-Accepted-LGPL
*/
#include "blurinterface.h"
#include <wayland/display.h>
#include <wayland/surface.h>
#include <opengl/gltexture.h>
#include <opengl/glframebuffer.h>
#include <opengl/glshader.h>
#include <opengl/glshadermanager.h>
#include <core/graphicsbuffer.h>
#include <core/graphicsbufferview.h>
#include "qwayland-server-mbition-blur-v1.h"
#include <kwinblurng_debug.h>

namespace KWin
{
static const quint32 s_version = 1;

class BlurNGMaskInterfacePrivate : public QtWaylandServer::mbition_blur_mask_v1
{
public:
    BlurNGMaskInterfacePrivate(BlurNGMaskInterface *q, wl_resource *resource)
        : QtWaylandServer::mbition_blur_mask_v1(resource)
        , q(q)
    {}

    void mbition_blur_mask_v1_destroy(Resource * resource) override {
        m_geometry = {};
        m_texture.reset();
        Q_EMIT q->maskChanged();
        wl_resource_destroy(resource->handle);
    }
    void mbition_blur_mask_v1_destroy_resource(Resource * resource) override {
        delete q;
    }
    void mbition_blur_mask_v1_set_mask(Resource *resource, struct ::wl_resource *mask) override
    {
        m_buffer = Display::bufferForResource(mask);
        if (!m_buffer.buffer()) [[unlikely]] {
            qCWarning(KWIN_BLUR) << "received empty mask buffer";
        }
        m_texture.reset();
        m_dirty = true;
    }

    void mbition_blur_mask_v1_set_geometry(Resource * resource, int32_t x, int32_t y, uint32_t width, uint32_t height) override
    {
        const auto geo = QRect(x, y, width, height);
        if (m_geometry == geo) {
            return;
        }
        m_geometry = geo;
        m_dirty = true;
    }

    void mbition_blur_mask_v1_done(Resource * resource) override
    {
        if (!m_dirty) {
            return;
        }
        Q_EMIT q->maskChanged();
        m_dirty = false;
    }

    std::shared_ptr<GLTexture> texture() {
        if (!m_texture) {
            if (!m_buffer.buffer()) {
                qCWarning(KWIN_BLUR) << "empty mask buffer";
                return nullptr;
            }
            GraphicsBufferView view(m_buffer.buffer());
            if (view.isNull()) {
                qCWarning(KWIN_BLUR) << "empty mask";
                return nullptr;
            }
            m_texture = GLTexture::upload(*view.image());
        }
        return m_texture;
    }

    BlurNGMaskInterface *const q;
    bool m_dirty = true;
    QRect m_geometry;
    GraphicsBufferRef m_buffer;
    std::shared_ptr<GLTexture> m_texture;
};

class BlurNGManagerInterfacePrivate : public QtWaylandServer::mbition_blur_manager_v1
{
public:
    BlurNGManagerInterfacePrivate(BlurNGManagerInterface *q, Display *d);

    BlurNGManagerInterface *q;

    void mbition_blur_manager_v1_destroy_global() override
    {
        delete q;
    }

    void mbition_blur_manager_v1_get_blur(Resource *resource, uint32_t id, struct ::wl_resource *surface) override;
    void mbition_blur_manager_v1_get_blur_mask(Resource *resource, uint32_t id) override;

    QHash<SurfaceInterface *, BlurNGSurfaceInterface *> m_blurs;
};

class BlurNGSurfaceInterfacePrivate : public QtWaylandServer::mbition_blur_surface_v1
{
public:
    BlurNGSurfaceInterfacePrivate(BlurNGSurfaceInterface *q, wl_resource *resource, SurfaceInterface *s)
        : QtWaylandServer::mbition_blur_surface_v1(resource)
        , q(q)
        , m_surface(s)
    {
    }

    BlurNGSurfaceInterface *const q;
    SurfaceInterface *const m_surface;
    std::shared_ptr<GLTexture> m_texture;
    QVector<BlurNGMaskInterface *> m_masks;
    std::unique_ptr<GLFramebuffer> m_fbo;

    bool loadShmTexture(const QRegion &update)
    {
        if (m_masks.empty()) {
            m_texture.reset();
            return true;
        }

        if (m_masks.size() == 1) {
            m_texture = m_masks[0]->d->texture();
            return true;
        }

        const QRect reg = q->region().boundingRect();

        if (!m_texture || m_texture->size() != reg.size()) {
            m_texture = GLTexture::allocate(GL_R8, reg.size());
            m_fbo = std::make_unique<GLFramebuffer>(m_texture.get());
        }
        m_texture->clear();

        QRectF worldRect = {{0, 0}, reg.size()};

        GLFramebuffer::pushFramebuffer(m_fbo.get());
        m_texture->bind();
        glEnable(GL_BLEND);

        for (auto mask : std::as_const(m_masks)) {
            const QRectF geo = mask->geometry();
            // if (!update.contains(geo)) {
                // continue;
            // }

            // for (auto rect : update) ???


            ShaderBinder shaderBinder(ShaderTrait::MapTexture);
            QMatrix4x4 projectionMatrix;
            projectionMatrix.scale(1, -1);
            projectionMatrix.ortho(worldRect);
            QMatrix4x4 maskProjectionMatrix = projectionMatrix;
            maskProjectionMatrix.translate(geo.left() - reg.left(), geo.top() - reg.top());
            shaderBinder.shader()->setUniform(GLShader::Mat4Uniform::ModelViewProjectionMatrix, maskProjectionMatrix);
            auto tex = mask->d->texture();
            if (tex) [[likely]] {
                tex->render({{0,0}, tex->size()}, update, geo.size());
            }
        }
        glDisable(GL_BLEND);
        GLFramebuffer::popFramebuffer();
        return m_texture.get();
    }

protected:
    void mbition_blur_surface_v1_destroy(Resource *resource) override;
    void mbition_blur_surface_v1_destroy_resource(Resource *resource) override;
    void mbition_blur_surface_v1_add_mask(Resource */*resource*/, struct ::wl_resource *maskResource) override {
        m_texture.reset();
        auto mask = static_cast<BlurNGMaskInterfacePrivate *>(BlurNGMaskInterfacePrivate::Resource::fromResource(maskResource)->object())->q;
        QObject::connect(mask, &BlurNGMaskInterface::maskChanged, q, &BlurNGSurfaceInterface::scheduleBlurChanged);
        QObject::connect(mask, &BlurNGMaskInterface::aboutToBeDestroyed, q, [this, mask] {
            m_masks.removeAll(mask);
            q->scheduleBlurChanged();
        });
        m_masks.append(mask);
        q->scheduleBlurChanged();
    }
};

void BlurNGSurfaceInterface::scheduleBlurChanged()
{
    Q_ASSERT(d->m_surface);
    // Synchronise it with the surface commit
    connect(d->m_surface, &SurfaceInterface::committed, this, &BlurNGSurfaceInterface::emitBlurChanged, Qt::UniqueConnection);
}

void BlurNGSurfaceInterface::emitBlurChanged()
{
    disconnect(d->m_surface, &SurfaceInterface::committed, this, &BlurNGSurfaceInterface::emitBlurChanged);
    Q_EMIT blurChanged(d->m_surface);
}

BlurNGManagerInterfacePrivate::BlurNGManagerInterfacePrivate(BlurNGManagerInterface *_q, Display *d)
    : QtWaylandServer::mbition_blur_manager_v1(*d, s_version)
    , q(_q)
{
}

void BlurNGManagerInterfacePrivate::mbition_blur_manager_v1_get_blur(Resource *resource, uint32_t id, wl_resource *surface)
{
    SurfaceInterface *s = SurfaceInterface::get(surface);
    if (!s) {
        wl_resource_post_error(resource->handle, 0, "Invalid surface");
        return;
    }
    wl_resource *blur_resource = wl_resource_create(resource->client(), &mbition_blur_surface_v1_interface, resource->version(), id);
    if (!blur_resource) {
        wl_client_post_no_memory(resource->client());
        return;
    }
    auto &blur = m_blurs[s];
    if (blur) {
        wl_resource_post_error(resource->handle, MBITION_BLUR_MANAGER_V1_ERROR_BLUR_EXISTS, "the surface has already been provided");
        return;
    }
    blur = new BlurNGSurfaceInterface(blur_resource, s);
    q->connect(blur, &BlurNGSurfaceInterface::blurChanged, q, &BlurNGManagerInterface::blurChanged);
    q->connect(s, &SurfaceInterface::aboutToBeDestroyed, q, [this, s] {
        m_blurs.remove(s);
    });
}

void BlurNGManagerInterfacePrivate::mbition_blur_manager_v1_get_blur_mask(Resource *resource, uint32_t id)
{
    wl_resource *newResource = wl_resource_create(resource->client(), &mbition_blur_mask_v1_interface, resource->version(), id);
    if (!newResource) {
        wl_client_post_no_memory(resource->client());
        return;
    }
    new BlurNGMaskInterface(newResource);
}

BlurNGManagerInterface::BlurNGManagerInterface(Display *display, QObject *parent)
    : QObject(parent)
    , d(new BlurNGManagerInterfacePrivate(this, display))
{
}

BlurNGManagerInterface::~BlurNGManagerInterface()
{
}

BlurNGSurfaceInterface *BlurNGManagerInterface::surface(SurfaceInterface *surface) const
{
    return d->m_blurs[surface];
}

std::shared_ptr<GLTexture> BlurNGSurfaceInterface::mask() const
{
    d->loadShmTexture(QRect{{0,0}, d->m_surface->size().toSize()});
    return d->m_texture;
}

QRegion BlurNGSurfaceInterface::region() const
{
    QRegion region;
    for (auto mask : std::as_const(d->m_masks)) {
        region |= mask->geometry();
    }
    return region;
}

void BlurNGManagerInterface::remove()
{
    d->globalRemove();
}

void BlurNGSurfaceInterfacePrivate::mbition_blur_surface_v1_destroy(Resource *resource)
{
    wl_resource_destroy(resource->handle);
}

void BlurNGSurfaceInterfacePrivate::mbition_blur_surface_v1_destroy_resource(Resource */*resource*/)
{
    delete q;
}

BlurNGSurfaceInterface::BlurNGSurfaceInterface(wl_resource *resource, SurfaceInterface *s)
    : QObject()
    , d(new BlurNGSurfaceInterfacePrivate(this, resource, s))
{
}

BlurNGSurfaceInterface::~BlurNGSurfaceInterface() = default;

BlurNGMaskInterface::BlurNGMaskInterface(wl_resource *resource)
    : QObject()
    , d(new BlurNGMaskInterfacePrivate(this, resource))
{
}

BlurNGMaskInterface::~BlurNGMaskInterface()
{
    Q_EMIT aboutToBeDestroyed();
}

GraphicsBufferRef BlurNGMaskInterface::buffer() const
{
    return d->m_buffer;
}

QRect BlurNGMaskInterface::geometry() const
{
    return d->m_geometry;
}

}

#include "wayland/moc_blurinterface.cpp"

