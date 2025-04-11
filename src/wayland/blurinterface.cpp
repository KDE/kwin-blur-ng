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
#include <core/graphicsbuffer.h>
#include <core/graphicsbufferview.h>

#include "qwayland-server-mbition-blur-v1.h"

namespace KWin
{
static const quint32 s_version = 1;

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
    GraphicsBufferRef m_buffer;
    SurfaceInterface *const m_surface;
    std::shared_ptr<GLTexture> m_texture;

    bool loadShmTexture()
    {
        const GraphicsBufferView view(m_buffer.buffer());
        if (Q_UNLIKELY(view.isNull())) {
            return false;
        }

        m_texture = GLTexture::upload(*view.image());
        return m_texture.get();
    }

    void updateShmTexture(GraphicsBuffer *buffer, const QRegion &region)
    {
        const GraphicsBufferView view(buffer);
        if (Q_UNLIKELY(view.isNull())) {
            return;
        }

        for (const QRect &rect : region) {
            m_texture->update(*view.image(), rect.topLeft(), rect);
        }
    }

protected:
    void mbition_blur_surface_v1_destroy(Resource *resource) override;
    void mbition_blur_surface_v1_destroy_resource(Resource *resource) override;
    void mbition_blur_surface_v1_set_mask(Resource *resource, struct ::wl_resource *mask) override {
        m_texture.reset();
        m_buffer = Display::bufferForResource(mask);
        Q_ASSERT(m_buffer->shmAttributes()); //TODO add support for dmabuf?
        Q_EMIT q->blurChanged(m_surface);
    }

    void mbition_blur_surface_v1_unset_mask(Resource * /*resource*/) override {
        m_buffer = {};
        m_texture.reset();
        Q_EMIT q->blurChanged(m_surface);
    }
};

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
        wl_resource_post_error(resource->handle, MBITION_BLUR_MANAGER_V1_ERROR_BLUR_EXISTS, "the surface already has been provided");
        return;
    }
    blur = new BlurNGSurfaceInterface(blur_resource, s);
    q->connect(blur, &BlurNGSurfaceInterface::blurChanged, q, &BlurNGManagerInterface::blurChanged);
}

BlurNGManagerInterface::BlurNGManagerInterface(Display *display, QObject *parent)
    : QObject(parent)
    , d(new BlurNGManagerInterfacePrivate(this, display))
{
}

BlurNGManagerInterface::~BlurNGManagerInterface()
{
}

std::shared_ptr<GLTexture> BlurNGManagerInterface::mask(SurfaceInterface *surface) const
{
    auto x = d->m_blurs[surface];
    if (!x) {
        return nullptr;
    }
    if (!x->d->m_texture) {
        x->d->loadShmTexture();
    }
    return x->d->m_texture;
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

}

#include "wayland/moc_blurinterface.cpp"

