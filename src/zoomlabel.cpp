#include "zoomlabel.h"

#include <QDebug>
#include <QTimer>
#include <QEvent>
#include <QWheelEvent>
#include <QPixmap>

ZoomLabel::ZoomLabel(QWidget *parent)
    : QLabel(parent)
{
    m_updateTimer = new QTimer(this);
    m_updateTimer->setSingleShot(true);
    connect(m_updateTimer, &QTimer::timeout,
            this, &ZoomLabel::sltOnScaleUpdate);
}

void ZoomLabel::setPixmap(const QPixmap &pixmap)
{
    m_scaleFactor = 1;
    m_pixmap = std::make_shared<QPixmap>(pixmap);
    QLabel::setPixmap(*m_pixmap);
}

void ZoomLabel::scaledPixmap(double scaleFactor)
{
    if (m_scaleFactor == scaleFactor)
    {
        return;
    }

    m_scaleFactor = scaleFactor;
    m_updateTimer->start(SCALED_DELAY_TIME);
}

bool ZoomLabel::event(QEvent *e)
{
    return QLabel::event(e);
}

void ZoomLabel::wheelEvent(QWheelEvent *event)
{
    if (event->angleDelta().y() > 0)
    {
        m_scaleFactor *= 1.1; // 放大10%
    }
    else
    {
        m_scaleFactor *= 0.9; // 缩小10%
    }

    m_updateTimer->start(SCALED_DELAY_TIME);
}

void ZoomLabel::sltOnScaleUpdate()
{
    if (!m_pixmap)
    {
        return;
    }

    int newWidth = m_pixmap->width() * m_scaleFactor;
    int newHeight = m_pixmap->height() * m_scaleFactor;
    std::shared_ptr<QPixmap> newPixmap = std::make_shared<QPixmap>(
                m_pixmap->scaled(newWidth, newHeight, Qt::KeepAspectRatio, Qt::SmoothTransformation));
    QLabel::setPixmap(*newPixmap);

    emit sigScaled(m_scaleFactor);
}
