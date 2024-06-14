#ifndef ZOOMLABEL_H
#define ZOOMLABEL_H

#include <QLabel>
#include <QStack>

class QTimer;

enum class PenModel {
    PEN = 0,
    ERASE = 1,
    UNKNOWN = 2
};

class ZoomLabel : public QLabel
{
    Q_OBJECT
public:
   explicit ZoomLabel(QWidget *parent = nullptr);

public:
    void setPixmap(const QPixmap &pixmap);
    const QPixmap *getPixmap() const;
    void scaledPixmap(double scaleFactor);
    void setHandRemoveBackground(bool handRemoveBackground);
    void setPenModel(const PenModel &penModel);

private:
    virtual bool event(QEvent *e) override;
    virtual void wheelEvent(QWheelEvent *event) override;
    virtual void mousePressEvent(QMouseEvent *ev) override;
    virtual void mouseReleaseEvent(QMouseEvent *ev) override;
    virtual void mouseMoveEvent(QMouseEvent *ev) override;

    void drawOrEraseLine();

    double m_scaleFactor = 1;
    QTimer *m_updateTimer = nullptr;
    const int SCALED_DELAY_TIME = 100;
    std::shared_ptr<QPixmap> m_pixmap = nullptr;

    bool m_handRemoveBackground = false;
    bool m_drawing = false;
    PenModel m_penModel = PenModel::UNKNOWN;
    QStack<QLine> m_drawingHistory;
    QPoint m_lastPoint;

signals:
    void sigScaled(double m_scaleFactor);

private slots:
    void sltOnScaleUpdate();
};

#endif // ZOOMLABEL_H
