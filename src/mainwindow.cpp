#include "mainwindow.h"
#include "ui_mainwindow.h"

#include <QFileDialog>
#include <QPixmap>
#include <QPainter>
#include <QColor>
#include <QDebug>
#include <QTimer>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    m_originImage = std::make_shared<QImage>();
    m_scaledImage = std::make_shared<QImage>();
    m_pixelImage = std::make_shared<QImage>();

    m_ppuTimer = std::make_shared<QTimer>();
    m_ppuTimer->setSingleShot(true);
    connect(m_ppuTimer.get(), &QTimer::timeout,
            this, &MainWindow::sltOnPpuTimerOut);

    m_scaledTimer = std::make_shared<QTimer>();
    m_scaledTimer->setSingleShot(true);
    connect(m_scaledTimer.get(), &QTimer::timeout,
            this, &MainWindow::sltOnScaledTimerOut);

    ui->actionSave->setEnabled(false);

    ui->sliderPPU->setEnabled(false);
    ui->spinBoxPPU->setEnabled(false);

    ui->spinBoxWidth->setEnabled(false);
    ui->spinBoxHeight->setEnabled(false);
    ui->checkBoxLockAspectRatio->setEnabled(false);
    ui->btnResetSize->setEnabled(false);

    toolSetStyleSheetFile(":/style.qss");
}

MainWindow::~MainWindow()
{
    delete ui;
}

std::shared_ptr<QImage> MainWindow::convertToPixelImg(std::shared_ptr<QImage> originImge, int ppu)
{
    if (!originImge)
    {
        return nullptr;
    }

    if (ppu <= 0)
    {
        return nullptr;
    }

    std::shared_ptr<QImage> pixeImage = std::make_shared<QImage>(*originImge); // 创建原始图像的副本

    // 迭代像素块
    int blockSize = ppu;
    for (int y = 0; y < pixeImage->height(); y += blockSize)
    {
        for (int x = 0; x < pixeImage->width(); x += blockSize)
        {
            // 初始化块内平均颜色和计数
            int totalRed = 0, totalGreen = 0, totalBlue = 0;
            int count = 0;

            // 累加块内颜色分量
            for (int j = y; j < y + blockSize && j < pixeImage->height(); ++j)
            {
                for (int i = x; i < x + blockSize && i < pixeImage->width(); ++i)
                {
                    QColor pixelColor(pixeImage->pixel(i, j));
                    totalRed += pixelColor.red();
                    totalGreen += pixelColor.green();
                    totalBlue += pixelColor.blue();
                    count++;
                }
            }

            // 计算平均颜色
            int avgRed = totalRed / count;
            int avgGreen = totalGreen / count;
            int avgBlue = totalBlue / count;
            QColor avgColor(avgRed, avgGreen, avgBlue);

            // 设置块内所有像素为平均颜色
            for (int j = y; j < y + blockSize && j < m_scaledImage->height(); ++j)
            {
                for (int i = x; i < x + blockSize && i < m_scaledImage->width(); ++i)
                {
                    pixeImage->setPixel(i, j, avgColor.rgb());
                }
            }
        }
    }

    return pixeImage;
}

std::shared_ptr<QImage> MainWindow::scaledImg(std::shared_ptr<QImage> originImge, int width, int height)
{
    if (!originImge)
    {
        return nullptr;
    }

    std::shared_ptr<QImage> scaledImg = std::make_shared<QImage>(
                originImge->scaled(width, height, Qt::IgnoreAspectRatio, Qt::SmoothTransformation));
    return scaledImg;
}

double MainWindow::calculatePSNR(const QImage &originalImage, const QImage &processedImage)
{
    int width = originalImage.width();
    int height = originalImage.height();

    double sumSquaredError = 0.0;
    double maxPixelValue = 255.0;

    // Calculate MSE
    for (int y = 0; y < height; ++y)
    {
        for (int x = 0; x < width; ++x)
        {
            QColor colorOriginal(originalImage.pixel(x, y));
            QColor colorProcessed(processedImage.pixel(x, y));

            // Extract RGB components
            int rOriginal = colorOriginal.red();
            int gOriginal = colorOriginal.green();
            int bOriginal = colorOriginal.blue();

            int rProcessed = colorProcessed.red();
            int gProcessed = colorProcessed.green();
            int bProcessed = colorProcessed.blue();

            // Calculate squared error for each color channel
            double squaredErrorR = pow(rOriginal - rProcessed, 2);
            double squaredErrorG = pow(gOriginal - gProcessed, 2);
            double squaredErrorB = pow(bOriginal - bProcessed, 2);

            // Accumulate squared error
            sumSquaredError += squaredErrorR + squaredErrorG + squaredErrorB;
        }
    }

    // Calculate MSE
    double mse = sumSquaredError / (width * height * 3); // 乘以3是因为考虑了RGB三个通道

    // Calculate PSNR
    double psnr = 10.0 * log10((maxPixelValue * maxPixelValue) / mse);

    return psnr;

}

void MainWindow::toolSetStyleSheetFile(const QString &file)
{
    QFile styleSheet(file);
    if (!styleSheet.open(QFile::ReadOnly | QFile::Text))
    {
        return;
    }

    QTextStream stream(&styleSheet);
    setStyleSheet(stream.readAll());
    styleSheet.close();
}

void MainWindow::sltOnPpuTimerOut()
{
    std::shared_ptr<QImage> pixeImage = convertToPixelImg(m_originImage, ui->spinBoxPPU->value());
    if (!pixeImage)
    {
        return;
    }

    m_pixelImage = pixeImage;
    ui->labelScaledImg->setPixmap(QPixmap::fromImage(*m_pixelImage));
}

void MainWindow::sltOnScaledTimerOut()
{
    std::shared_ptr<QImage> scaledImage = scaledImg(m_pixelImage,
                                                    ui->spinBoxWidth->value(),
                                                    ui->spinBoxHeight->value());
    if (!scaledImage)
    {
        return;
    }

    m_scaledImage = scaledImage;
    ui->labelScaledImg->setPixmap(QPixmap::fromImage(*m_scaledImage));
}

void MainWindow::on_spinBoxPPU_valueChanged(int arg1)
{
    ui->sliderPPU->setValue(arg1);
    m_ppuTimer->start(PPU_DELAY_TIME);
}

void MainWindow::on_sliderPPU_sliderMoved(int position)
{
    ui->spinBoxPPU->setValue(position);
    m_ppuTimer->start(PPU_DELAY_TIME);
}

void MainWindow::on_spinBoxHeight_valueChanged(int arg1)
{
    if (ui->checkBoxLockAspectRatio->isChecked())
    {
        double aspectRatio = static_cast<double>(m_pixelImage->width()) / m_pixelImage->height();
        int newWidth = static_cast<int>(arg1 * aspectRatio);
        ui->spinBoxWidth->setValue(newWidth);
    }

    m_scaledTimer->start(SCALED_DELAY_TIME);
}

void MainWindow::on_spinBoxWidth_valueChanged(int arg1)
{
    if (ui->checkBoxLockAspectRatio->isChecked())
    {
        double aspectRatio = static_cast<double>(m_pixelImage->height()) / m_pixelImage->width();
        int newHeight = static_cast<int>(arg1 * aspectRatio);
        ui->spinBoxHeight->setValue(newHeight);
    }

    m_scaledTimer->start(SCALED_DELAY_TIME);
}

void MainWindow::on_actionBrowse_triggered()
{
    // 加载图像
    QString fileName = QFileDialog::getOpenFileName(this,
                                                    tr("Open Image"),
                                                    QString::fromLocal8Bit("E:/User/好看的图图/111"),
                                                    tr("Image Files (*.png *.jpg *.bmp)"));
    if (fileName.isEmpty())
    {
        return;
    }

    m_imgSuffix = QFileInfo(fileName).suffix();

    ui->labelPath->setText(fileName);

    m_originImage->load(fileName);
    ui->labelOriginImg->setPixmap(QPixmap::fromImage(*m_originImage));

    m_pixelImage = std::make_shared<QImage>(*m_originImage);
    m_scaledImage = std::make_shared<QImage>(*m_pixelImage);
    ui->labelScaledImg->setPixmap(QPixmap::fromImage(*m_scaledImage));

    ui->actionSave->setEnabled(true);

    ui->sliderPPU->setEnabled(true);
    ui->sliderPPU->setValue(ui->sliderPPU->minimum());
    ui->spinBoxPPU->setEnabled(true);
    ui->spinBoxPPU->setValue(ui->spinBoxPPU->minimum());

    ui->spinBoxWidth->setEnabled(true);
    ui->spinBoxWidth->setValue(m_originImage->width());
    ui->spinBoxHeight->setEnabled(true);
    ui->spinBoxHeight->setValue(m_originImage->height());
    ui->checkBoxLockAspectRatio->setEnabled(true);
    ui->btnResetSize->setEnabled(true);

    ui->statusbar->showMessage(QString::fromLocal8Bit("原始图像尺寸：%1 * %2").arg(m_originImage->width())
                               .arg(m_originImage->height()));
}

void MainWindow::on_actionSave_triggered()
{
    QString fileName = QFileDialog::getSaveFileName(this,
                                                    tr("Save Image"),
                                                    QString::fromLocal8Bit("E:/User/好看的图图/111"),
                                                    tr("*.") + m_imgSuffix);
    if (fileName.isEmpty())
    {
        return;
    }

    if (!m_scaledImage || !m_scaledImage->save(fileName))
    {
        return;
    }
}

void MainWindow::on_btnResetSize_clicked(bool checked)
{
    Q_UNUSED(checked);

    std::shared_ptr<QImage> scaledImage = scaledImg(m_pixelImage,
                                                    m_pixelImage->width(),
                                                    m_pixelImage->height());
    if (!scaledImage)
    {
        return;
    }

    m_scaledImage = scaledImage;
    ui->labelScaledImg->setPixmap(QPixmap::fromImage(*m_scaledImage));
    ui->spinBoxWidth->setValue(m_pixelImage->width());
    ui->spinBoxHeight->setValue(m_pixelImage->height());
}
