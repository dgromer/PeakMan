#ifndef ECGPLOT_H
#define ECGPLOT_H

#include <QRubberBand>
#include "qcustomplot.h"

class ECGPlot : public QCustomPlot
{
    Q_OBJECT

public:
    explicit ECGPlot(QWidget *parent);
    ~ECGPlot();
    void showIbiHighlightRect(double x, double width);

signals:
    void insertPeakAtPos(QPoint position);
    void deletePeak(QCPAbstractItem*);
    void deletePeaks(QList<QCPAbstractItem*>);

    void globalThresholdChanged(int);

public slots:
    void updateGlobalThresholdLine(int y);
    void setGlobalThresholdLineVisible(bool visible);

private slots:
    void mousePressEvent(QMouseEvent *event);
    void mouseReleaseEvent(QMouseEvent *event);
    void mouseMoveEvent(QMouseEvent *event);
    void mouseDoubleClickEvent(QMouseEvent *event);
    void keyPressEvent(QKeyEvent *event);

    void highlightTimerUpdate();

private:
    QRubberBand *rubberBand;
    QPoint origin;

    QCPItemStraightLine *globalThresholdLine;
    bool moveGlobalThresholdLine;

    QCPItemRect *highlightRect;
    int highlightRectOpacity;
    QTimer *highlightTimer;
};

#endif // ECGPLOT_H
