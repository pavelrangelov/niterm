#include <QSerialPort>
#include <QSerialPortInfo>

#include "settingsdialog.h"
#include "ui_settingsdialog.h"
#include "wserialport.h"
#include "settings.h"
#include "mainwindow.h"

///////////////////////////////////////////////////////////////////////////////
SettingsDialog::SettingsDialog(QWidget *parent) : QDialog(parent), ui(new Ui::SettingsDialog)
{
    ui->setupUi(this);

    QString comPort = g_Settings.comPort;
    QString comDesc = getPortDescripion(g_Settings.comPort);

    if (!comDesc.isEmpty())
    {
        comPort += QString(" (%1)").arg(comDesc);
    }

    ui->comboComPort->addItems(getPortNames());
    ui->comboComPort->setCurrentText(comPort);

    for (int i=0; i<BAUD_RATES_COUNT; i++)
    {
        ui->comboBaudRate->addItems(QStringList() << QString("%1").arg(WSerialPort::BaudRatesArray[i]));
    }
    ui->comboBaudRate->setCurrentIndex(g_Settings.baudRateIndex);

    ui->comboDataBits->addItem("5");
    ui->comboDataBits->addItem("6");
    ui->comboDataBits->addItem("7");
    ui->comboDataBits->addItem("8");
    ui->comboDataBits->setCurrentIndex(g_Settings.dataBitsIndex);

    ui->comboStopBits->addItem("1");
    ui->comboStopBits->addItem("1.5");
    ui->comboStopBits->addItem("2");
    ui->comboStopBits->setCurrentIndex(g_Settings.stopBitsIndex);

    ui->comboParity->addItem("None");
    ui->comboParity->addItem("Even");
    ui->comboParity->addItem("Odd");
    ui->comboParity->setCurrentIndex(g_Settings.parityIndex);

    ui->comboFlowControl->addItem("None");
    ui->comboFlowControl->addItem("Hardware");
    ui->comboFlowControl->addItem("Software");
    ui->comboFlowControl->setCurrentIndex(g_Settings.flowControlIndex);

    ui->spinCharDelay->setValue(g_Settings.charDelay);

    MainWindow *mw = (MainWindow*)parent;

    ui->comboEncoding->addItems(mw->getEncodingList());
    ui->comboEncoding->setCurrentText(g_Settings.textEncoding);

    ui->fontComboBox->setCurrentText(g_Settings.fontName);
    ui->spinFontSize->setValue(g_Settings.fontSize);

    ui->comboTimestamp->addItem("Disable");
    ui->comboTimestamp->addItem("Enable");
    ui->comboTimestamp->setCurrentText(g_Settings.timeStamp);
}

///////////////////////////////////////////////////////////////////////////////
SettingsDialog::~SettingsDialog()
{
    delete ui;
}

///////////////////////////////////////////////////////////////////////////////
void SettingsDialog::on_btnOK_clicked()
{
    g_Settings.comPort          = parsePort(ui->comboComPort->currentText());   // 1
    g_Settings.baudRateIndex    = ui->comboBaudRate->currentIndex();            // 2
    g_Settings.dataBitsIndex    = ui->comboDataBits->currentIndex();            // 3
    g_Settings.stopBitsIndex    = ui->comboStopBits->currentIndex();            // 4
    g_Settings.parityIndex      = ui->comboParity->currentIndex();              // 5
    g_Settings.flowControlIndex = ui->comboFlowControl->currentIndex();         // 6
    g_Settings.charDelay        = ui->spinCharDelay->value();                   // 7
    g_Settings.textEncoding     = ui->comboEncoding->currentText();             // 8
    g_Settings.fontName         = ui->fontComboBox->currentText();              // 9
    g_Settings.fontSize         = ui->spinFontSize->value();                    // 10
    g_Settings.timeStamp        = ui->comboTimestamp->currentText();            // 11

    accept();
}

///////////////////////////////////////////////////////////////////////////////
void SettingsDialog::on_btnCancel_clicked()
{
    reject();
}

///////////////////////////////////////////////////////////////////////////////
QString SettingsDialog::getPortDescripion(const QString &text)
{
    QString desc = "";

    foreach (QSerialPortInfo spi, QSerialPortInfo::availablePorts())
    {
        if (spi.portName() == text)
        {
            return spi.description();
        }
    }

    return desc;
}

///////////////////////////////////////////////////////////////////////////////
QString SettingsDialog::parsePort(const QString &text)
{
    QString portName = text;

    int i = text.indexOf(QChar(0x20));

    if (i != -1)
    {
        portName = text.left(i);
    }

    return portName;
}

///////////////////////////////////////////////////////////////////////////////
QStringList SettingsDialog::getPortNames()
{
    QStringList list;

    foreach (QSerialPortInfo spi, QSerialPortInfo::availablePorts())
    {
        if (spi.description().isEmpty())
        {
            list.append(QString("%1").arg(spi.portName()));
        }
        else
        {
            list.append(QString("%1 (%2)").arg(spi.portName()).arg(spi.description()));
        }
    }

    return list;
}
