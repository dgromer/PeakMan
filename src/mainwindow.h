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
    void horzScrollBarChanged(int value);
    void xAxisChanged(QCPRange range);
    void yAxisChanged(QCPRange range);
    void vertSliderChanged(int value);

    void getFileName();
    void closeCurrentFile();
    void saveInterbeatIntervals();

    void peakdet();
    void insertPeakAtPos(QPoint position); // Used for inserting a peak at clicked position
    void insertPeakAtPoint(double position); // Used for inserting a peak at a specific time
    void deletePeak(QCPAbstractItem *peak);
    void deletePeaks(QList<QCPAbstractItem*> peaksToDelete);

    void setupIbiPlot();
    void jumpToSelection();
    void artifactDetection();
    void insertMissingPeaks();

private:
    Ui::MainWindow *ui;

    QString openFileName;
    void execOpenFileDialog();
    void openEcgFile();
    void openPeaksFile();
    void openIbiFile();
    void dragEnterEvent(QDragEnterEvent *event);
    void dropEvent(QDropEvent *event);

    int sampleRate;
    QLabel *sampleRateLabel;
    void updateSampleRateLabel();

    QVector<double> ecg_x;
    QVector<double> ecg_y;

    QLinkedList<QCPItemStraightLine*> peaks;
    QCPItemStraightLine* insertNewPeak(double position);

    QVector<double> ibi_x;
    QVector<double> ibi_y;

    QVector<double> hist_x;
    QVector<double> hist_y;

    void setupHistPlot(double maxIbiValue);

    QVector<double> artifacts_x;
    QVector<double> artifacts_y;

    void closeEvent(QCloseEvent *event);
    void saveSettings();
    void loadSettings();
};

#endif // MAINWINDOW_H
