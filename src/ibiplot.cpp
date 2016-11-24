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
    // Initialize graphs
    ibi = addGraph();
    artifacts = addGraph();

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
    selection->setGraph(ibi);
}

void IBIPlot::unsetTracer()
{
    selection->setGraph(0);
}

// Return x value of tracer
double IBIPlot::getSelectionPosX()
{
    return selection->graphKey();
}

// Return y value of tracer
double IBIPlot::getSelectionPosY()
{
    return selection->position->value();
}

double IBIPlot::getSelectionTimePoint()
{
    double x = 0;

    // Sum interbeat intervals up to selection to get x-axis position in ecgPlot
    for (int i  = 0; i < (int)getSelectionPosX(); i++)
    {
        x += ibi_y[i] / 1000;
    }

    return x;
}

double IBIPlot::getReferenceInterval()
{
    return ibi_y[(int)getSelectionPosX() - 2] / 1000;
}

// Compute interbeat intervals from peak positions
void IBIPlot::computeInterbeatIntervals(QLinkedList<QCPItemStraightLine*> peaks)
{
    clear();

    double lastPeakPosition = peaks.first()->point1->key() * 1000;
    int i = 0;

    // Compute interbeat intervals in msec
    foreach (QCPItemStraightLine *peak, peaks)
    {
        ibi_x << (double) i++;
        ibi_y << peak->point1->key() * 1000 - lastPeakPosition;

        lastPeakPosition += ibi_y.last();
    }

    // First element is zero
    ibi_x.removeFirst();
    ibi_y.removeFirst();
}

void IBIPlot::plot(QVector<double> x, QVector<double> y, bool set_range)
{
    ibi->setData(x, y);
    ibi->setPen(QColor(77, 77, 76));

    if (set_range)
    {
        // Find largest interbeat interval and set plot view accordingly
        yAxis->setRange(0, getMaxIbi() + 200);
    }

    replot();
}

void IBIPlot::resetView()
{
    // Find largest interbeat interval and set plot view accordingly
    xAxis->setRange(-5, ibi->data()->size() + 5);
    yAxis->setRange(0, getMaxIbi() + 200);

    replot();
}

void IBIPlot::clear()
{
    ibi->clearData();
    ibi_x.clear();
    ibi_y.clear();

    clearArtifacts();
    unsetTracer();

    replot();
}

void IBIPlot::plotArtifacts(QVector<double> x, QVector<double> y)
{
    artifacts->setData(x, y);
    artifacts->setPen(Qt::NoPen);
    artifacts->setScatterStyle(QCPScatterStyle(QCPScatterStyle::ssCircle, QColor(200, 40, 41), QColor(200, 40, 41), 7));
    replot();
}

void IBIPlot::clearArtifacts()
{
    artifacts->clearData();
}

double IBIPlot::getMaxIbi()
{
    double maxIbi = 0;

    QList<QCPData> data = ibi->data()->values();

    for (int i = 0; i < data.size(); i++)
    {
        maxIbi = qMax(maxIbi, data[i].value);
    }

    return maxIbi;
}

QVector<double> IBIPlot::getIbi_y()
{
    return ibi_y;
}

void IBIPlot::setup(QLinkedList<QCPItemStraightLine *> peaks, bool set_range)
{
    computeInterbeatIntervals(peaks);
    plot(ibi_x, ibi_y, set_range);
    setTracer();
    emit setupHistPlot(ibi_y, getMaxIbi());
}

void IBIPlot::artifactDetection()
{
    QVector<double> artifacts_x;
    QVector<double> artifacts_y;

    for (int i = 1; i < ibi_y.size(); i++)
    {
        if (qAbs(ibi_y[i] - ibi_y[i - 1]) > .2 * ibi_y[i - 1])
        {
            artifacts_x << ibi_x[i];
            artifacts_y << ibi_y[i];
        }
    }

    plotArtifacts(artifacts_x, artifacts_y);
}

// TODO: improve selecting mechanism here: should also look at y-position of click (maybe nearest point of ibi line)
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

void IBIPlot::mouseDoubleClickEvent(QMouseEvent *event)
{
    if (selection->visible())
    {
        emit ibiSelectedDoubleClick();
    }
}



