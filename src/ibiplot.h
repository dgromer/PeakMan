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
    double getSelectionGraphKey();
    double getSelectionValue();

signals:
    void ibiSelected(bool);

private slots:
    void mousePressEvent(QMouseEvent *event);

private:
    QCPItemTracer *selection;
};

#endif // IBIPLOT_H
