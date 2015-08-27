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
