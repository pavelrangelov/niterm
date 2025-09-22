#include "helpdialog.h"
#include "ui_helpdialog.h"
#include "settings.h"

///////////////////////////////////////////////////////////////////////////////
HelpDialog::HelpDialog(QWidget *parent) : QDialog(parent), ui(new Ui::HelpDialog) {
    ui->setupUi(this);

    ui->labelText->setText(QString("%1 version %2").arg(APP_NAME).arg(APP_VERS));
}

///////////////////////////////////////////////////////////////////////////////
HelpDialog::~HelpDialog() {
    delete ui;
}

///////////////////////////////////////////////////////////////////////////////
void HelpDialog::on_btnOk_clicked() {
    accept();
}
