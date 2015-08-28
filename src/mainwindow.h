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

#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include "qcustomplot.h"
#include "ecgplot.h"
#include "openfiledialog.h"

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();

private slots:
    // Functions for plot interaction
    void horzScrollBarChanged(int value);
    void xAxisChanged(QCPRange range);
    void yAxisChanged(QCPRange range);
    void vertSliderChanged(int value);

    void getFileName();
    void closeCurrentFile();
    void saveInterbeatIntervals();

    void peakdet(); // The peak detection algorithm
    void insertPeakAtPos(QPoint position); // Used for inserting a peak at clicked position
    void insertPeakAtPoint(double position); // Used for inserting a peak at a specific time
    void deletePeak(QCPAbstractItem *peak); // Delete a single peak
    void deletePeaks(QList<QCPAbstractItem*> peaksToDelete); // Delete a list of peaks

    void setupIbiPlot();
    void jumpToSelection(); // Highlight a selected interbeat interval in ecg view
    void artifactDetection(); // Search for artifacts in sequence of interbeat intervals
    void insertMissingPeaks(); // Subdivides an interbeat interval into shorter intervals

    void aboutPeakMan();

private:
    Ui::MainWindow *ui;

    QString openFileName;
    void execOpenFileDialog(); // Display a dialog for file opening
    void openEcgFile(); // Read a text file with ecg data
    void openPeaksFile();
    void openIbiFile();
    void dragEnterEvent(QDragEnterEvent *event); // Allows to drag something into the application
    void dropEvent(QDropEvent *event); // Checks the dropped file and opens it

    int sampleRate; // Stores the samplerate in hertz
    QLabel *sampleRateLabel; // Used for displaying the samplerate in the top right corner of the gui
    void updateSampleRateLabel(); // Updates the samplerate label

    QVector<double> ecg_x; // x-axis values for ecg_y (time in seconds)
    QVector<double> ecg_y; // Stores the actual ecg signal

    QLinkedList<QCPItemStraightLine*> peaks; // List of detected peaks
    QCPItemStraightLine* insertNewPeak(double position); // Insert a new peak at 'position' (time in seconds)
    void clearPeaks(); // Removes all peaks from the plot and clears the peaks vector

    QVector<double> ibi_x; // x-axis values for ibi_y (1, 2, ...)
    QVector<double> ibi_y; // Stores interbeat intervals

    QVector<double> hist_x;
    QVector<double> hist_y;

    void setupHistPlot(double maxIbiValue);

    QVector<double> artifacts_x;
    QVector<double> artifacts_y;

    void closeEvent(QCloseEvent *event);
    void saveSettings(); // Save settings when quitting
    void loadSettings(); // Load settings at startup
};

#endif // MAINWINDOW_H
