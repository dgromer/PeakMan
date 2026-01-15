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

#include "ibiplot.h"
#include <limits>

IBIPlot::IBIPlot(QWidget *parent) : QCustomPlot(parent)
{
    setFocusPolicy(Qt::ClickFocus);

    // Initialize graphs
    ibi = addGraph();
    artifacts = addGraph();

    // Initialize artifact detection threshold (default 20%)
    artifactThreshold = 0.2;

    // Initialize artifact navigation state
    currentArtifactIndex = -1;

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
    selection->setVisible(false);
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

void IBIPlot::setArtifactThreshold(double thresholdPercent)
{
    // Convert percentage to decimal (e.g., 20.0 -> 0.2)
    artifactThreshold = thresholdPercent / 100.0;

    // If we have data, re-run detection with new threshold
    if (!ibi_y.isEmpty())
    {
        artifactDetection();
    }
}

double IBIPlot::getArtifactThreshold() const
{
    return artifactThreshold;
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

    // Reset artifact navigation state
    artifactIndices.clear();
    currentArtifactIndex = -1;

    // Reset tracer position and unset it
    selection->setGraphKey(0);
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
    artifactDetection();
}

void IBIPlot::artifactDetection()
{
    QVector<double> artifacts_x;
    QVector<double> artifacts_y;

    // Clear artifact indices list
    artifactIndices.clear();
    currentArtifactIndex = -1;

    for (int i = 1; i < ibi_y.size(); i++)
    {
        // Check if current IBI differs from previous IBI by more than threshold percentage
        if (qAbs(ibi_y[i] - ibi_y[i - 1]) > artifactThreshold * ibi_y[i - 1])
        {
            // Would be flagged as artifact - but check if previous IBI is already an artifact
            // If so, also compare against the second-to-last IBI to avoid false positives
            if (artifactIndices.contains(i - 1) && i >= 2)
            {
                // Previous is an artifact, re-check against second-to-last IBI
                if (qAbs(ibi_y[i] - ibi_y[i - 2]) > artifactThreshold * ibi_y[i - 2])
                {
                    artifacts_x << ibi_x[i];
                    artifacts_y << ibi_y[i];
                    artifactIndices << i;  // Store the index
                }
            }
            else
            {
                // Previous is NOT an artifact, flag this one
                artifacts_x << ibi_x[i];
                artifacts_y << ibi_y[i];
                artifactIndices << i;  // Store the index
            }
        }
    }

    plotArtifacts(artifacts_x, artifacts_y);
    emit artifactsChanged();
}

int IBIPlot::findNearestDataPoint(const QPoint &pixelPos) const
{
    // Check if we have data to search
    if (ibi_x.isEmpty() || ibi_y.isEmpty())
        return -1;

    // Convert click position to QPointF
    QPointF clickPixel(pixelPos);

    // Track minimum distance (squared) and the index
    double minDistSqr = std::numeric_limits<double>::max();
    int nearestIndex = -1;

    // Iterate through all data points
    for (int i = 0; i < ibi_x.size(); ++i)
    {
        // Convert data point coordinates to pixel space
        double dataX = xAxis->coordToPixel(ibi_x[i]);
        double dataY = yAxis->coordToPixel(ibi_y[i]);

        // Calculate squared distance
        double dx = clickPixel.x() - dataX;
        double dy = clickPixel.y() - dataY;
        double distSqr = dx * dx + dy * dy;

        // Update if this is closer
        if (distSqr < minDistSqr)
        {
            minDistSqr = distSqr;
            nearestIndex = i;
        }
    }

    return nearestIndex;
}

// Set tracer on mouse click
void IBIPlot::mousePressEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton)
    {
        bool bIbiSelected = false;

        if (plottableAt(event->pos()))
        {
            // Find nearest data point by 2D distance
            int nearestIndex = findNearestDataPoint(event->pos());

            if (nearestIndex >= 0)
            {
                selection->setGraphKey(ibi_x[nearestIndex]);
                selection->setVisible(true);
                bIbiSelected = true;

                // Update currentArtifactIndex if selected point is an artifact
                currentArtifactIndex = artifactIndices.indexOf(nearestIndex);
            }
            else
            {
                selection->setVisible(false);
                currentArtifactIndex = -1;
            }
        }
        else
        {
            selection->setVisible(false);
            currentArtifactIndex = -1;
        }

        replot();
        emit ibiSelected(bIbiSelected);
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

// Artifact navigation methods
int IBIPlot::getArtifactCount() const
{
    return artifactIndices.size();
}

QVector<int> IBIPlot::getArtifactIndices() const
{
    return artifactIndices;
}

bool IBIPlot::isCurrentSelectionArtifact() const
{
    if (!selection->visible())
        return false;

    // Get current selection index
    double selectionX = selection->graphKey();
    int selectionIndex = -1;

    // Find the index in ibi_x that matches the selection
    for (int i = 0; i < ibi_x.size(); ++i)
    {
        if (qAbs(ibi_x[i] - selectionX) < 0.001)  // Use small epsilon for floating point comparison
        {
            selectionIndex = i;
            break;
        }
    }

    if (selectionIndex == -1)
        return false;

    // Check if this index is in artifactIndices
    return artifactIndices.contains(selectionIndex);
}

int IBIPlot::getCurrentArtifactNumber() const
{
    if (currentArtifactIndex == -1)
        return -1;

    // Return 1-based position
    return currentArtifactIndex + 1;
}

void IBIPlot::selectArtifact(int artifactListIndex)
{
    // Bounds check
    if (artifactListIndex < 0 || artifactListIndex >= artifactIndices.size())
        return;

    // Get the data index from artifactIndices
    int dataIndex = artifactIndices[artifactListIndex];

    // Bounds check for ibi_x
    if (dataIndex < 0 || dataIndex >= ibi_x.size())
        return;

    // Set tracer position
    selection->setGraphKey(ibi_x[dataIndex]);
    selection->setVisible(true);

    // Update current artifact index
    currentArtifactIndex = artifactListIndex;

    // Emit signal
    emit ibiSelected(true);

    // Redraw
    replot();
}

int IBIPlot::findNearestArtifactLeft(int fromIndex) const
{
    // Search backward through artifactIndices
    for (int i = artifactIndices.size() - 1; i >= 0; --i)
    {
        if (artifactIndices[i] < fromIndex)
        {
            return i;  // Return position in artifact list
        }
    }

    return -1;  // No artifact found to the left
}

int IBIPlot::findNearestArtifactRight(int fromIndex) const
{
    // Search forward through artifactIndices
    for (int i = 0; i < artifactIndices.size(); ++i)
    {
        if (artifactIndices[i] > fromIndex)
        {
            return i;  // Return position in artifact list
        }
    }

    return -1;  // No artifact found to the right
}
