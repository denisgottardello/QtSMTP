#include "qfmainform.h"
#include "ui_qfmainform.h"

QFMainForm::QFMainForm(QWidget *parent) : QMainWindow(parent), ui(new Ui::QFMainForm) {
    ui->setupUi(this);
    QRect scr= QApplication::desktop()->screenGeometry();
    move(scr.center()- rect().center());
}

QFMainForm::~QFMainForm() {
    delete ui;
}

void QFMainForm::on_QPBClose_clicked() {
    this->close();
}

void QFMainForm::on_QPBSend_clicked() {
    QCNetSMTP QCNetSMTP(true);
    QCNetSMTP.Server= ui->QLEServer->text();
    QCNetSMTP.Socket= ui->QSBSocket->value();
    QCNetSMTP.Authentication= ui->QCBAuthentication->isChecked();
    QCNetSMTP.UserID= ui->QLEUserID->text();
    QCNetSMTP.Password= ui->QLEPassword->text();
    QCNetSMTP.From= ui->QLEFrom->text();
    QCNetSMTP.To= ui->QLETo->text();
    QCNetSMTP.Cc= ui->QLECc->text();
    QCNetSMTP.Bcc= ui->QLEBcc->text();
    QCNetSMTP.Subject= ui->QLESubject->text();
    QCNetSMTP.BodyPlain= ui->QPTEBodyPlain->document()->toPlainText();
    QCNetSMTP.BodyHtml= ui->QPTEBodyHtml->document()->toPlainText();
    QCNetSMTP.HtmlFormat= ui->QCBHtmlFormat->isChecked();
    QCNetSMTP.SSLConnection= ui->QCBSSLConnection->isChecked();
    if (ui->QRBNormal->isChecked()) QCNetSMTP.Priority= QCNetSMTP::PRIORITYNORMAL;
    else if (ui->QRBMedium->isChecked()) QCNetSMTP.Priority= QCNetSMTP::PRIORITYMEDIUM;
    else if (ui->QRBHigh->isChecked()) QCNetSMTP.Priority= QCNetSMTP::PRIORITYHIGH;
    //QCNetSMTP.AddAttach(ui->QLEAttach->text(), "a");
    QCNetSMTP.AddAttach("b.jpg", "b");
    //QCNetSMTP.AddAttach("c.jpg");
    if (QCNetSMTP.Execute(60000)) qDebug() << "Ok";
    else qDebug() << "Error!";
}
