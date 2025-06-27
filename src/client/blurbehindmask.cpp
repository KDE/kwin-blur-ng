/*
    SPDX-FileCopyrightText: 2023 Aleix Pol Gonzalez <aleix.pol_gonzalez@mercerdes-benz.com>

    SPDX-License-Identifier: LGPL-2.1-or-later
*/

#include "blurbehindmask.h"
#include <QGuiApplication>
#include <QPainter>
#include <QQuickWindow>
#include "blurclient.h"

BlurBehindMask::BlurBehindMask(QQuickItem *parent)
    : QQuickItem(parent)
{
    connect(this, &BlurBehindMask::intensityChanged, this, &BlurBehindMask::refresh);
}

BlurBehindMask::~BlurBehindMask()
{
}

void BlurBehindMask::geometryChange(const QRectF &newGeometry, const QRectF &oldGeometry)
{
    Q_UNUSED(newGeometry);
    Q_UNUSED(oldGeometry);
    QQuickItem::geometryChange(newGeometry, oldGeometry);

    if (m_mask) {
        m_mask->setGeometry({mapToGlobal({0, 0}), QSizeF{width(), height()}});
        m_mask->sendDone();
    }
}

void BlurBehindMask::componentComplete()
{
    m_completed = true;
    QQuickItem::componentComplete();
    refresh();

    connect(this, &QQuickItem::visibleChanged, this, &BlurBehindMask::refresh);
    connect(window(), &QWindow::visibilityChanged, this, &BlurBehindMask::refresh);
}

void BlurBehindMask::refresh()
{
    if (!m_completed || !window() || !window()->isVisible() || m_maskPath.isNull()) {
        m_mask.reset();
        return;
    }

    if (!isVisible() || !m_activated || width() <= 0 || height() <= 0 || m_intensity <= 0) {
        m_mask.reset();
        return;
    }

    const bool create = !m_mask;
    if (create) {
        m_mask = std::make_unique<BlurMask>(BlurManager::instance()->get_blur_mask());
        m_mask->setMask(m_maskImage);
    }
    m_mask->setIntensity(m_intensity);
    m_mask->setGeometry({mapToGlobal({0, 0}), QSizeF{width(), height()}});
    m_mask->sendDone();
    m_mask->setSurface(BlurManager::instance()->surface(window()));
}

void BlurBehindMask::setMaskPath(const QString &maskPath)
{
    if (m_maskPath == maskPath) {
        return;
    }
    m_maskPath = maskPath;
    setMask(QImage(m_maskPath));
    Q_EMIT maskPathChanged();
}

void BlurBehindMask::setMask(const QImage& mask)
{
    m_maskImage = mask.convertedTo(QImage::Format_Grayscale8);
    if (!m_mask) {
        refresh();
    } else {
        m_mask->setGeometry({mapToGlobal({0, 0}), QSizeF{width(), height()}});
        m_mask->setIntensity(m_intensity);
        m_mask->setMask(m_maskImage);
        m_mask->sendDone();
    }
    Q_EMIT maskChanged();
}
