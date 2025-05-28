/*
    SPDX-FileCopyrightText: 2023 Aleix Pol Gonzalez <aleix.pol_gonzalez@mercerdes-benz.com>

    SPDX-License-Identifier: LGPL-2.1-or-later
*/

#pragma once

#include <QImage>
#include <QQmlParserStatus>
#include <QQuickItem>
#include <QtQmlIntegration>

class BlurManager;
class BlurMask;

class BlurBehindMask : public QQuickItem
{
    Q_OBJECT
    QML_NAMED_ELEMENT(BlurBehindMask)
    QML_ADDED_IN_VERSION(1, 0)
    Q_PROPERTY(bool activated READ activated WRITE setActivated NOTIFY activatedChanged)
    Q_PROPERTY(QString maskPath READ maskPath WRITE setMaskPath NOTIFY maskPathChanged)
public:
    BlurBehindMask(QQuickItem *target = nullptr);
    ~BlurBehindMask() override;

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

    QString maskPath() const { return m_maskPath; }
    void setMaskPath(const QString &maskPath);

Q_SIGNALS:
    void activatedChanged();
    void maskPathChanged();

protected:
    void geometryChange(const QRectF &newGeometry, const QRectF &oldGeometry) override;

private:
    void refresh();
    bool m_completed = true;
    bool m_activated = true;
    QString m_maskPath;
    std::unique_ptr<BlurMask> m_mask;
};
