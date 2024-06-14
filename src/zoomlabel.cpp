﻿#include "zoomlabel.h"

#include <QDebug>
#include <QTimer>
#include <QEvent>
#include <QWheelEvent>
#include <QPixmap>
#include <QPainter>

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

const QPixmap *ZoomLabel::getPixmap() const
{
    return m_pixmap.get();
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

void ZoomLabel::mousePressEvent(QMouseEvent *ev)
{
    if (!m_handRemoveBackground || m_penModel == PenModel::UNKNOWN)
    {
        return;
    }

    if (ev->button() != Qt::LeftButton)
    {
        return;
    }

    m_drawing = true;
    m_lastPoint = ev->pos();
}

void ZoomLabel::mouseReleaseEvent(QMouseEvent *ev)
{
    if (!m_handRemoveBackground)
    {
        return;
    }

    if (ev->button() != Qt::LeftButton)
    {
        return;
    }

    m_drawing = false;
}

void ZoomLabel::mouseMoveEvent(QMouseEvent *ev)
{
    if (!m_handRemoveBackground)
    {
        return;
    }

    if (!m_drawing)

    {
        return;
    }

//    if (m_penModel == PenModel::PEN)
//    {
        QLine line(m_lastPoint, ev->pos());
        m_drawingHistory.push(line);
        m_lastPoint = ev->pos();
//    }

    drawOrEraseLine();
    return;
}

void ZoomLabel::drawOrEraseLine()
{
    if (m_drawingHistory.empty())
    {
        return;
    }

    std::shared_ptr<QPixmap> pixmap = std::make_shared<QPixmap>(*m_pixmap);
    QPainter painter(pixmap.get());
    painter.setPen(QPen(Qt::transparent, 20, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));

    if (m_penModel == PenModel::PEN)
    {
        painter.setCompositionMode(QPainter::CompositionMode_Clear);
        painter.drawLine(m_drawingHistory.top());
    }
    else if (m_penModel == PenModel::ERASE)
    {
        painter.setCompositionMode(QPainter::CompositionMode_SourceOver);
        painter.drawLine(m_drawingHistory.top());
    }

    QLabel::setPixmap(*pixmap);
    m_pixmap = pixmap;
}

void ZoomLabel::setPenModel(const PenModel &penModel)
{
    m_penModel = penModel;
}

void ZoomLabel::setHandRemoveBackground(bool handRemoveBackground)
{
    m_handRemoveBackground = handRemoveBackground;
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
