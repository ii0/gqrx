/* -*- c++ -*- */
/*
 * Gqrx SDR: Software defined radio receiver powered by GNU Radio and Qt
 *           http://gqrx.dk/
 *
 * Copyright 2011-2013 Alexandru Csete OZ9AEC.
 *
 * Gqrx is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3, or (at your option)
 * any later version.
 *
 * Gqrx is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Gqrx; see the file COPYING.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street,
 * Boston, MA 02110-1301, USA.
 */
#include <QString>
#include <QSettings>
#include <QDebug>
#include <QVariant>
#include "dockfft.h"
#include "ui_dockfft.h"

#define DEFAULT_FFT_MAXIMUM_DB   -0.0
#define DEFAULT_FFT_MINIMUM_DB   -135.0
#define DEFAULT_FFT_RATE         25
#define DEFAULT_FFT_SIZE         8192
#define DEFAULT_FFT_SPLIT        35
#define DEFAULT_FFT_AVG          75
#define DEFAULT_FFT_REF_LEVEL    0
#define DEFAULT_FFT_RANGE        130

DockFft::DockFft(QWidget *parent) :
    QDockWidget(parent),
    ui(new Ui::DockFft)
{
    ui->setupUi(this);

#ifdef Q_OS_MAC
    // Is this really only needed on Mac to make the color picker button appear square like the other buttons?
    ui->fillButton->setAttribute(Qt::WA_LayoutUsesWidgetRect);
#if QT_VERSION < 0x050000
    // Workaround for Mac, see http://stackoverflow.com/questions/3978889/why-is-qhboxlayout-causing-widgets-to-overlap
    // Fixed in Qt 5?
    ui->resetButton->setAttribute(Qt::WA_LayoutUsesWidgetRect);
    ui->centerButton->setAttribute(Qt::WA_LayoutUsesWidgetRect);
    ui->demodButton->setAttribute(Qt::WA_LayoutUsesWidgetRect);
#endif
#endif

#ifdef Q_OS_LINUX
    // buttons can be smaller than 50x32
    ui->peakDetectionButton->setMinimumSize(48, 24);
    ui->peakHoldButton->setMinimumSize(48, 24);
    ui->resetButton->setMinimumSize(48, 24);
    ui->centerButton->setMinimumSize(48, 24);
    ui->demodButton->setMinimumSize(48, 24);
    ui->fillButton->setMinimumSize(48, 24);
    ui->colorPicker->setMinimumSize(48, 24);
#endif

    m_sample_rate = 0.f;

    // Add predefined gqrx colors to chooser.
    ui->colorPicker->insertColor(QColor(0xFF,0xFF,0xFF,0xFF), "White");
    ui->colorPicker->insertColor(QColor(0xFA,0xFA,0x7F,0xFF), "Yellow");
    ui->colorPicker->insertColor(QColor(0x97,0xD0,0x97,0xFF), "Green");
    ui->colorPicker->insertColor(QColor(0xFF,0xC8,0xC8,0xFF), "Pink");
    ui->colorPicker->insertColor(QColor(0xB7,0xE0,0xFF,0xFF), "Blue");
    ui->colorPicker->insertColor(QColor(0x7F,0xFA,0xFA,0xFF), "Cyan");
}
DockFft::~DockFft()
{
    delete ui;
}


/**
 * @brief Get current FFT rate setting.
 * @return The current FFT rate in frames per second (always non-zero)
 */
int DockFft::fftRate()
{
    bool ok;
    int fps = 10;
    QString strval = ui->fftRateComboBox->currentText();

    strval.remove(" fps");
    fps = strval.toInt(&ok, 10);

    if (!ok)
        qDebug() << "DockFft::fftRate : Could not convert" <<
                    strval << "to number.";
    else
        qDebug() << "New FFT rate:" << fps << "Hz";

    return fps;
}

/**
 * @brief Select new FFT rate in the combo box.
 * @param rate The new rate.
 * @returns The actual FFT rate selected.
 */
int DockFft::setFftRate(int fft_rate)
{
    int idx = -1;
    QString rate_str = QString("%1 fps").arg(fft_rate);

    qDebug() << __func__ << "to" << rate_str;

    idx = ui->fftRateComboBox->findText(rate_str, Qt::MatchExactly);
    if(idx != -1)
        ui->fftRateComboBox->setCurrentIndex(idx);

    updateInfoLabels();
    return fftRate();
}

/**
 * @brief Select new FFT size in the combo box.
 * @param rate The new FFT size.
 * @returns The actual FFT size selected.
 */
int DockFft::setFftSize(int fft_size)
{
    int idx = -1;
    QString size_str = QString::number(fft_size);

    qDebug() << __func__ << "to" << size_str;

    idx = ui->fftSizeComboBox->findText(size_str, Qt::MatchExactly);
    if(idx != -1)
        ui->fftSizeComboBox->setCurrentIndex(idx);

    updateInfoLabels();
    return fftSize();
}

void DockFft::setSampleRate(float sample_rate)
{
    if (sample_rate < 0.1f)
        return;

    m_sample_rate = sample_rate;
    updateInfoLabels();
}

/**
 * @brief Get current FFT rate setting.
 * @return The current FFT rate in frames per second (always non-zero)
 */
int DockFft::fftSize()
{
    bool ok;
    int fft_size = 10;
    QString strval = ui->fftSizeComboBox->currentText();

    fft_size = strval.toInt(&ok, 10);

    if (!ok)
    {
        qDebug() << __func__ << "could not convert" << strval << "to number.";
    }

    if (fft_size == 0)
    {
        qDebug() << "Somehow we ended up with FFT size = 0; using" << DEFAULT_FFT_SIZE;
        fft_size = DEFAULT_FFT_SIZE;
    }

    return fft_size;
}

/** Save FFT settings. */
void DockFft::saveSettings(QSettings *settings)
{
    int  intval;

    if (!settings)
        return;

    settings->beginGroup("fft");

    intval = fftSize();
    if (intval != DEFAULT_FFT_SIZE)
        settings->setValue("fft_size", intval);
    else
        settings->remove("fft_size");

    intval = fftRate();
    if (intval != DEFAULT_FFT_RATE)
        settings->setValue("fft_rate", fftRate());
    else
        settings->remove("fft_rate");

    if (ui->fftAvgSlider->value() != DEFAULT_FFT_AVG)
        settings->setValue("averaging", ui->fftAvgSlider->value());
    else
        settings->remove("averaging");

    if (ui->fftSplitSlider->value() != DEFAULT_FFT_SPLIT)
        settings->setValue("split", ui->fftSplitSlider->value());
    else
        settings->remove("split");

    QColor fftColor = ui->colorPicker->currentColor();
    if (fftColor != QColor(0xFF,0xFF,0xFF,0xFF))
        settings->setValue("pandapter_color", fftColor);
    else
        settings->remove("pandapter_color");

    if (ui->fillButton->isChecked())
        settings->setValue("pandapter_fill", true);
    else
        settings->remove("pandapter_fill");

    if (ui->reflevelSlider->value() != DEFAULT_FFT_REF_LEVEL)
        settings->setValue("reference_level", ui->reflevelSlider->value());
    else
        settings->remove("reference_level");

    if (ui->rangeSlider->value() != DEFAULT_FFT_RANGE)
        settings->setValue("fft_range", ui->rangeSlider->value());
    else
        settings->remove("fft_range");

    settings->endGroup();
}

/** Read FFT settings. */
void DockFft::readSettings(QSettings *settings)
{
    int intval;
    bool bool_val = false;
    bool conv_ok = false;
    QColor color;

    if (!settings)
        return;

    settings->beginGroup("fft");

    intval = settings->value("fft_rate", DEFAULT_FFT_RATE).toInt(&conv_ok);
    if (conv_ok)
        setFftRate(intval);

    intval = settings->value("fft_size", DEFAULT_FFT_SIZE).toInt(&conv_ok);
    if (conv_ok)
        setFftSize(intval);

    intval = settings->value("averaging", DEFAULT_FFT_AVG).toInt(&conv_ok);
    if (conv_ok)
        ui->fftAvgSlider->setValue(intval);

    intval = settings->value("split", DEFAULT_FFT_SPLIT).toInt(&conv_ok);
    if (conv_ok)
        ui->fftSplitSlider->setValue(intval);

    color = settings->value("pandapter_color", QColor(0xFF,0xFF,0xFF,0xFF)).value<QColor>();
    ui->colorPicker->setCurrentColor(color);

    bool_val = settings->value("pandapter_fill", false).toBool();
    ui->fillButton->setChecked(bool_val);

    int ref = settings->value("reference_level", DEFAULT_FFT_REF_LEVEL).toInt(&conv_ok);
    int range = settings->value("fft_range", DEFAULT_FFT_RANGE).toInt(&conv_ok);
    setFftRange(ref, range);
    emit fftRangeChanged(ui->reflevelSlider->value(), ui->rangeSlider->value());

    settings->endGroup();
}

void DockFft::setFftRange(float reflevel, float range)
{
    ui->reflevelSlider->blockSignals(true);
    ui->reflevelSlider->setValue((int)reflevel);
    ui->reflevelSlider->blockSignals(false);
    ui->reflevelLabel->setText(QString("%1 dB").arg((int)reflevel));

    ui->rangeSlider->blockSignals(true);
    ui->rangeSlider->setValue((int)range);
    ui->rangeSlider->blockSignals(false);
    ui->rangeLabel->setText(QString("%1 dB").arg((int)range));
}

/** FFT size changed. */
void DockFft::on_fftSizeComboBox_currentIndexChanged(const QString &text)
{
    int value = text.toInt();
    emit fftSizeChanged(value);
    updateInfoLabels();
}

/** FFT rate changed. */
void DockFft::on_fftRateComboBox_currentIndexChanged(const QString & text)
{
    int fps = fftRate();
    Q_UNUSED(text);

    emit fftRateChanged(fps);
    updateInfoLabels();
}


static const quint64 wf_span_table[] =
{
    0,              // Auto
    5*60*1000,      // 5 minutes
    10*60*1000,     // 10 minutes
    15*60*1000,     // 15 minutes
    20*60*1000,     // 20 minutes
    30*60*1000,     // 30 minutes
    1*60*60*1000,   // 1 hour
    2*60*60*1000,   // 2 hours
    5*60*60*1000,   // 5 hours
    10*60*60*1000,  // 10 hours
    16*60*60*1000,  // 16 hours
    24*60*60*1000,  // 24 hours
    48*60*60*1000   // 48 hours
};

/** Waterfall time span changed. */
void DockFft::on_wfSpanComboBox_currentIndexChanged(int index)
{
    if (index < 0 || index > 12)
        return;

    emit wfSpanChanged(wf_span_table[index]);
}

/** Set waterfall time resolution. */
void DockFft::setWfResolution(quint64 msec_per_line)
{
    float res = 1.0e-3 * (float)msec_per_line;

    ui->wfResLabel->setText(QString("Res: %1 s").arg(res, 0, 'f', 2));
}

/**
 * @brief Split between waterfall and pandapter changed.
 * @param value The percentage of the waterfall.
 */
void DockFft::on_fftSplitSlider_valueChanged(int value)
{
    emit fftSplitChanged(value);
}

/** FFT filter gain changed. */
void DockFft::on_fftAvgSlider_valueChanged(int value)
{
    float avg = 1.0 - 1.0e-2 * ((float)value);

    emit fftAvgChanged(avg);
}

/** FFT zoom level changed */
void DockFft::on_fftZoomSlider_valueChanged(int level)
{
    ui->zoomLevelLabel->setText(QString("%1x").arg(level));
    emit fftZoomChanged((float)level);
}

/** Reference level changed */
void DockFft::on_reflevelSlider_valueChanged(int value)
{
    ui->reflevelLabel->setText(QString("%1 dB").arg(value));
    emit fftRangeChanged(value, ui->rangeSlider->value());
}

/** FFT plot range changed */
void DockFft::on_rangeSlider_valueChanged(int value)
{
    ui->rangeLabel->setText(QString("%1 dB").arg(value));
    emit fftRangeChanged(ui->reflevelSlider->value(), value);
}

void DockFft::on_resetButton_clicked(void)
{
    ui->zoomLevelLabel->setText(QString("1x"));
    ui->fftZoomSlider->setValue(0);
    emit resetFftZoom();
}

void DockFft::on_centerButton_clicked(void)
{
    emit gotoFftCenter();
}

void DockFft::on_demodButton_clicked(void)
{
    emit gotoDemodFreq();
}

/** FFT color has changed. */
void DockFft::on_colorPicker_colorChanged(const QColor &color)
{
    emit fftColorChanged(color);
}

/** FFT plot fill button toggled. */
void DockFft::on_fillButton_toggled(bool checked)
{
    emit fftFillToggled(checked);
}

/** peakHold button toggled */
void DockFft::on_peakHoldButton_toggled(bool checked)
{
    emit fftPeakHoldToggled(checked);
}

/** peakDetection button toggled */
void DockFft::on_peakDetectionButton_toggled(bool checked)
{
    emit peakDetectionToggled(checked);
}

/** Update RBW and FFT overlab labels */
void DockFft::updateInfoLabels(void)
{
    float   rate;
    float   size;
    float   rbw;
    float   ovr;
    float   sps;

    if (m_sample_rate == 0.f)
        return;

    rate = fftRate();
    size = fftSize();

    rbw = m_sample_rate / size;
    if (rbw < 1.e3f)
        ui->fftRbwLabel->setText(QString("RBW: %1 Hz").arg(rbw, 0, 'f', 1));
    else if (rbw < 1.e6f)
        ui->fftRbwLabel->setText(QString("RBW: %1 kHz").arg(1.e-3 * rbw, 0, 'f', 1));
    else
        ui->fftRbwLabel->setText(QString("RBW: %1 MHz").arg(1.e-6 * rbw, 0, 'f', 1));

    sps = size * rate;
    if (sps <= m_sample_rate)
        ovr = 0;
    else
        ovr = 100 * (sps / m_sample_rate - 1.f);
    ui->fftOvrLabel->setText(QString("Overlap: %1%").arg(ovr, 0, 'f', 0));
}
