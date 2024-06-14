#ifndef ZOOMLABEL_H
#define ZOOMLABEL_H

#include <QLabel>

class QTimer;
class ZoomLabel : public QLabel
{
    Q_OBJECT
public:
   explicit ZoomLabel(QWidget *parent = nullptr);

public:
    void setPixmap(const QPixmap &pixmap);
    void scaledPixmap(double scaleFactor);

private:
    virtual bool event(QEvent *e) override;
    virtual void wheelEvent(QWheelEvent *event) override;

    double m_scaleFactor = 1;
    QTimer *m_updateTimer = nullptr;
    const int SCALED_DELAY_TIME = 100;
    std::shared_ptr<QPixmap> m_pixmap = nullptr;

signals:
    void sigScaled(double m_scaleFactor);

private slots:
    void sltOnScaleUpdate();
};

#endif // ZOOMLABEL_H
