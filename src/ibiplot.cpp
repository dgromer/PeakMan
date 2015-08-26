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

