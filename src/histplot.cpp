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

#include "histplot.h"

HistPlot::HistPlot(QWidget *parent) : QCustomPlot(parent)
{
    // Appereance of axis grid
    xAxis->grid()->setPen(QPen(QColor(200, 200, 200), 1, Qt::DotLine));
    yAxis->grid()->setPen(QPen(QColor(200, 200, 200), 1, Qt::DotLine));
    xAxis->grid()->setSubGridPen(QPen(QColor(220, 220, 220), 1, Qt::DotLine));
    yAxis->grid()->setSubGridPen(QPen(QColor(220, 220, 220), 1, Qt::DotLine));
    xAxis->grid()->setSubGridVisible(true);
    yAxis->grid()->setSubGridVisible(true);
    xAxis->grid()->setZeroLinePen(Qt::NoPen);
    yAxis->grid()->setZeroLinePen(Qt::NoPen);
    xAxis->setUpperEnding(QCPLineEnding::esSpikeArrow);
    yAxis->setTicks(false);
    yAxis->setTickLabels(false);

    replot();
}

HistPlot::~HistPlot()
{

}

//void HistPlot::plot(QVector<double> x, QVector<double> y, double max_x, double max_y)
//{
//    if (plottableCount() > 0)
//    {
//        removePlottable(0);
//    }

//    QCPBars *bars = new QCPBars(xAxis, yAxis);
//    addPlottable(bars);
//    bars->setWidth(10);
//    bars->setData(x, y);
//    bars->setPen(QPen(Qt::black));
//    bars->setBrush(QColor(77, 77, 76));

//    xAxis->setRange(0, max_x + 20);
//    yAxis->setRange(0, max_y + 5);

//    replot();
//}

void HistPlot::plot(QVector<double> ibis, double maxIbi)
{
    QVector<double> hist_y = QVector<double>(qFloor(maxIbi / 10) + 1);
    QVector<double> hist_x = QVector<double>(hist_y.size());

    double maxHistValue = 0;

    for (int i = 0; i < ibis.size(); i++)
    {
        hist_y[qFloor(ibis[i] / 10)]++;
    }

    for (int i = 0; i < hist_x.size(); i++)
    {
        hist_x[i] = i * 10 + 5;
        maxHistValue = qMax(maxHistValue, hist_y[i]);
    }

    if (plottableCount() > 0)
    {
        removePlottable(0);
    }

    QCPBars *bars = new QCPBars(xAxis, yAxis);
    addPlottable(bars);
    bars->setWidth(10);
    bars->setData(hist_x, hist_y);
    bars->setPen(QPen(Qt::black));
    bars->setBrush(QColor(77, 77, 76));

    xAxis->setRange(0, maxIbi + 20);
    yAxis->setRange(0, maxHistValue + 5);

    replot();
}

void HistPlot::clear()
{
    removePlottable(0);
    replot();
}

void HistPlot::setup(QVector<double> ibis, double maxIbi)
{
    plot(ibis, maxIbi);
}
