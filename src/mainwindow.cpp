#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "src/zoomlabel.h"

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
    m_resImage = std::make_shared<QImage>();

    m_ppuTimer = std::make_shared<QTimer>();
    m_ppuTimer->setSingleShot(true);
    connect(m_ppuTimer.get(), &QTimer::timeout,
            this, &MainWindow::sltOnPpuTimerOut);

    m_scaledTimer = std::make_shared<QTimer>();
    m_scaledTimer->setSingleShot(true);
    connect(m_scaledTimer.get(), &QTimer::timeout,
            this, &MainWindow::sltOnScaledTimerOut);

    m_removeBackgroundTimer = std::make_shared<QTimer>();
    m_removeBackgroundTimer->setSingleShot(true);
    connect(m_removeBackgroundTimer.get(), &QTimer::timeout,
            this, &MainWindow::sltOnRemoveBackgroundTimerOut);

    toolSwitchUIState(false);

    toolSetStyleSheetFile(":/style.qss");

    // 对图片缩放设置
    ui->btnResetSizeViewBase->setHidden(true);
    ui->btnResetSizeViewCompare->setHidden(true);
    toolConnectViewLabels();

    setHandRemoveModel(false);
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
            for (int j = y; j < y + blockSize && j < pixeImage->height(); ++j)
            {
                for (int i = x; i < x + blockSize && i < pixeImage->width(); ++i)
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
    if (!originImge || width <=0 || height <= 0)
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

std::shared_ptr<QImage> MainWindow::removeBackground(std::shared_ptr<QImage> originImge,
                                                     const QColor &bgColor,
                                                     int tolerance)
{
    std::shared_ptr<QImage> result = std::make_shared<QImage>(
                originImge->convertToFormat(QImage::Format_ARGB32));
    for (int y = 0; y < result->height(); ++y)
    {
        for (int x = 0; x < result->width(); ++x)
        {
            QColor pixelColor = result->pixelColor(x, y);
            // 判断当前像素是否接近背景颜色
            if (qAbs(pixelColor.red() - bgColor.red()) < tolerance &&
                    qAbs(pixelColor.green() - bgColor.green()) < tolerance &&
                    qAbs(pixelColor.blue() - bgColor.blue()) < tolerance)
            {
                // 将背景像素设置为透明
                result->setPixelColor(x, y, QColor(0, 0, 0, 0));
            }
        }
    }
    return result;
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

void MainWindow::toolSwitchUIState(bool isEnabled)
{
    ui->actionSave->setEnabled(isEnabled);

    ui->sliderPPU->setEnabled(isEnabled);
    ui->sliderPPU->setValue(ui->sliderPPU->minimum());
    ui->spinBoxPPU->setEnabled(isEnabled);
    ui->spinBoxPPU->setValue(ui->spinBoxPPU->minimum());

    ui->spinBoxWidth->setEnabled(isEnabled);
    if (m_originImage)
    {
        ui->spinBoxWidth->setValue(m_originImage->width());
    }
    ui->spinBoxHeight->setEnabled(isEnabled);
    if (m_originImage)
    {
        ui->spinBoxHeight->setValue(m_originImage->height());
    }
    ui->checkBoxLockAspectRatio->setEnabled(isEnabled);
    ui->btnResetSize->setEnabled(isEnabled);

    ui->spinBoxTolerance->setEnabled(isEnabled);
    ui->spinBoxTolerance->setValue(ui->spinBoxTolerance->minimum());
    ui->btnResetBackground->setEnabled(isEnabled);

    ui->checkBoxViewSize->setEnabled(isEnabled);
    ui->checkBoxRemoveByHandCheckbox->setEnabled(isEnabled);
}

void MainWindow::toolConnectViewLabels()
{
    connect(ui->labelOriginImg, &ZoomLabel::sigScaled,
            [this](double scaleFactor)
    {
        ui->labelScaledImg->scaledPixmap(scaleFactor);
        if (!ui->btnResetSizeViewBase->isVisible())
        {
            ui->btnResetSizeViewBase->setHidden(false);
        }
    });
    connect(ui->labelScaledImg, &ZoomLabel::sigScaled,
            [this](double scaleFactor)
    {
        ui->labelOriginImg->scaledPixmap(scaleFactor);
        if (!ui->btnResetSizeViewCompare->isVisible())
        {
            ui->btnResetSizeViewCompare->setHidden(false);
        }
    });
}

void MainWindow::setHandRemoveModel(bool handRemove)
{
    // 图片手动去背景设置
    ui->gridFrameHandRemove->setHidden(!handRemove);
    ui->frameConfiguration->setHidden(handRemove);
    ui->labelScaledImg->setHandRemoveBackground(handRemove);

    if (!handRemove)
    {
        ui->labelScaledImg->setPenModel(PenModel::UNKNOWN);
    }
}

void MainWindow::sltOnPpuTimerOut()
{
    std::shared_ptr<QImage> pixeImage = convertToPixelImg(m_originImage, ui->spinBoxPPU->value());
    if (!pixeImage)
    {
        return;
    }

    m_pixelImage = pixeImage;
    m_resImage = pixeImage;
    ui->labelScaledImg->setPixmap(QPixmap::fromImage(*pixeImage));

    ui->spinBoxWidth->setValue(pixeImage->width());
    ui->spinBoxHeight->setValue(pixeImage->height());
    ui->spinBoxTolerance->setValue(ui->spinBoxTolerance->minimum());
}

void MainWindow::sltOnScaledTimerOut()
{
    std::shared_ptr<QImage> scaledImage = scaledImg(m_pixelImage,
                                                    ui->spinBoxWidth->value(),
                                                    ui->spinBoxHeight->value());
    if (!scaledImage)
    {
        ui->labelScaledImg->clear();
        return;
    }

    m_scaledImage = scaledImage;
    m_resImage = scaledImage;
    ui->labelScaledImg->setPixmap(QPixmap::fromImage(*scaledImage));
    ui->spinBoxTolerance->setValue(ui->spinBoxTolerance->minimum());
}

void MainWindow::sltOnRemoveBackgroundTimerOut()
{
    std::shared_ptr<QImage> scaledImage = m_scaledImage;
    if (scaledImage->size() != m_pixelImage->size())
    {
        scaledImage = scaledImg(m_pixelImage,
                                ui->spinBoxWidth->value(),
                                ui->spinBoxHeight->value());
        if (!scaledImage)
        {
            return;
        }
    }

    std::shared_ptr<QImage> removeImg = removeBackground(scaledImage,
                                                    QColor(255, 255, 255),
                                                    ui->spinBoxTolerance->value());
    if (!removeImg)
    {
        return;
    }

    m_removeImage = m_resImage;
    m_resImage = removeImg;
    ui->labelScaledImg->setPixmap(QPixmap::fromImage(*removeImg));
}

void MainWindow::on_spinBoxPPU_valueChanged(int arg1)
{
    ui->sliderPPU->setValue(arg1);
    m_ppuTimer->start(PPU_DELAY_TIME);
}

void MainWindow::on_spinBoxHeight_valueChanged(int arg1)
{
    if (m_scaleUpdating)
    {
        return;
    }

    if (ui->checkBoxLockAspectRatio->isChecked())
    {
        m_scaleUpdating = true;
        int newWidth = static_cast<int>(arg1 * m_scaleRatioWToH);
        ui->spinBoxWidth->setValue(newWidth);
        m_scaleUpdating = false;
    }

    m_scaledTimer->start(SCALED_DELAY_TIME);
}

void MainWindow::on_spinBoxWidth_valueChanged(int arg1)
{
    if (m_scaleUpdating)
    {
        return;
    }

    if (ui->checkBoxLockAspectRatio->isChecked())
    {
        m_scaleUpdating = true;
        int newHeight = static_cast<int>(arg1 * m_scaleRatioHToW);
        ui->spinBoxHeight->setValue(newHeight);
        m_scaleUpdating = false;
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
    m_resImage = std::make_shared<QImage>(*m_originImage);
    ui->labelScaledImg->setPixmap(QPixmap::fromImage(*m_resImage));

    m_scaleRatioWToH = m_originImage->width() * 1.0  / m_originImage->height() * 1.0;
    m_scaleRatioHToW = m_originImage->height() * 1.0  / m_originImage->width() * 1.0;

    toolSwitchUIState(true);

    if (!m_statusLabel)
    {
        m_statusLabel = new QLabel(ui->statusbar);
        m_statusLabel->setObjectName("statusLabel");
        ui->statusbar->addPermanentWidget(m_statusLabel);
    }
    m_statusLabel->setText(QString::fromLocal8Bit("原始图像尺寸：%1 * %2").arg(m_originImage->width())
                           .arg(m_originImage->height()));

    ui->btnResetSizeViewBase->setHidden(true);
    ui->btnResetSizeViewCompare->setHidden(true);

    ui->checkBoxRemoveByHandCheckbox->setChecked(false);
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

    if (!m_resImage || !m_resImage->save(fileName))
    {
        return;
    }
}

void MainWindow::on_btnResetSize_clicked(bool checked)
{
    Q_UNUSED(checked);

    ui->spinBoxWidth->setValue(0);
    ui->spinBoxHeight->setValue(0);

    ui->spinBoxWidth->setValue(m_originImage->width());
    ui->spinBoxHeight->setValue(m_originImage->height());
}

void MainWindow::on_btnResetBackground_clicked()
{
    ui->spinBoxTolerance->setValue(ui->spinBoxTolerance->minimum());
}

void MainWindow::on_spinBoxTolerance_valueChanged(int arg1)
{
    Q_UNUSED(arg1);

    m_removeBackgroundTimer->start(REMOVE_BACKGROUND_DELAY_TIME);
}

void MainWindow::on_sliderPPU_valueChanged(int value)
{
    ui->spinBoxPPU->setValue(value);
    m_ppuTimer->start(PPU_DELAY_TIME);
}

void MainWindow::on_checkBoxLockAspectRatio_toggled(bool checked)
{
    Q_UNUSED(checked);

//    if (checked)
//    {
//        int minHeight = qMax(static_cast<int>(m_scaleRatioHToW), 0);
//        int minWidth = qMax(static_cast<int>(m_scaleRatioWToH), 0);
//        ui->spinBoxWidth->setMinimum(minHeight);
//        ui->spinBoxHeight->setMinimum(minWidth);
//    }
//    else
//    {
//        ui->spinBoxWidth->setMinimum(0);
//        ui->spinBoxHeight->setMinimum(0);
//    }
}

void MainWindow::on_btnResetSizeViewBase_clicked(bool checked)
{
    Q_UNUSED(checked);

    ui->labelOriginImg->scaledPixmap(1);
}

void MainWindow::on_btnResetSizeViewCompare_clicked(bool checked)
{
    Q_UNUSED(checked);

    ui->labelScaledImg->scaledPixmap(1);
}

void MainWindow::on_checkBoxViewSize_toggled(bool checked)
{
    if (checked)
    {
        toolConnectViewLabels();
    }
    else
    {
        disconnect(ui->labelOriginImg, &ZoomLabel::sigScaled, nullptr, nullptr);
        disconnect(ui->labelScaledImg, &ZoomLabel::sigScaled, nullptr, nullptr);
    }
}

void MainWindow::on_checkBoxRemoveByHandCheckbox_toggled(bool checked)
{
    setHandRemoveModel(checked);
}

void MainWindow::on_btnQuitHandRemove_clicked()
{
    ui->checkBoxRemoveByHandCheckbox->setChecked(false);

    ui->radioBtnPen->setChecked(false);
    ui->radioBtnEraser->setChecked(false);
}

void MainWindow::on_radioBtnPen_toggled(bool checked)
{
    if (!checked)
    {
        return;
    }

    ui->labelScaledImg->setPenModel(PenModel::PEN);
}

void MainWindow::on_radioBtnEraser_toggled(bool checked)
{
    if (!checked)
    {
        return;
    }

    ui->labelScaledImg->setPenModel(PenModel::ERASE);
}
