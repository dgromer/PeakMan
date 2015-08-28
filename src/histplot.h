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

#ifndef HISTPLOT_H
#define HISTPLOT_H

#include "qcustomplot.h"

class HistPlot : public QCustomPlot
{
    Q_OBJECT

public:
    explicit HistPlot(QWidget *parent);
    ~HistPlot();

    void plot(QVector<double> x, QVector<double> y, double max_x, double max_y);
    void clear();
};

#endif // HISTPLOT_H
