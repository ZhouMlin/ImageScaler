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
    std::shared_ptr<QImage> m_removeImage = nullptr;
    std::shared_ptr<QImage> m_resImage = nullptr;
    QString m_imgSuffix;

    std::shared_ptr<QTimer> m_ppuTimer = nullptr;
    const int PPU_DELAY_TIME = 100;

    std::shared_ptr<QTimer> m_scaledTimer = nullptr;
    const int SCALED_DELAY_TIME = 100;

    std::shared_ptr<QTimer> m_removeBackgroundTimer = nullptr;
    const int REMOVE_BACKGROUND_DELAY_TIME = 100;

    std::shared_ptr<QImage> convertToPixelImg(std::shared_ptr<QImage> originImge, int ppu);
    std::shared_ptr<QImage> scaledImg(std::shared_ptr<QImage> originImge, int width, int height);
    double calculatePSNR(const QImage& originalImage, const QImage& processedImage);
    std::shared_ptr<QImage> removeBackground(std::shared_ptr<QImage> originImge,
                                             const QColor &bgColor, int tolerance);

    void toolSetStyleSheetFile(const QString &file);
    void toolSwitchUIState(bool isEnabled);

private slots:
    void sltOnPpuTimerOut();
    void sltOnScaledTimerOut();
    void sltOnRemoveBackgroundTimerOut();

private slots:
    void on_spinBoxPPU_valueChanged(int arg1);
    void on_spinBoxHeight_valueChanged(int arg1);
    void on_spinBoxWidth_valueChanged(int arg1);
    void on_actionBrowse_triggered();
    void on_actionSave_triggered();
    void on_btnResetSize_clicked(bool checked);
    void on_btnResetBackground_clicked();
    void on_spinBoxTolerance_valueChanged(int arg1);
    void on_sliderPPU_valueChanged(int value);
};
#endif // MAINWINDOW_H
