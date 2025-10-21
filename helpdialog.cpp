#include "helpdialog.h"
#include "ui_helpdialog.h"
#include "settings.h"

///////////////////////////////////////////////////////////////////////////////
HelpDialog::HelpDialog(QWidget *parent) : QDialog(parent), ui(new Ui::HelpDialog) {
    ui->setupUi(this);

    ui->labelText->setText(QString("%1 version %2").arg(APP_NAME, APP_VERS));

    QObject::connect(ui->btnOk, &QPushButton::clicked, this, [this](){accept();});
}

///////////////////////////////////////////////////////////////////////////////
HelpDialog::~HelpDialog() {
    delete ui;
}
