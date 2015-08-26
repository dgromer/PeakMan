#ifndef OPENFILEDIALOG_H
#define OPENFILEDIALOG_H

#include <QDialog>
#include "mainwindow.h"

namespace Ui {
class OpenFileDialog;
}

class OpenFileDialog : public QDialog
{
    Q_OBJECT

public:
    explicit OpenFileDialog(QWidget *parent, QString openFileName, int sampleRate);
    ~OpenFileDialog();
    int getSampleRate();
    QString getRadioButtonPushed();

private:
    Ui::OpenFileDialog *ui;
    QWidget *myParent;
};

#endif // OPENFILEDIALOG_H
