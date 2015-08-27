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

#include "ecgplot.h"

ECGPlot::ECGPlot(QWidget *parent) : QCustomPlot(parent)
{
    setFocusPolicy(Qt::ClickFocus);

    // Initialize rubberband
    rubberBand = 0;

    // Initialize layers
    addLayer("peaks");
    addLayer("data");
    addLayer("globalthresholdline");
    addLayer("highlight");

    // Set axis labels
    xAxis->setLabel("Time (s)");
    yAxis->setLabel("Voltage (mV)");

    // Activate interactions
    setInteractions(QCP::iRangeDrag | QCP::iRangeZoom | QCP::iMultiSelect | QCP::iSelectItems);

    // Appereance of axis grid
    xAxis->grid()->setPen(QPen(QColor(200, 200, 200), 1, Qt::DotLine));
    yAxis->grid()->setPen(QPen(QColor(200, 200, 200), 1, Qt::DotLine));
    xAxis->grid()->setSubGridPen(QPen(QColor(220, 220, 220), 1, Qt::DotLine));
    yAxis->grid()->setSubGridPen(QPen(QColor(220, 220, 220), 1, Qt::DotLine));
    xAxis->grid()->setSubGridVisible(true);
    yAxis->grid()->setSubGridVisible(true);

    // Global threshold line
    globalThresholdLine = new QCPItemStraightLine(this);
    globalThresholdLine->setPen(QPen(QColor(200, 40, 41)));
    globalThresholdLine->setSelectedPen(QPen(QBrush(QColor(234, 183, 0, 200)), 3));
    globalThresholdLine->setLayer("globalthresholdline");
    moveGlobalThresholdLine = false;

    // Interbeat interval highlight rectangle
    highlightRect = new QCPItemRect(this);
    highlightRect->setPen(Qt::NoPen);
    highlightRect->setBrush(QBrush(QColor(234, 183, 0, 100)));
    highlightRect->setLayer("highlight");
    highlightRect->setSelectable(false);
    highlightRect->setVisible(false);

    highlightTimer = new QTimer(this);
    connect(highlightTimer, SIGNAL(timeout()), this, SLOT(highlightTimerUpdate()));

    replot();
}

ECGPlot::~ECGPlot()
{

}

void ECGPlot::showIbiHighlightRect(double x, double width)
{
    highlightRectOpacity = 100;

    highlightRect->setBrush(QBrush(QColor(234, 183, 0, highlightRectOpacity)));
    highlightRect->position("topLeft")->setCoords(x - width / 2, 100000);
    highlightRect->position("bottomRight")->setCoords(x + width / 2, -100000);
    highlightRect->setVisible(true);

    highlightTimer->start(100);

    replot();
}

void ECGPlot::updateGlobalThresholdLine(int y)
{
    globalThresholdLine->point1->setCoords(0, y);
    globalThresholdLine->point2->setCoords(1, y);

    replot();
}

void ECGPlot::setGlobalThresholdLineVisible(bool visible)
{
    globalThresholdLine->setVisible(visible);

    replot();
}

void ECGPlot::mousePressEvent(QMouseEvent *event)
{
    if (globalThresholdLine->selected() && qAbs(event->pos().y() - (int)yAxis->coordToPixel(globalThresholdLine->point1->value())) < 10)
    {
        setCursor(Qt::ClosedHandCursor);
        moveGlobalThresholdLine = true;
        return;
    }

    if (event->button() == Qt::LeftButton)
    {
        origin = event->pos();

        if (!rubberBand)
        {
            rubberBand = new QRubberBand(QRubberBand::Rectangle, this);
        }

        rubberBand->setGeometry(QRect(origin, QSize()));
        rubberBand->show();
    }

    QCustomPlot::mousePressEvent(event);

    // Check if global threshold line is selected together with peaks
    if (selectedItems().contains(globalThresholdLine) && selectedItems().size() > 1)
    {
        // If so, remove global threshold line from selection
        globalThresholdLine->setSelected(false);
    }
}

void ECGPlot::mouseReleaseEvent(QMouseEvent *event)
{
    if (moveGlobalThresholdLine)
    {
        setCursor(Qt::OpenHandCursor);
        moveGlobalThresholdLine = false;

        double new_pos = qRound(globalThresholdLine->point1->value());

        globalThresholdLine->point1->setCoords(0, new_pos);
        globalThresholdLine->point2->setCoords(1, new_pos);

        emit globalThresholdChanged((int)new_pos);

        return;
    }

    // Check if user was selecting
    if (event->button() == Qt::LeftButton && rubberBand)
    {
        rubberBand->hide();

        if (!(event->modifiers() == Qt::ControlModifier))
        {
            deselectAll();
        }

        double x1, x2;

        x1 = qMin(xAxis->pixelToCoord((double) origin.x()), xAxis->pixelToCoord((double) event->x()));
        x2 = qMax(xAxis->pixelToCoord((double) origin.x()), xAxis->pixelToCoord((double) event->x()));

        QCPItemStraightLine *lineItem;

        for (int i = 0; i < itemCount(); i++)
        {
            lineItem = qobject_cast<QCPItemStraightLine*>(item(i));

            if (lineItem->point1->key() >= x1 && lineItem->point1->key() <= x2)
            {
                lineItem->setSelected(true);
            }
        }

        replot();
    }

    QCustomPlot::mouseReleaseEvent(event);
}

void ECGPlot::mouseMoveEvent(QMouseEvent *event)
{
    QCustomPlot::mouseMoveEvent(event);

    // Check if user is dragging with right mouse button
    if (event->button() == Qt::RightButton) return;

    if (moveGlobalThresholdLine)
    {
        double pos = yAxis->pixelToCoord((double) event->y());

        globalThresholdLine->point1->setCoords(0, pos);
        globalThresholdLine->point2->setCoords(1, pos);

        replot();

        return;
    }

    if (event->button() == Qt::NoButton)
    {
        if (globalThresholdLine->selected() && qAbs(event->pos().y() - (int)yAxis->coordToPixel(globalThresholdLine->point1->value())) < 10)
        {
            setCursor(Qt::OpenHandCursor);
        }
        else
        {
            unsetCursor();
        }
    }

    // Check if user is selecting
    if (rubberBand)
    {
        rubberBand->setGeometry(QRect(origin, event->pos()).normalized());
    }
}

void ECGPlot::mouseDoubleClickEvent(QMouseEvent *event)
{
    if (event->button() == Qt::RightButton) return;

    if (itemAt(event->pos()))
    {
        emit deletePeak(itemAt(event->pos()));
    }
    else
    {
        emit insertPeakAtPos(event->pos());
    }

    // TODO: implementation of slot in mainwindow
}

void ECGPlot::keyPressEvent(QKeyEvent *event)
{
    if (event->key() == Qt::Key_Delete && !selectedItems().isEmpty())
    {
        emit deletePeaks(selectedItems());
    }
}

void ECGPlot::highlightTimerUpdate()
{
    highlightRectOpacity -= 5;

    if (highlightRectOpacity == 0)
    {
        highlightTimer->stop();
        highlightRect->setVisible(false);
    }
    else
    {
        highlightRect->setBrush(QBrush(QColor(234, 183, 0, highlightRectOpacity)));
    }

    replot();
}

