#ifndef HISTPLOT_H
#define HISTPLOT_H

#include "qcustomplot.h"

class HistPlot : public QCustomPlot
{
    Q_OBJECT

public:
    explicit HistPlot(QWidget *parent);
    ~HistPlot();
};

#endif // HISTPLOT_H
