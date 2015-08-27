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

#include "openfiledialog.h"
#include "ui_openfiledialog.h"

OpenFileDialog::OpenFileDialog(QWidget *parent, QString openFileName, int sampleRate) :
    QDialog(parent),
    ui(new Ui::OpenFileDialog)
{
    ui->setupUi(this);

    setWindowTitle("Open File");

    ui->fileNameLineEdit->setText(openFileName);
    ui->sampleRateSpinBox->setValue(sampleRate);
}

OpenFileDialog::~OpenFileDialog()
{
    delete ui;
}

int OpenFileDialog::getSampleRate()
{
    return ui->sampleRateSpinBox->value();
}

QString OpenFileDialog::getRadioButtonPushed()
{
    if (ui->ecgSignalButton->isChecked())
    {
        return "ecgsignal";
    }
    else if (ui->peaksButton->isChecked())
    {
        return "peaks";
    }
    else if (ui->ibiButton->isChecked())
    {
        return "ibi";
    } else
    {
        return "error";
    }
}
