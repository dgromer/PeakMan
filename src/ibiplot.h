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

#ifndef IBIPLOT_H
#define IBIPLOT_H

#include "qcustomplot.h"

class IBIPlot : public QCustomPlot
{
    Q_OBJECT

public:
    explicit IBIPlot(QWidget *parent);
    ~IBIPlot();

    void setTracer();
    void unsetTracer();
    double getSelectionPosX();
    double getSelectionPosY();
    double getSelectionTimePoint();
    double getReferenceInterval();
    void computeInterbeatIntervals(QLinkedList<QCPItemStraightLine *> peaks);
    void setup(QLinkedList<QCPItemStraightLine *> peaks, bool set_range = true);
    //void update(QLinkedList<QCPItemStraightLine *> peaks);
    void plot(QVector<double> x, QVector<double> y, bool set_range = true);
    void clear();
    void plotArtifacts(QVector<double> x, QVector<double> y);
    void clearArtifacts();
    double getMaxIbi();

    QVector<double> getIbi_y();

public slots:
    void artifactDetection(); // Search for artifacts in sequence of interbeat intervals

signals:
    void ibiSelected(bool);
    void setupHistPlot(QVector<double>, double);

private slots:
    void mousePressEvent(QMouseEvent *event);

private:
    QCPItemTracer *selection;

    QCPGraph *ibi;
    QVector<double> ibi_x;
    QVector<double> ibi_y;

    QCPGraph *artifacts;
};

#endif // IBIPLOT_H
