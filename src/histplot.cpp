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

