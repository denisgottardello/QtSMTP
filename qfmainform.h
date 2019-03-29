#ifndef QFMAINFORM_H
#define QFMAINFORM_H

#include <QMainWindow>
#include "QDesktopWidget"
#include "QDebug"
#include "qcnetsmtp.h"

namespace Ui {
    class QFMainForm;
}

class QFMainForm : public QMainWindow
{
    Q_OBJECT

public:
    explicit QFMainForm(QWidget *parent = 0);
    ~QFMainForm();

private:
    Ui::QFMainForm *ui;

private slots:
    void on_QPBSend_clicked();
    void on_QPBClose_clicked();

};

#endif // QFMAINFORM_H
