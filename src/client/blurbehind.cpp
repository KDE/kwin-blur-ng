/*
    SPDX-FileCopyrightText: 2023 Aleix Pol Gonzalez <aleix.pol_gonzalez@mercerdes-benz.com>

    SPDX-License-Identifier: LGPL-2.1-or-later
*/

#include "blurbehind.h"
#include <QQuickItemGrabResult>
#include <QQuickWindow>
#include <QGuiApplication>
#include <QPainter>
#include "blurclient.h"

BlurBehind::BlurBehind(QQuickItem *parent)
    : QQuickItem(parent)
{
    connect(this, &BlurBehind::intensityChanged, this, &BlurBehind::refresh);
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

    if (!isVisible() || !m_activated || width() <= 0 || height() <= 0 || m_intensity < 0.01) {
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

    if (m_lastGrab) {
        m_schedule = true;
        return;
    }

    m_lastGrab = grabToImage();
    if (!m_lastGrab) {
        return;
    }

    connect(m_lastGrab.data(), &QQuickItemGrabResult::ready, this, [this]() {
        if (!window()) {
            m_mask.reset();
            return;
        }
        auto image = m_lastGrab->image().convertedTo(QImage::Format_Alpha8);
        if (!m_mask) {
            m_mask = std::make_unique<BlurMask>(BlurManager::instance()->get_blur_mask());
        }
        m_mask->setIntensity(m_intensity);
        m_mask->setGeometry({mapToGlobal({0, 0}), QSizeF{width(), height()}});
        m_mask->setMask(std::move(image));
        m_mask->sendDone();
        m_mask->setSurface(BlurManager::instance()->surface(window()));
        if (m_schedule) {
            QTimer::singleShot(0, this, &BlurBehind::refresh);
            m_schedule = false;
        }
        m_lastGrab.reset();
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
