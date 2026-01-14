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

#include "settingsdialog.h"
#include "ui_settingsdialog.h"

SettingsDialog::SettingsDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::SettingsDialog)
{
    ui->setupUi(this);
    setWindowTitle("Settings");
}

SettingsDialog::~SettingsDialog()
{
    delete ui;
}

bool SettingsDialog::getIncludeStartEnd() const
{
    return ui->includeStartEndCheckBox->isChecked();
}

void SettingsDialog::setIncludeStartEnd(bool include)
{
    ui->includeStartEndCheckBox->setChecked(include);
}

double SettingsDialog::getArtifactThreshold() const
{
    return ui->artifactThresholdSpinBox->value();
}

void SettingsDialog::setArtifactThreshold(double threshold)
{
    ui->artifactThresholdSpinBox->setValue(threshold);
}
