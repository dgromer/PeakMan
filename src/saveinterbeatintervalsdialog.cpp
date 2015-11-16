#include "saveinterbeatintervalsdialog.h"
#include "ui_saveinterbeatintervalsdialog.h"

SaveInterbeatIntervalsDialog::SaveInterbeatIntervalsDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::SaveInterbeatIntervalsDialog)
{
    ui->setupUi(this);

    setWindowTitle("Save Interbeat Intervals");
}

SaveInterbeatIntervalsDialog::~SaveInterbeatIntervalsDialog()
{
    delete ui;
}

bool SaveInterbeatIntervalsDialog::includeSignalStartEnd()
{
    if (ui->includeSignalStartEndCheckBox->isChecked())
    {
        return true;
    }
    else
    {
        return false;
    }
}
