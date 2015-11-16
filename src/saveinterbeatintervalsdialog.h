#ifndef SAVEINTERBEATINTERVALSDIALOG_H
#define SAVEINTERBEATINTERVALSDIALOG_H

#include <QDialog>

namespace Ui {
class SaveInterbeatIntervalsDialog;
}

class SaveInterbeatIntervalsDialog : public QDialog
{
    Q_OBJECT

public:
    explicit SaveInterbeatIntervalsDialog(QWidget *parent = 0);
    ~SaveInterbeatIntervalsDialog();

    bool includeSignalStartEnd();

private:
    Ui::SaveInterbeatIntervalsDialog *ui;
};

#endif // SAVEINTERBEATINTERVALSDIALOG_H
