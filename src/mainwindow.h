#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();


private:
    Ui::MainWindow *ui;
    std::shared_ptr<QImage> m_originImage = nullptr;
    std::shared_ptr<QImage> m_pixelImage = nullptr;
    std::shared_ptr<QImage> m_scaledImage = nullptr;
    QString m_imgSuffix;

    std::shared_ptr<QTimer> m_ppuTimer = nullptr;
    const int PPU_DELAY_TIME = 100;

    std::shared_ptr<QTimer> m_scaledTimer = nullptr;
    const int SCALED_DELAY_TIME = 100;

    std::shared_ptr<QImage> convertToPixelImg(std::shared_ptr<QImage> originImge, int ppu);
    std::shared_ptr<QImage> scaledImg(std::shared_ptr<QImage> originImge, int width, int height);
    double calculatePSNR(const QImage& originalImage, const QImage& processedImage);

    void toolSetStyleSheetFile(const QString &file);

private slots:
    void sltOnPpuTimerOut();
    void sltOnScaledTimerOut();

private slots:
    void on_spinBoxPPU_valueChanged(int arg1);
    void on_sliderPPU_sliderMoved(int position);
    void on_spinBoxHeight_valueChanged(int arg1);
    void on_spinBoxWidth_valueChanged(int arg1);
    void on_actionBrowse_triggered();
    void on_actionSave_triggered();
    void on_btnResetSize_clicked(bool checked);
};
#endif // MAINWINDOW_H
