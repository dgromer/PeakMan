/*
 * Copyright (C) 2014-2015 Daniel Gromer
 *
 * This file is part of PeakMan.
 *
 * PeakMan is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.

 * PeakMan is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with PeakMan.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QDebug>

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    // Allow drag and drop from windows explorer
    setAcceptDrops(true);

    // Create connections for menu items
    connect(ui->menuOpenFile, SIGNAL(triggered()), this, SLOT(getFileName()));
    connect(ui->menuCloseCurrentFile, SIGNAL(triggered()), this, SLOT(closeCurrentFile()));
    connect(ui->menuSaveInterbeatIntervals, SIGNAL(triggered()), this, SLOT(saveInterbeatIntervals()));
    connect(ui->menuAboutPeakMan, SIGNAL(triggered(bool)), this, SLOT(aboutPeakMan()));

    // Configure scroll bars
    ui->horizontalScrollBar->setRange(1000, 2000);
    ui->verticalSlider->setRange(10, 20000);
    ui->verticalSlider->setSliderPosition(5000);

    // Create connection between axes and scroll bars
    connect(ui->horizontalScrollBar, SIGNAL(valueChanged(int)), this, SLOT(horzScrollBarChanged(int)));
    connect(ui->ecgPlot->xAxis, SIGNAL(rangeChanged(QCPRange)), this, SLOT(xAxisChanged(QCPRange)));
    connect(ui->ecgPlot->yAxis, SIGNAL(rangeChanged(QCPRange)), this, SLOT(yAxisChanged(QCPRange)));
    connect(ui->verticalSlider, SIGNAL(valueChanged(int)), this, SLOT(vertSliderChanged(int)));

    // Initialize axis range (and scroll bar positions via signals we just connected):
    ui->ecgPlot->xAxis->setRange(-.5, 20);
    ui->ecgPlot->yAxis->setRange(-1000, 3000);

    // Global threshold line interaction
    connect(ui->globalThresholdSpinBox, SIGNAL(valueChanged(int)), ui->ecgPlot, SLOT(updateGlobalThresholdLine(int)));
    connect(ui->showGlobalThresholdCheckBox, SIGNAL(toggled(bool)), ui->ecgPlot, SLOT(setGlobalThresholdLineVisible(bool)));
    connect(ui->ecgPlot, SIGNAL(globalThresholdChanged(int)), ui->globalThresholdSpinBox, SLOT(setValue(int)));

    // Peak detection
    connect(ui->detectPeaksButton, SIGNAL(clicked()), this, SLOT(peakDetection()));

    // Update interbeat intervals
    connect(ui->updateIbiButton, SIGNAL(clicked()), this, SLOT(setupIbiPlot()));
    connect(ui->ibiPlot, SIGNAL(setupHistPlot(QVector<double>, double)), ui->histPlot, SLOT(setup(QVector<double>, double)));

    // Apply correction button and jump to position button
    connect(ui->artifactDetectionPushButton, SIGNAL(clicked()), ui->ibiPlot, SLOT(artifactDetection()));
    connect(ui->insertMissingPeaksButton, SIGNAL(clicked()), this, SLOT(insertMissingPeaks()));
    connect(ui->ibiPlot, SIGNAL(ibiSelected(bool)), ui->jumpToSelectionButton, SLOT(setEnabled(bool)));
    connect(ui->jumpToSelectionButton, SIGNAL(clicked()), this, SLOT(jumpToSelection()));

    // Initialize sample rate label
    ui->ecgPlot->setSampleRate(0);
    sampleRateLabel = new QLabel(this);
    ui->statusBar->addPermanentWidget(sampleRateLabel);
    updateSampleRateLabel();

    loadSettings();
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::horzScrollBarChanged(int value)
{
    if (qAbs(ui->ecgPlot->xAxis->range().center()-value/100.0) > 0.01) // if user is dragging plot, we don't want to replot twice
    {
        ui->ecgPlot->xAxis->setRange(value/100.0, ui->ecgPlot->xAxis->range().size(), Qt::AlignCenter);
        ui->ecgPlot->replot();
    }
}

void MainWindow::xAxisChanged(QCPRange range)
{
  ui->horizontalScrollBar->setValue(qRound(range.center()*100.0)); // adjust position of scroll bar slider
  ui->horizontalScrollBar->setPageStep(qRound(range.size()*100.0)); // adjust size of scroll bar slider
}

void MainWindow::yAxisChanged(QCPRange range)
{
  // adjust position of slider
  ui->verticalSlider->setSliderPosition(qRound(range.size()));
}

void MainWindow::vertSliderChanged(int value)
{
    ui->ecgPlot->yAxis->setRange(ui->ecgPlot->yAxis->range().center(), value, Qt::AlignCenter);
    ui->ecgPlot->replot();
}

void MainWindow::getFileName()
{
    // Get filename via input dialog
    openFileName = QFileDialog::getOpenFileName(this, tr("Open File"), QDir::currentPath(), tr("Text Files (*.txt)"));

    if (openFileName != "")
    {
        execOpenFileDialog();
    }
}

void MainWindow::closeCurrentFile()
{
    // Clear plots
    ui->ecgPlot->clear();
    ui->ibiPlot->clear();
    ui->histPlot->clear();

    // Disable buttons
    ui->detectPeaksButton->setEnabled(false);
    ui->menuCloseCurrentFile->setEnabled(false);
    ui->menuSavePeakPositions->setEnabled(false);
    ui->menuSaveInterbeatIntervals->setEnabled(false);

    openFileName = "";
}

void MainWindow::saveInterbeatIntervals()
{
    // New filename prototype
    QFileInfo fn(openFileName);
    QString newFn = fn.canonicalPath() + QDir::separator() + fn.baseName() + "_ibi.txt";

    // Get new filename
    QString outFileName = QFileDialog::getSaveFileName(this, "Save As", newFn);

    // Check if dialog was canceled
    if (outFileName == "") return;

    // Open new file
    QFile outFile(outFileName);
    if (!outFile.open(QIODevice::WriteOnly | QIODevice::Text)) return;

    // Paste text
    QTextStream out(&outFile);

    QVector<double> ibi_y = ui->ibiPlot->getIbi_y();

    for (int i = 0; i < ibi_y.size(); i++)
    {
        out << ibi_y[i] << "\n";
    }

    // Close
    outFile.flush();
    outFile.close();

    ui->statusBar->showMessage("Interbeat intervals exported", 2000);
}

void MainWindow::peakDetection()
{
    ui->ecgPlot->peakdet(ui->localThresholdSpinBox->value(), ui->globalThresholdSpinBox->value(), ui->minRRIntervallSpinBox->value());

    // Plot interbeat intervals and histogram
    setupIbiPlot();

    // Enable buttons
    ui->menuSaveInterbeatIntervals->setEnabled(true);
    ui->updateIbiButton->setEnabled(true);
    ui->artifactDetectionPushButton->setEnabled(true);
    ui->insertMissingPeaksButton->setEnabled(true);
}

void MainWindow::setupIbiPlot()
{
    if (!ui->ecgPlot->getPeaks().isEmpty())
    {
        ui->ibiPlot->setup(ui->ecgPlot->getPeaks());
    }
}

void MainWindow::jumpToSelection()
{
    ui->tabWidget->setCurrentIndex(0);

    double x = ui->ibiPlot->getSelectionTimePoint();

    // Add position of first peak to x
    x += ui->ecgPlot->getPeaks().first()->point1->key();

    // Set view port to selected peak
    ui->ecgPlot->xAxis->setRange(x, ui->ecgPlot->xAxis->range().size(), Qt::AlignCenter);

    // x-value for highlight rect
    double key = x - ui->ibiPlot->getSelectionPosY() / 1000 / 2;

    // Add highlight line
    ui->ecgPlot->showIbiHighlightRect(key, ui->ibiPlot->getSelectionPosY() / 1000);
}

void MainWindow::insertMissingPeaks()
{
    double artifact_size = ui->ibiPlot->getSelectionPosY() / 1000;
    double artifact_pos = ui->ibiPlot->getSelectionTimePoint() + ui->ecgPlot->getTimeBeforeFirstPeak() - artifact_size;

    // Get size of the interbeat interval before selection as reference interval
    double ref = ui->ibiPlot->getReferenceInterval();

    // Calculate number of new intervals
    int n = qRound(artifact_size / ref);

    // Get size of new beats
    double new_size = artifact_size / n;

    // TODO: evtl. hier insertPeakAtTimePoints definieren
    for (int i = 1; i < n; i++)
    {
        ui->ecgPlot->insertPeakAtTimePoint(artifact_pos + (double)i * new_size);
    }

    ui->ecgPlot->replot();

    // TODO: don't reset viewport here
    // Plot interbeat intervals and histogram
    //ui->ibiPlot->setup(ui->ecgPlot->getPeaks());
    ui->ibiPlot->setup(ui->ecgPlot->getPeaks(), false);
}

void MainWindow::aboutPeakMan()
{
    QMessageBox::about(this, "About PeakMan",
                       "<p><b>PeakMan</b><br>Version 0.3.0</p>"
                       "<p>Copyright (C) 2014-2015 Daniel Gromer</p>"
                       "<p><a href='https://github.com/dgromer/PeakMan'>https://github.com/dgromer/PeakMan</a></p>"
                       "<p>This program is licensed to you under the terms of version 3 of the GNU <a href='http://www.gnu.org/licenses/gpl-3.0.txt'>General Public License</a>.");
}

void MainWindow::execOpenFileDialog()
{
    OpenFileDialog dialog(this, openFileName, ui->ecgPlot->getSampleRate());
    dialog.exec();

    if (dialog.result() == QDialog::Accepted)
    {
        //sampleRate = dialog.getSampleRate();
        ui->ecgPlot->setSampleRate(dialog.getSampleRate());
        updateSampleRateLabel();

        if (dialog.getRadioButtonPushed() == "ecgsignal")
        {
            openEcgFile();
        }
        else if (dialog.getRadioButtonPushed() == "peaks")
        {
            openPeaksFile();
        }
        else if (dialog.getRadioButtonPushed() == "ibi")
        {
            openIbiFile();
        }
    }
}

void MainWindow::openEcgFile()
{
    // If there's already an open file, close it before opening the new one
    if (!ui->ecgPlot->getEcg_y().isEmpty()) closeCurrentFile();

    ui->statusBar->showMessage("Opening file ...");

    QFile file(openFileName);

    // Open file
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) return;

    QTextStream in(&file);

    // Store ecg signal here
    QVector<double> ecg_y;

    // Read file line by line
    while (!in.atEnd())
    {
        ecg_y << in.readLine().toDouble();
    }

    file.close();

    // x-axis vector for ecg signal with time points in seconds
    QVector<double> ecg_x;

    // Create a vector with time for x-axis
    for (int i = 0; i < ecg_y.size(); i++)
    {
        ecg_x << (double) i / ui->ecgPlot->getSampleRate();
    }

    // Plot ecg signal
    ui->ecgPlot->plot(ecg_x, ecg_y);

    // Adjust size of horizontal scrollbar
    ui->horizontalScrollBar->setRange(0, ecg_x.last() * 100);

    // Enable menu entries
    ui->detectPeaksButton->setEnabled(true);
    ui->menuCloseCurrentFile->setEnabled(true);

    ui->statusBar->showMessage("File opened", 2000);
}

void MainWindow::openPeaksFile()
{

}

void MainWindow::openIbiFile()
{

}

void MainWindow::dragEnterEvent(QDragEnterEvent *event)
{
    event->accept();
}

void MainWindow::dropEvent(QDropEvent *event)
{
    // Read paths from drop event
    QList<QUrl> urls = event->mimeData()->urls();

    // Error if more than one file is dropped
    if (urls.length() > 1)
    {
        QMessageBox::information(this, "Error", "Only one file allowed");
        return;
    }

    // Only open one file, so use first path only
    QFileInfo in(urls.first().toLocalFile());

    if (in.suffix() != "txt")
    {
        QMessageBox::information(this, "Error", "Only text files allowed");
        return;
    }

    openFileName = in.absoluteFilePath();

    execOpenFileDialog();
}

void MainWindow::updateSampleRateLabel()
{
    sampleRateLabel->setText("   Sample Rate: " + QString::number(ui->ecgPlot->getSampleRate()) + " Hz ");
}

void MainWindow::closeEvent(QCloseEvent *event)
{
    saveSettings();
    event->accept();
}

void MainWindow::saveSettings()
{
    QSettings settings(QDir::currentPath() + "/peakman.ini", QSettings::IniFormat);

    settings.beginGroup("MainWindow");

    // Save position and size of window
    settings.setValue("size", size());
    settings.setValue("pos", pos());

    // Save sample rate for ecg data
    settings.setValue("samplerate", ui->ecgPlot->getSampleRate());

    // Save settings for peak detection algorithm
    settings.setValue("delta", ui->localThresholdSpinBox->value());
    settings.setValue("threshold", ui->globalThresholdSpinBox->value());
    settings.setValue("minrrintervall", ui->minRRIntervallSpinBox->value());

    // Save whether to show global threshold
    settings.setValue("showthreshold", ui->showGlobalThresholdCheckBox->isChecked());

    settings.endGroup();
}

void MainWindow::loadSettings()
{
    // Coordinates of screen center for default window positioning
    QRect desktopRect = QApplication::desktop()->availableGeometry(this);
    QPoint center = desktopRect.center();
    center.setX(center.x() - width() * 0.5);
    center.setY(center.y() - height() * 0.5);

    QSettings settings(QDir::currentPath() + "/peakman.ini", QSettings::IniFormat);

    settings.beginGroup("MainWindow");

    // Set position and size of window
    resize(settings.value("size", QSize(1081, 693)).toSize());
    move(settings.value("pos", center).toPoint());

    // Set sample rate for ecg data
    ui->ecgPlot->setSampleRate(settings.value("samplerate", "").toInt());
    updateSampleRateLabel();

    // Set settings for peak detection algorithm
    ui->localThresholdSpinBox->setValue(settings.value("delta", "200").toInt());
    ui->globalThresholdSpinBox->setValue(settings.value("threshold", "500").toInt());
    ui->minRRIntervallSpinBox->setValue(settings.value("minrrinterval", "270").toInt());

    // Set show global threshold
    ui->ecgPlot->setGlobalThresholdLineVisible(settings.value("showthreshold", true).toBool());
    ui->showGlobalThresholdCheckBox->setChecked(settings.value("showthreshold", true).toBool());

    settings.endGroup();
}
