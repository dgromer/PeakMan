/*
 * Copyright (C) 2014-2026 Daniel Gromer
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
#include <QMessageBox>
#include <QFile>
#include <QFileInfo>

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
    connect(ui->menuExportData, SIGNAL(triggered()), this, SLOT(exportData()));
    connect(ui->menuSettings, SIGNAL(triggered()), this, SLOT(openSettings()));
    connect(ui->menuAboutPeakMan, SIGNAL(triggered(bool)), this, SLOT(aboutPeakMan()));

    // Configure scroll bars for ECG plot
    ui->horizontalScrollBar->setRange(1000, 2000);
    ui->verticalSlider->setRange(10, 20000);
    ui->verticalSlider->setSliderPosition(5000);

    // Zoom buttons for ECG plot
    connect(ui->zoomEcgInButton, SIGNAL(clicked()), this, SLOT(zoomEcgIn()));
    connect(ui->zoomEcgOutButton, SIGNAL(clicked()), this, SLOT(zoomEcgOut()));

    // Zoom buttons for IBI plot
    connect(ui->zoomIbiInButton, SIGNAL(clicked()), this, SLOT(zoomIbiIn()));
    connect(ui->zoomIbiOutButton, SIGNAL(clicked()), this, SLOT(zoomIbiOut()));

    // Create connection between axes and scroll bars
    connect(ui->horizontalScrollBar, SIGNAL(valueChanged(int)), this, SLOT(horzScrollBarChanged(int)));
    connect(ui->ecgPlot->xAxis, SIGNAL(rangeChanged(QCPRange)), this, SLOT(xAxisChanged(QCPRange)));
    connect(ui->ecgPlot->yAxis, SIGNAL(rangeChanged(QCPRange)), this, SLOT(yAxisChanged(QCPRange)));
    connect(ui->verticalSlider, SIGNAL(valueChanged(int)), this, SLOT(vertSliderChanged(int)));

    // Create connection between IBI plot axes and scroll bar
    connect(ui->horizontalScrollBarIbiPlot, SIGNAL(valueChanged(int)), this, SLOT(horzScrollBarIbiChanged(int)));
    connect(ui->ibiPlot->xAxis, SIGNAL(rangeChanged(QCPRange)), this, SLOT(xAxisIbiChanged(QCPRange)));

    // Initialize axis range (and scroll bar positions via signals we just connected):
    ui->ecgPlot->xAxis->setRange(-.5, 20);
    ui->ecgPlot->yAxis->setRange(-1000, 3000);

    // Global threshold line interaction
    connect(ui->globalThresholdSpinBox, SIGNAL(valueChanged(int)), ui->ecgPlot, SLOT(updateGlobalThresholdLine(int)));
    connect(ui->showGlobalThresholdCheckBox, SIGNAL(toggled(bool)), ui->ecgPlot, SLOT(setGlobalThresholdLineVisible(bool)));
    connect(ui->ecgPlot, SIGNAL(globalThresholdChanged(int)), ui->globalThresholdSpinBox, SLOT(setValue(int)));

    // Peak detection
    connect(ui->detectPeaksButton, SIGNAL(clicked()), this, SLOT(peakDetection()));

    // Create keyboard shortcut for peak detection (Ctrl+D)
    peakDetectionShortcut = new QShortcut(QKeySequence(Qt::CTRL | Qt::Key_D), this);
    connect(peakDetectionShortcut, SIGNAL(activated()), this, SLOT(peakDetection()));
    peakDetectionShortcut->setEnabled(false);  // Initially disabled like the button

    // Create keyboard shortcuts for artifact navigation (Arrow keys)
    artifactNavigateLeftShortcut = new QShortcut(QKeySequence(Qt::Key_P), this);
    connect(artifactNavigateLeftShortcut, SIGNAL(activated()), this, SLOT(navigateToArtifactLeft()));
    artifactNavigateLeftShortcut->setEnabled(false);  // Initially disabled

    artifactNavigateRightShortcut = new QShortcut(QKeySequence(Qt::Key_N), this);
    connect(artifactNavigateRightShortcut, SIGNAL(activated()), this, SLOT(navigateToArtifactRight()));
    artifactNavigateRightShortcut->setEnabled(false);  // Initially disabled

    insertMissingPeaksShortcut = new QShortcut(QKeySequence(Qt::Key_I), this);
    connect(insertMissingPeaksShortcut, SIGNAL(activated()), this, SLOT(insertMissingPeaks()));
    insertMissingPeaksShortcut->setEnabled(false);  // Initially disabled

    // Update interbeat intervals automatically when peaks change
    connect(ui->ecgPlot, SIGNAL(peaksChanged()), this, SLOT(setupIbiPlot()));

    // Apply correction button and jump to position button
    connect(ui->insertMissingPeaksButton, SIGNAL(clicked()), this, SLOT(insertMissingPeaks()));
    connect(ui->ibiPlot, SIGNAL(ibiSelected(bool)), ui->jumpToSelectionButton, SLOT(setEnabled(bool)));
    connect(ui->ibiPlot, SIGNAL(ibiSelected(bool)), this, SLOT(toggleInsertMissingPeaksButton(bool)));
    connect(ui->jumpToSelectionButton, SIGNAL(clicked()), this, SLOT(jumpToSelection()));
    connect(ui->ibiPlot, SIGNAL(ibiSelectedDoubleClick()), this, SLOT(jumpToSelection()));
    connect(ui->resetIbiViewButton, SIGNAL(clicked()), ui->ibiPlot, SLOT(resetView()));

    // Artifact navigation
    connect(ui->ibiPlot, SIGNAL(artifactsChanged()), this, SLOT(updateArtifactSelectionUI()));
    connect(ui->ibiPlot, SIGNAL(ibiSelected(bool)), this, SLOT(updateArtifactSelectionUI()));
    connect(ui->artifactSelectLeftPushButton, SIGNAL(clicked()), this, SLOT(navigateToArtifactLeft()));
    connect(ui->artifactSelectRightPushButton, SIGNAL(clicked()), this, SLOT(navigateToArtifactRight()));
    ui->artifactSelectLineEdit->setReadOnly(true);

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
  // Dynamically adjust time format based on visible range
  if (range.size() < 5.0) {
    // When showing less than 5 seconds, include milliseconds
    ui->ecgPlot->setTimeFormat("hh:mm:ss.zzz");
  } else {
    // Default format for larger ranges
    ui->ecgPlot->setTimeFormat("hh:mm:ss");
  }

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

void MainWindow::horzScrollBarIbiChanged(int value)
{
    // Check if replot is needed to avoid double-replot during drag
    if (qAbs(ui->ibiPlot->xAxis->range().center() - value) > 0.01)
    {
        ui->ibiPlot->xAxis->setRange(value, ui->ibiPlot->xAxis->range().size(), Qt::AlignCenter);
        ui->ibiPlot->replot();
    }
}

void MainWindow::xAxisIbiChanged(QCPRange range)
{
    // Adjust position and size of scroll bar slider
    ui->horizontalScrollBarIbiPlot->setValue(qRound(range.center()));
    ui->horizontalScrollBarIbiPlot->setPageStep(qRound(range.size()));
}

void MainWindow::zoomEcgIn()
{
    // Use the same zoom factor as QCustomPlot's mouse wheel (0.85)
    const double zoomFactor = 0.85;

    // Get current range centers to zoom around the center of visible area
    double xCenter = ui->ecgPlot->xAxis->range().center();
    double yCenter = ui->ecgPlot->yAxis->range().center();

    // Scale both axes (factor < 1 makes range smaller = zoom in)
    ui->ecgPlot->xAxis->scaleRange(zoomFactor, xCenter);
    ui->ecgPlot->yAxis->scaleRange(zoomFactor, yCenter);

    // Replot to show changes
    ui->ecgPlot->replot();
}

void MainWindow::zoomEcgOut()
{
    // Use inverse of zoom factor for zoom out (1/0.85 ≈ 1.176)
    const double zoomFactor = 1.0 / 0.85;

    // Get current range centers to zoom around the center of visible area
    double xCenter = ui->ecgPlot->xAxis->range().center();
    double yCenter = ui->ecgPlot->yAxis->range().center();

    // Scale both axes (factor > 1 makes range larger = zoom out)
    ui->ecgPlot->xAxis->scaleRange(zoomFactor, xCenter);
    ui->ecgPlot->yAxis->scaleRange(zoomFactor, yCenter);

    // Replot to show changes
    ui->ecgPlot->replot();
}

void MainWindow::zoomIbiIn()
{
    // Use the same zoom factor as QCustomPlot's mouse wheel (0.85)
    const double zoomFactor = 0.85;

    // Get current range centers to zoom around the center of visible area
    double xCenter = ui->ibiPlot->xAxis->range().center();

    // Scale both axes (factor < 1 makes range smaller = zoom in)
    ui->ibiPlot->xAxis->scaleRange(zoomFactor, xCenter);

    // Replot to show changes
    ui->ibiPlot->replot();
}

void MainWindow::zoomIbiOut()
{
    // Use inverse of zoom factor for zoom out (1/0.85 ≈ 1.176)
    const double zoomFactor = 1.0 / 0.85;

    // Get current range centers to zoom around the center of visible area
    double xCenter = ui->ibiPlot->xAxis->range().center();

    // Scale both axes (factor > 1 makes range larger = zoom out)
    ui->ibiPlot->xAxis->scaleRange(zoomFactor, xCenter);

    // Replot to show changes
    ui->ibiPlot->replot();
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

    // Reset scrollbars
    ui->horizontalScrollBarIbiPlot->setRange(0, 100);  // Default range

    // Disable buttons
    ui->detectPeaksButton->setEnabled(false);
    peakDetectionShortcut->setEnabled(false);
    ui->menuCloseCurrentFile->setEnabled(false);
    ui->menuExportData->setEnabled(false);

    // Disable IBI-related buttons
    ui->resetIbiViewButton->setEnabled(false);
    ui->jumpToSelectionButton->setEnabled(false);
    ui->insertMissingPeaksButton->setEnabled(false);
    ui->insertMissingPeaksButton->setChecked(false);
    insertMissingPeaksShortcut->setEnabled(false);

    // Disable and clear artifact navigation UI
    ui->artifactSelectGroupBox->setEnabled(false);
    ui->artifactSelectLineEdit->clear();
    ui->artifactSelectLeftPushButton->setEnabled(false);
    ui->artifactSelectRightPushButton->setEnabled(false);
    artifactNavigateLeftShortcut->setEnabled(false);
    artifactNavigateRightShortcut->setEnabled(false);

    openFileName = "";
}

bool MainWindow::exportPeaksToFile(const QString &filename)
{
    // Open new file
    QFile outFile(filename);
    if (!outFile.open(QIODevice::WriteOnly | QIODevice::Text))
        return false;

    // Write peaks to file
    QTextStream out(&outFile);
    QLinkedList<QCPItemStraightLine*> peaks = ui->ecgPlot->getPeaks();
    QLinkedList<QCPItemStraightLine*>::iterator iter;

    for(iter = peaks.begin(); iter != peaks.end(); iter++)
    {
        out << (*iter)->point1->key() << "\n";
    }

    // Close
    outFile.flush();
    outFile.close();

    return true;
}

bool MainWindow::exportIbiToFile(const QString &filename, bool includeStartEnd)
{
    // Create new file and open it
    QFile outFile(filename);
    if (!outFile.open(QIODevice::WriteOnly | QIODevice::Text))
        return false;

    // Write IBI data to file
    QTextStream out(&outFile);

    if (includeStartEnd)
    {
        // Include signal start to first peak as interbeat interval
        out << ui->ecgPlot->getPeaks().first()->point1->key() * 1000 << "\n";
    }

    QVector<double> ibi_y = ui->ibiPlot->getIbi_y();

    for (int i = 0; i < ibi_y.size(); i++)
    {
        out << ibi_y[i] << "\n";
    }

    if (includeStartEnd)
    {
        // Include last peak to signal end as interbeat interval
        out << ui->ecgPlot->getEcg_x().last() * 1000 - ui->ecgPlot->getPeaks().last()->point1->key() * 1000 << "\n";
    }

    // Close
    outFile.flush();
    outFile.close();

    return true;
}

QString MainWindow::buildOverwriteMessage(bool peaksExists, bool ibiExists,
                                          const QString &peaksFile,
                                          const QString &ibiFile)
{
    QFileInfo peaksInfo(peaksFile);
    QFileInfo ibiInfo(ibiFile);

    QString message;

    if (peaksExists && ibiExists)
    {
        // Use HTML list formatting for clean rendering without wrapping
        message = "<p>The following files already exist:</p>"
                  "<ul>"
                  "<li>" + peaksInfo.fileName() + "</li>"
                  "<li>" + ibiInfo.fileName() + "</li>"
                  "</ul>"
                  "<p>Do you want to overwrite them?</p>";
    }
    else if (peaksExists)
    {
        message = "<p>The file <b>" + peaksInfo.fileName() +
                  "</b> already exists.</p>"
                  "<p>Do you want to overwrite it?</p>";
    }
    else // ibiExists
    {
        message = "<p>The file <b>" + ibiInfo.fileName() +
                  "</b> already exists.</p>"
                  "<p>Do you want to overwrite it?</p>";
    }

    return message;
}

void MainWindow::exportData()
{
    // Generate default base filename from current ECG file
    QFileInfo fn(openFileName);
    QString defaultBase = fn.canonicalPath() + QDir::separator() + fn.baseName();

    // Show single file dialog to choose base filename
    QString baseFileName = QFileDialog::getSaveFileName(this, "Choose base filename for data export", defaultBase);

    // Check if dialog was canceled
    if (baseFileName.isEmpty())
        return;

    // Generate output filenames
    QString peaksFileName = baseFileName + "_peaks.txt";
    QString ibiFileName = baseFileName + "_ibi.txt";

    // Check if files exist and confirm overwrite if necessary
    bool peaksExists = QFile::exists(peaksFileName);
    bool ibiExists = QFile::exists(ibiFileName);

    if (peaksExists || ibiExists)
    {
        QString message = buildOverwriteMessage(peaksExists, ibiExists,
                                                peaksFileName, ibiFileName);

        // Create message box with explicit configuration
        QMessageBox msgBox(QMessageBox::Question, "Confirm Overwrite", message,
                           QMessageBox::Yes | QMessageBox::No, this);
        msgBox.setTextFormat(Qt::RichText);  // Enable HTML rendering

        QMessageBox::StandardButton reply =
            (QMessageBox::StandardButton)msgBox.exec();

        if (reply == QMessageBox::No)
        {
            // User canceled, abort export
            ui->statusBar->showMessage("Export canceled", 2000);
            return;
        }
    }

    // Read includeStartEnd setting
    QSettings settings(QDir::currentPath() + "/peakman.ini", QSettings::IniFormat);
    bool includeStartEnd = settings.value("includeStartEnd", true).toBool();

    // Export both files
    bool peaksSuccess = exportPeaksToFile(peaksFileName);
    bool ibiSuccess = exportIbiToFile(ibiFileName, includeStartEnd);

    // Show appropriate status message
    if (peaksSuccess && ibiSuccess)
    {
        ui->statusBar->showMessage("Peak positions and interbeat intervals exported", 2000);
    }
    else if (peaksSuccess)
    {
        ui->statusBar->showMessage("Export partially successful: peaks exported, IBI failed", 3000);
    }
    else if (ibiSuccess)
    {
        ui->statusBar->showMessage("Export partially successful: IBI exported, peaks failed", 3000);
    }
    else
    {
        ui->statusBar->showMessage("Export failed: unable to write files", 3000);
    }
}

void MainWindow::openSettings()
{
    QSettings settings(QDir::currentPath() + "/peakman.ini", QSettings::IniFormat);

    SettingsDialog dialog(this);
    dialog.setIncludeStartEnd(settings.value("includeStartEnd", true).toBool());
    dialog.setArtifactThreshold(settings.value("artifactThreshold", 20.0).toDouble());

    if (dialog.exec() == QDialog::Accepted)
    {
        // Save the settings
        settings.setValue("includeStartEnd", dialog.getIncludeStartEnd());
        settings.setValue("artifactThreshold", dialog.getArtifactThreshold());

        // Apply artifact threshold to IBI plot
        ui->ibiPlot->setArtifactThreshold(dialog.getArtifactThreshold());
    }
}

void MainWindow::peakDetection()
{
    // Check if peaks already exist
    if (!ui->ecgPlot->getPeaks().isEmpty())
    {
        // Create confirmation dialog
        QMessageBox msgBox(QMessageBox::Question,
                          "Confirm Peak Detection",
                          "Existing peaks will be overwritten. Do you want to continue?",
                          QMessageBox::Yes | QMessageBox::No,
                          this);

        QMessageBox::StandardButton reply =
            (QMessageBox::StandardButton)msgBox.exec();

        if (reply == QMessageBox::No)
        {
            // User canceled, abort peak detection
            ui->statusBar->showMessage("Peak detection canceled", 2000);
            return;
        }
    }

    // Proceed with peak detection
    ui->ecgPlot->peakdet(ui->localThresholdSpinBox->value(), ui->globalThresholdSpinBox->value(), ui->minRRIntervallSpinBox->value());

    // Plot interbeat intervals
    setupIbiPlot();

    // Reset IBI plot ranges
    ui->ibiPlot->resetView();

    // Enable buttons
    ui->menuExportData->setEnabled(true);
    ui->resetIbiViewButton->setEnabled(true);
}

void MainWindow::setupIbiPlot()
{
    if (!ui->ecgPlot->getPeaks().isEmpty())
    {
        ui->ibiPlot->setup(ui->ecgPlot->getPeaks());

        // Set scrollbar range to match IBI data size
        int maxBeatIndex = ui->ibiPlot->getIbi_y().size();
        ui->horizontalScrollBarIbiPlot->setRange(0, maxBeatIndex);
    }
}

void MainWindow::jumpToSelection()
{
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

void MainWindow::toggleInsertMissingPeaksButton(bool enable)
{
    if (enable && ui->ibiPlot->getSelectionPosX() > 1)
    {
        double artifact_size = ui->ibiPlot->getSelectionPosY() / 1000;
        double ref = ui->ibiPlot->getReferenceInterval();

        // Calculate number of new intervals
        int n = qRound(artifact_size / ref);

        if (n > 1)
        {
            ui->insertMissingPeaksButton->setEnabled(true);
            insertMissingPeaksShortcut->setEnabled(true);
        }
        else
        {
            ui->insertMissingPeaksButton->setEnabled(false);
            insertMissingPeaksShortcut->setEnabled(false);
        }
    }
    else
    {
        ui->insertMissingPeaksButton->setEnabled(false);
        insertMissingPeaksShortcut->setEnabled(false);
    }
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
    // Plot interbeat intervals
    //ui->ibiPlot->setup(ui->ecgPlot->getPeaks());
    ui->ibiPlot->setup(ui->ecgPlot->getPeaks(), false);

    ui->jumpToSelectionButton->setEnabled(false);
    ui->insertMissingPeaksButton->setChecked(false);
    ui->insertMissingPeaksButton->setEnabled(false);
}

void MainWindow::updateArtifactSelectionUI()
{
    int artifactCount = ui->ibiPlot->getArtifactCount();

    // If no artifacts, disable everything
    if (artifactCount == 0)
    {
        ui->artifactSelectGroupBox->setEnabled(false);
        artifactNavigateLeftShortcut->setEnabled(false);
        artifactNavigateRightShortcut->setEnabled(false);
        ui->artifactSelectLineEdit->clear();
        return;
    }

    // Enable the group box
    ui->artifactSelectGroupBox->setEnabled(true);

    int currentArtifactNum = ui->ibiPlot->getCurrentArtifactNumber();

    if (currentArtifactNum != -1)
    {
        // An artifact is selected - show "x of y"
        ui->artifactSelectLineEdit->setText(QString("%1 of %2").arg(currentArtifactNum).arg(artifactCount));

        // Enable/disable navigation buttons based on position
        ui->artifactSelectLeftPushButton->setEnabled(currentArtifactNum > 1);
        artifactNavigateLeftShortcut->setEnabled(currentArtifactNum > 1);
        ui->artifactSelectRightPushButton->setEnabled(currentArtifactNum < artifactCount);
        artifactNavigateRightShortcut->setEnabled(currentArtifactNum < artifactCount);
    }
    else
    {
        // No artifact selected, or non-artifact IBI selected
        ui->artifactSelectLineEdit->clear();

        // Check if we can navigate from current position
        // Get current selection index, find if there are artifacts left/right
        double selectionX = ui->ibiPlot->getSelectionPosX();
        int selectionIndex = static_cast<int>(selectionX);

        QVector<int> artifactIndices = ui->ibiPlot->getArtifactIndices();
        bool hasArtifactLeft = false;
        bool hasArtifactRight = false;

        for (int idx : artifactIndices)
        {
            if (idx < selectionIndex) hasArtifactLeft = true;
            if (idx > selectionIndex) hasArtifactRight = true;
        }

        ui->artifactSelectLeftPushButton->setEnabled(hasArtifactLeft);
        artifactNavigateLeftShortcut->setEnabled(hasArtifactLeft);
        ui->artifactSelectRightPushButton->setEnabled(hasArtifactRight);
        artifactNavigateRightShortcut->setEnabled(hasArtifactRight);
    }
}

void MainWindow::navigateToArtifactLeft()
{
    int currentArtifactNum = ui->ibiPlot->getCurrentArtifactNumber();

    if (currentArtifactNum > 1)
    {
        // Navigate to previous artifact in list
        ui->ibiPlot->selectArtifact(currentArtifactNum - 2); // Convert to 0-based
        jumpToSelection();
    }
    else
    {
        // Find nearest artifact to the left of current selection
        double selectionX = ui->ibiPlot->getSelectionPosX();
        int selectionIndex = static_cast<int>(selectionX);
        int artifactListIndex = ui->ibiPlot->findNearestArtifactLeft(selectionIndex);

        if (artifactListIndex != -1)
        {
            ui->ibiPlot->selectArtifact(artifactListIndex);
            jumpToSelection();
        }
    }
}

void MainWindow::navigateToArtifactRight()
{
    int currentArtifactNum = ui->ibiPlot->getCurrentArtifactNumber();
    int artifactCount = ui->ibiPlot->getArtifactCount();

    if (currentArtifactNum != -1 && currentArtifactNum < artifactCount)
    {
        // Navigate to next artifact in list
        ui->ibiPlot->selectArtifact(currentArtifactNum); // currentArtifactNum is 1-based, so this gives next (0-based)
        jumpToSelection();
    }
    else
    {
        // Find nearest artifact to the right of current selection
        double selectionX = ui->ibiPlot->getSelectionPosX();
        int selectionIndex = static_cast<int>(selectionX);
        int artifactListIndex = ui->ibiPlot->findNearestArtifactRight(selectionIndex);

        if (artifactListIndex != -1)
        {
            ui->ibiPlot->selectArtifact(artifactListIndex);
            jumpToSelection();
        }
    }
}

void MainWindow::aboutPeakMan()
{
    QMessageBox::about(this, "About PeakMan",
                       "<p><b>PeakMan</b><br>Version 0.5.0</p>"
                       "<p>Copyright (C) 2014-2026 Daniel Gromer</p>"
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
    peakDetectionShortcut->setEnabled(true);
    ui->menuCloseCurrentFile->setEnabled(true);

    ui->statusBar->showMessage("File opened", 2000);
}

void MainWindow::openPeaksFile()
{
    if (ui->ecgPlot->getEcg_y().isEmpty())
    {
        // TODO: error message, load ecg data first
    }

    // If there are already peaks plotted, delete these
    if (!ui->ecgPlot->getPeaks().isEmpty())
    {
        ui->ecgPlot->clearPeaks();
    }

    ui->statusBar->showMessage("Opening file ...");

    QFile file(openFileName);

    // Open file
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) return;

    QTextStream in(&file);

    // Store peak positions in vector
    QVector<double> peaks_x;

    // Read file line by line
    while (!in.atEnd())
    {
        peaks_x << in.readLine().toDouble();
    }

    file.close();

    // Insert peaks in ecgplot
    ui->ecgPlot->insertPeaksFromVector(peaks_x);

    // Plot interbeat intervals
    setupIbiPlot();

    // Reset IBI plot ranges
    ui->ibiPlot->resetView();

    // Enable buttons
    ui->menuExportData->setEnabled(true);
    ui->resetIbiViewButton->setEnabled(true);

    ui->statusBar->showMessage("File opened", 2000);
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

    // Save artifact detection threshold
    settings.setValue("artifactThreshold", ui->ibiPlot->getArtifactThreshold() * 100.0);

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

    // Load artifact detection threshold and apply to IBI plot
    double artifactThresh = settings.value("artifactThreshold", 20.0).toDouble();
    ui->ibiPlot->setArtifactThreshold(artifactThresh);

    settings.endGroup();
}
