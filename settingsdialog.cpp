#include <QSerialPort>
#include <QSerialPortInfo>

#include "settingsdialog.h"
#include "ui_settingsdialog.h"
#include "settings.h"
#include "mainwindow.h"

///////////////////////////////////////////////////////////////////////////////
SettingsDialog::SettingsDialog(QWidget *parent) : QDialog(parent), ui(new Ui::SettingsDialog) {
    ui->setupUi(this);

    QString comPort = g_Settings.comPort;
    QString comDesc = getPortDescription(g_Settings.comPort);

    if (!comDesc.isEmpty()) {
        comPort += QString(" (%1)").arg(comDesc);
    }

    ui->comboComPort->addItems(getPortNames());
    ui->comboComPort->setCurrentText(comPort);

    ui->comboBaudRate->addItem("1200", QSerialPort::Baud1200);
    ui->comboBaudRate->addItem("2400", QSerialPort::Baud2400);
    ui->comboBaudRate->addItem("4800", QSerialPort::Baud4800);
    ui->comboBaudRate->addItem("9600", QSerialPort::Baud9600);
    ui->comboBaudRate->addItem("38400", QSerialPort::Baud38400);
    ui->comboBaudRate->addItem("57600", QSerialPort::Baud57600);
    ui->comboBaudRate->addItem("115200", QSerialPort::Baud115200);
    ui->comboBaudRate->setCurrentIndex(ui->comboBaudRate->findData(g_Settings.baudRate));

    ui->comboDataBits->addItem("5", QSerialPort::Data5);
    ui->comboDataBits->addItem("6", QSerialPort::Data6);
    ui->comboDataBits->addItem("7", QSerialPort::Data7);
    ui->comboDataBits->addItem("8", QSerialPort::Data8);
    ui->comboDataBits->setCurrentIndex(ui->comboDataBits->findData(g_Settings.dataBits));

    ui->comboStopBits->addItem("1", QSerialPort::OneStop);
    ui->comboStopBits->addItem("1.5", QSerialPort::OneAndHalfStop);
    ui->comboStopBits->addItem("2", QSerialPort::TwoStop);
    ui->comboStopBits->setCurrentIndex(ui->comboStopBits->findData(g_Settings.stopBits));

    ui->comboParity->addItem("None", QSerialPort::NoParity);
    ui->comboParity->addItem("Even", QSerialPort::EvenParity);
    ui->comboParity->addItem("Odd", QSerialPort::OddParity);
    ui->comboParity->setCurrentIndex(ui->comboParity->findData(g_Settings.parity));

    ui->comboFlowControl->addItem("None", QSerialPort::NoFlowControl);
    ui->comboFlowControl->addItem("Hardware", QSerialPort::HardwareControl);
    ui->comboFlowControl->addItem("Software", QSerialPort::SoftwareControl);
    ui->comboFlowControl->setCurrentIndex(ui->comboFlowControl->findData(g_Settings.flowControl));

    ui->spinCharDelay->setValue(g_Settings.charDelay);

    MainWindow *mw = (MainWindow*)parent;

    ui->comboEncoding->addItems(mw->getEncodingList());
    ui->comboEncoding->setCurrentText(g_Settings.textEncoding);

    ui->fontComboBox->setCurrentText(g_Settings.fontName);
    ui->spinFontSize->setValue(g_Settings.fontSize);

    ui->comboTimestamp->addItem("Disable");
    ui->comboTimestamp->addItem("Enable");
    ui->comboTimestamp->setCurrentText(g_Settings.timeStamp);

    QObject::connect(ui->btnOk, &QPushButton::clicked, this, &SettingsDialog::setSettings);
    QObject::connect(ui->btnCancel, &QPushButton::clicked, this, [this](){reject();});
}

///////////////////////////////////////////////////////////////////////////////
SettingsDialog::~SettingsDialog() {
    delete ui;
}

///////////////////////////////////////////////////////////////////////////////
void SettingsDialog::setSettings() {
    g_Settings.comPort = parsePort(ui->comboComPort->currentText());
    g_Settings.baudRate = ui->comboBaudRate->currentData().toInt();
    g_Settings.dataBits = ui->comboDataBits->currentData().toInt();
    g_Settings.stopBits = ui->comboStopBits->currentData().toInt();
    g_Settings.parity = ui->comboParity->currentData().toInt();
    g_Settings.flowControl = ui->comboFlowControl->currentData().toInt();
    g_Settings.charDelay = ui->spinCharDelay->value();
    g_Settings.textEncoding = ui->comboEncoding->currentText();
    g_Settings.fontName = ui->fontComboBox->currentText();
    g_Settings.fontSize = ui->spinFontSize->value();
    g_Settings.timeStamp = ui->comboTimestamp->currentText();
    accept();
}

///////////////////////////////////////////////////////////////////////////////
QString SettingsDialog::getPortDescription(const QString &text) {
    QString desc = "";

    foreach (QSerialPortInfo spi, QSerialPortInfo::availablePorts()) {
        if (spi.portName() == text) {
            return spi.description();
        }
    }

    return desc;
}

///////////////////////////////////////////////////////////////////////////////
QString SettingsDialog::parsePort(const QString &text) {
    QString portName = text;

    int i = text.indexOf(QChar(0x20));

    if (i != -1) {
        portName = text.left(i);
    }

    return portName;
}

///////////////////////////////////////////////////////////////////////////////
QStringList SettingsDialog::getPortNames() {
    QStringList list;

    foreach (QSerialPortInfo info, QSerialPortInfo::availablePorts()) {
        if (info.description().isEmpty()) {
            list.append(QString("%1").arg(info.portName()));
        } else {
            list.append(QString("%1 (%2)").arg(info.portName(), info.description()));
        }
    }

    return list;
}
