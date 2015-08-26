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
