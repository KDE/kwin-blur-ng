/*
    SPDX-FileCopyrightText: 2023 Aleix Pol Gonzalez <aleix.pol_gonzalez@mercerdes-benz.com>

    SPDX-License-Identifier: LGPL-2.1-or-later
*/

#ifndef BLURBEHIND_H
#define BLURBEHIND_H

#include <QQmlParserStatus>
#include <QQuickItem>
#include <QtQmlIntegration>

class BlurManager;
class BlurMask;

class BlurBehind : public QQuickItem
{
    Q_OBJECT
    QML_NAMED_ELEMENT(BlurBehind)
    QML_ADDED_IN_VERSION(1, 0)
    Q_PROPERTY(bool activated READ activated WRITE setActivated NOTIFY activatedChanged)
    Q_PROPERTY(qreal intensity MEMBER m_intensity NOTIFY intensityChanged)
public:
    BlurBehind(QQuickItem *target = nullptr);
    ~BlurBehind() override;

    void componentComplete() override;
    bool activated() const { return m_activated; }
    void setActivated(bool activated) {
        if (m_activated == activated) {
            return;
        }
        m_activated = activated;
        refresh();
        Q_EMIT activatedChanged();
    }
Q_SIGNALS:
    void activatedChanged();
    void intensityChanged(qreal intensity);

protected:
    void itemChange(ItemChange change, const ItemChangeData &value) override;
    void geometryChange(const QRectF &newGeometry, const QRectF &oldGeometry) override;

private:
    void refresh();
    bool m_completed = true;
    bool m_activated = true;
    std::unique_ptr<BlurMask> m_mask;
    QSharedPointer<QQuickItemGrabResult> m_lastGrab;
    bool m_schedule = false;
    qreal m_intensity = 1.;
};

#endif
