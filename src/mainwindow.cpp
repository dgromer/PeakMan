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
    connect(ui->detectPeaksButton, SIGNAL(clicked()), this, SLOT(peakdet()));

    // Peak insert and removal
    connect(ui->ecgPlot, SIGNAL(insertPeakAtPos(QPoint)), this, SLOT(insertPeakAtPos(QPoint)));
    connect(ui->ecgPlot, SIGNAL(deletePeak(QCPAbstractItem*)), this, SLOT(deletePeak(QCPAbstractItem*)));
    connect(ui->ecgPlot, SIGNAL(deletePeaks(QList<QCPAbstractItem*>)), this, SLOT(deletePeaks(QList<QCPAbstractItem*>)));

    // Update interbeat intervals
    connect(ui->updateIbiButton, SIGNAL(clicked()), this, SLOT(setupIbiPlot()));

    // Apply correction button and jump to position button
    connect(ui->artifactDetectionPushButton, SIGNAL(clicked()), this, SLOT(artifactDetection()));
    connect(ui->insertMissingPeaksButton, SIGNAL(clicked()), this, SLOT(insertMissingPeaks()));
    connect(ui->ibiPlot, SIGNAL(ibiSelected(bool)), ui->jumpToSelectionButton, SLOT(setEnabled(bool)));
    connect(ui->jumpToSelectionButton, SIGNAL(clicked()), this, SLOT(jumpToSelection()));

    // Initialize sample rate label
    sampleRate = 0;
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
    // Clear vectors holding ecg data
    ecg_x.clear();
    ecg_y.clear();

    // Remove ecg graph from plot
    ui->ecgPlot->removeGraph(0);

    // Remove peaks from plot
    foreach (QCPLayerable *l, ui->ecgPlot->layer("peaks")->children())
    {
        ui->ecgPlot->removeItem(qobject_cast<QCPAbstractItem*>(l));
    }

    peaks.clear();

    ui->ecgPlot->replot();

    if (!ibi_y.isEmpty())
    {
        ibi_x.clear();
        ibi_y.clear();
        ui->ibiPlot->unsetTracer();
        ui->ibiPlot->removeGraph(0);

        if (!artifacts_y.isEmpty())
        {
            artifacts_x.clear();
            artifacts_y.clear();
            ui->ibiPlot->removeGraph(1);
        }

        ui->ibiPlot->replot();

        hist_x.clear();
        hist_y.clear();
        ui->histPlot->removePlottable(0);
        ui->histPlot->replot();
    }

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

    for (int i = 0; i < ibi_y.size(); i++)
    {
        out << ibi_y[i] << "\n";
    }

    // Close
    outFile.flush();
    outFile.close();

    ui->statusBar->showMessage("Interbeat intervals exported", 2000);
}

void MainWindow::peakdet()
{
    // Check if there are already peaks
    if (!peaks.isEmpty())
    {
        // Remove peaks from plot
        foreach (QCPLayerable *l, ui->ecgPlot->layer("peaks")->children())
        {
            ui->ecgPlot->removeItem(qobject_cast<QCPAbstractItem*>(l));
        }

        peaks.clear();
    }

    // Peak detection algorithm starts here
    double mn = ecg_y.first(), mx = ecg_y.first(), mxpos = ecg_x.first(), curr;
    double delta = (double)ui->localThresholdSpinBox->value();
    double threshold = (double)ui->globalThresholdSpinBox->value();
    double minrrinterval = (double)ui->minRRIntervallSpinBox->value() / 1000;
    bool lookformax = true;

    for (int i = 0; i < ecg_y.size(); i++)
    {
        curr = ecg_y[i];

        if (curr > mx)
        {
            mx = curr;
            mxpos = ecg_x[i];
        }

        if (curr < mn)
        {
            mn = curr;
        }

        if (lookformax)
        {
            if (curr < mx - delta)
            {
                if(peaks.size() > 1)
                {
                    // Apply threshold and minimal RR interval
                    if (mxpos - peaks.last()->point1->key() > minrrinterval && (mx > threshold))
                    {
                        peaks.append(insertNewPeak(mxpos));
                    }
                }
                // Apply threshold only
                else if (mx > threshold)
                {
                    peaks.append(insertNewPeak(mxpos));
                }

                mn = curr;
                lookformax = false;
            }
        }
        else
        {
            if (curr > mn + delta)
            {
                mx = curr;
                mxpos = ecg_y[i];
                lookformax = true;
            }
        }
    }

    ui->ecgPlot->replot();

    setupIbiPlot();

    ui->menuSaveInterbeatIntervals->setEnabled(true);
    ui->updateIbiButton->setEnabled(true);
    ui->artifactDetectionPushButton->setEnabled(true);
    ui->insertMissingPeaksButton->setEnabled(true);
}

void MainWindow::insertPeakAtPos(QPoint position)
{
    double pos_x = ui->ecgPlot->xAxis->pixelToCoord((double) position.x());

    // Cancel if click was outside of graph
    if (pos_x < ecg_x.first() || pos_x > ecg_x.last()) return;

    int newpos = pos_x * sampleRate;

    // Search for maximum around clicked position
    for (int i = (int) ((pos_x - .1) * sampleRate); i < (int) ((pos_x + .1) * sampleRate); i++)
    {
        if (ecg_y[i] > ecg_y[newpos]) newpos = i;
    }

    double insert = (double)newpos / (double)sampleRate;

    QLinkedList<QCPItemStraightLine*>::iterator iter;

    // Search through peaks list for insertion point
    for(iter = peaks.begin(); iter != peaks.end(); iter++)
    {
        if ((*iter)->point1->key() > insert)
        {
            QCPItemStraightLine* newPeak = insertNewPeak(insert);
            peaks.insert(iter, newPeak);

            return;
        }
    }
}

void MainWindow::insertPeakAtPoint(double position)
{
    // Set x position to fit with samplerate
    double insert = qRound(position * (double)sampleRate) / (double)sampleRate;

    QLinkedList<QCPItemStraightLine*>::iterator iter;

    // Search through peaks list for insertion point
    for(iter = peaks.begin(); iter != peaks.end(); iter++)
    {
        if ((*iter)->point1->key() > insert)
        {
            QCPItemStraightLine* newPeak = insertNewPeak(insert);
            peaks.insert(iter, newPeak);

            return;
        }
    }
}

void MainWindow::deletePeak(QCPAbstractItem *peak)
{
    peaks.removeOne(qobject_cast<QCPItemStraightLine*>(peak));
    ui->ecgPlot->removeItem(peak);
}

void MainWindow::deletePeaks(QList<QCPAbstractItem*> peaksToDelete)
{
    foreach(QCPAbstractItem* peak, peaksToDelete)
    {
        peaks.removeOne(qobject_cast<QCPItemStraightLine*>(peak));
        ui->ecgPlot->removeItem(peak);
    }

    ui->ecgPlot->replot();
}

void MainWindow::setupIbiPlot()
{
    if (peaks.isEmpty()) return;

    if (!ibi_y.empty())
    {
        ibi_x.clear();
        ibi_y.clear();

        if (!artifacts_y.empty())
        {
            artifacts_x.clear();
            artifacts_y.clear();
            ui->ibiPlot->removeGraph(1);
        }
    }

    double lastPeakPosition = peaks.first()->point1->key() * 1000;
    int i = 0;
    double maxIbi = 0;

    // Compute interbeat intervals in msec
    foreach (QCPItemStraightLine *peak, peaks)
    {
        ibi_x << (double) i++;
        ibi_y << peak->point1->key() * 1000 - lastPeakPosition;

        maxIbi = qMax(maxIbi, ibi_y.last());

        lastPeakPosition += ibi_y.last();
    }

    // First element is zero
    ibi_x.removeFirst();
    ibi_y.removeFirst();

    ui->ibiPlot->addGraph();
    ui->ibiPlot->graph(0)->setData(ibi_x, ibi_y);
    ui->ibiPlot->graph(0)->setPen(QColor(77, 77, 76));
    ui->ibiPlot->xAxis->setRange(-5, ibi_x.size() + 5);
    ui->ibiPlot->yAxis->setRange(0, maxIbi + 200);

    ui->ibiPlot->replot();

    // Enable selection in ibi plot
    ui->ibiPlot->setTracer();

    setupHistPlot(maxIbi);
}

void MainWindow::jumpToSelection()
{
    ui->tabWidget->setCurrentIndex(0);

    double x = 0;

    // Sum interbeat intervals up to selection to get x-axis position in ecgPlot
    for (int i  = 0; i < (int)ui->ibiPlot->getSelectionGraphKey(); i++)
    {
        x += ibi_y[i] / 1000;
    }

    // Add position of first peak to x
    x += peaks.first()->point1->key();

    // Set view port to selected peak
    ui->ecgPlot->xAxis->setRange(x, ui->ecgPlot->xAxis->range().size(), Qt::AlignCenter);

    // x-value for highlight rect
    double key = x - ui->ibiPlot->getSelectionValue() / 1000 / 2;

    // Add highlight line
    ui->ecgPlot->showIbiHighlightRect(key, ui->ibiPlot->getSelectionValue() / 1000);

    ui->ecgPlot->replot();
}

void MainWindow::artifactDetection()
{
    if (!artifacts_y.isEmpty())
    {
        artifacts_x.clear();
        artifacts_y.clear();
        ui->ibiPlot->removeGraph(1);
    }

    for (int i = 1; i < ibi_y.size(); i++)
    {
        if (qAbs(ibi_y[i] - ibi_y[i - 1]) > .2 * ibi_y[i - 1])
        {
            artifacts_x << ibi_x[i];
            artifacts_y << ibi_y[i];
        }
    }

    ui->ibiPlot->addGraph();
    ui->ibiPlot->graph(1)->setData(artifacts_x, artifacts_y);
    ui->ibiPlot->graph(1)->setPen(Qt::NoPen);
    ui->ibiPlot->graph(1)->setScatterStyle(QCPScatterStyle(QCPScatterStyle::ssCircle, QColor(200, 40, 41), QColor(200, 40, 41), 7));
    ui->ibiPlot->replot();
}

void MainWindow::insertMissingPeaks()
{
    double x = 0;
    int i, m = (int)ui->ibiPlot->getSelectionGraphKey() - 1;

    // Sum interbeat intervals up to selection to get x-axis position in ecgPlot
    for (i = 0; i < m; i++)
    {
        x += ibi_y[i] / 1000;
    }

    // Add position of first peak to x
    x += peaks.first()->point1->key();

    // Get size of the interbeat interval before selection as reference interval
    double ref = ibi_y[i - 1] / 1000;

    // Get size of selected artifact
    double total = ui->ibiPlot->getSelectionValue() / 1000;

    // Calculate number of new intervals
    int n = qRound(total / ref);

    // Get size of new beats
    double new_size = total / n;

    for (int j = 1; j < n; j++)
    {
        insertPeakAtPoint(x + (double)j * new_size);
    }

    ui->ecgPlot->replot();
}

void MainWindow::execOpenFileDialog()
{
    OpenFileDialog dialog(this, openFileName, sampleRate);
    dialog.exec();

    if (dialog.result() == QDialog::Accepted)
    {
        sampleRate = dialog.getSampleRate();
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
    if (!ecg_y.isEmpty()) closeCurrentFile();

    ui->statusBar->showMessage("Opening file ...");

    QFile file(openFileName);

    // Open file
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
    {
        return;
    }

    QTextStream in(&file);

    // Read file line by line
    while (!in.atEnd())
    {
        ecg_y << in.readLine().toDouble();
    }

    file.close();

    // Create a vector with time for x-axis
    for(int i = 0; i < ecg_y.size(); i++)
    {
        ecg_x << (double) i / sampleRate;
    }

    // Plot data
    ui->ecgPlot->addGraph();
    ui->ecgPlot->graph(0)->setPen(QPen(QColor(77, 77, 76)));
    ui->ecgPlot->graph(0)->setData(ecg_x, ecg_y);
    ui->ecgPlot->graph(0)->setLayer("data");
    ui->ecgPlot->replot();

    // Adjust size of horizontal scrollbar
    ui->horizontalScrollBar->setRange(0, ecg_x.last() * 100);

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
    sampleRateLabel->setText("   Sample Rate: " + QString::number(sampleRate) + " Hz ");
}

QCPItemStraightLine* MainWindow::insertNewPeak(double position)
{
    QCPItemStraightLine* newPeak = new QCPItemStraightLine(ui->ecgPlot);

    ui->ecgPlot->addItem(newPeak);
    newPeak->setLayer("peaks");

    // Set coordinates
    newPeak->point1->setCoords(position, 1);
    newPeak->point2->setCoords(position, 0);

    newPeak->setPen(QPen(QBrush(QColor(66, 113, 174, 130)), 5));
    newPeak->setSelectedPen(QPen(QBrush(QColor(234, 183, 0, 200)), 3));

    return newPeak;
}

void MainWindow::setupHistPlot(double maxIbiValue)
{
    hist_y = QVector<double>(qFloor(maxIbiValue / 10) + 1);
    hist_x = QVector<double>(hist_y.size());

    double maxHistValue = 0;

    for (int i = 0; i < ibi_y.size(); i++)
    {
        hist_y[qFloor(ibi_y[i] / 10)]++;
    }

    for (int i = 0; i < hist_x.size(); i++)
    {
        hist_x[i] = i * 10 + 5;
        maxHistValue = qMax(maxHistValue, hist_y[i]);
    }

    if (ui->histPlot->plottableCount() > 0)
    {
        ui->histPlot->removePlottable(0);
    }

    QCPBars *bars = new QCPBars(ui->histPlot->xAxis, ui->histPlot->yAxis);
    ui->histPlot->addPlottable(bars);
    bars->setWidth(10);
    bars->setData(hist_x, hist_y);
    bars->setPen(QPen(Qt::black));
    bars->setBrush(QColor(77, 77, 76));

    ui->histPlot->xAxis->setRange(0, maxIbiValue + 20);
    ui->histPlot->yAxis->setRange(0, maxHistValue + 5);

    ui->histPlot->replot();
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
    settings.setValue("samplerate", sampleRate);

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
    sampleRate = settings.value("samplerate", "").toInt();
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
