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

#include "ibiplot.h"

IBIPlot::IBIPlot(QWidget *parent) : QCustomPlot(parent)
{
    // Initialize tracer
    selection = new QCPItemTracer(this);
    selection->setStyle(QCPItemTracer::tsCircle);
    selection->setSize(10);
    selection->setPen(QPen(QBrush(QColor(234, 183, 0, 200)), 2));

    // Interactions
    setInteractions(QCP::iRangeDrag | QCP::iRangeZoom);
    axisRect()->setRangeZoom(xAxis->orientation());
    axisRect()->setRangeDrag(xAxis->orientation());

    // Set axis label
    yAxis->setLabel("Interbeat interval (ms)");

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
    yAxis->setUpperEnding(QCPLineEnding::esSpikeArrow);
    xAxis->setTicks(false);
    xAxis->setTickLabels(false);

    replot();
}

IBIPlot::~IBIPlot()
{

}

void IBIPlot::setTracer()
{
    selection->setGraph(graph(0));
}

void IBIPlot::unsetTracer()
{
    selection->setGraph(0);
}

// Return x value of tracer
double IBIPlot::getSelectionGraphKey()
{
    return selection->graphKey();
}

// Return y value of tracer
double IBIPlot::getSelectionValue()
{
    return selection->position->value();
}

void IBIPlot::plot(QVector<double> x, QVector<double> y)
{
    addGraph();
    graph(0)->setData(x, y);
    graph(0)->setPen(QColor(77, 77, 76));

    // Find largest interbeat interval and set plot view accordingly
    xAxis->setRange(-5, graph(0)->data()->size() + 5);
    yAxis->setRange(0, getMaxIbi() + 200);

    replot();
}

void IBIPlot::update(QVector<double> x, QVector<double> y)
{
    graph(0)->setData(x, y);
    yAxis->setRange(0, getMaxIbi() + 200);

    replot();
}

double IBIPlot::getMaxIbi()
{
    double maxIbi = 0;

    QList<QCPData> data = graph(0)->data()->values();

    for (int i = 0; i < data.size(); i++)
    {
        maxIbi = qMax(maxIbi, data[i].value);
    }

    return maxIbi;
}

// Set tracer on mouse click
void IBIPlot::mousePressEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton)
    {
        if (plottableAt(event->pos()))
        {
            double pos = qRound(xAxis->pixelToCoord((double) event->x()));

            selection->setGraphKey(pos);
            selection->setVisible(true);

            emit ibiSelected(true);
        }
        else
        {
            selection->setVisible(false);

            emit ibiSelected(false);
        }

        replot();
    }

    QCustomPlot::mousePressEvent(event);
}

