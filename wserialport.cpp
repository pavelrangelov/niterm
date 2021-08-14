#include <QApplication>
#include <QByteArray>
#include <QMutex>
#include <QWaitCondition>
#include <QMessageBox>
#include <QDateTime>
#include <QEventLoop>
#include <QSerialPort>
#include <QSerialPortInfo>

#include "settings.h"
#include "wserialport.h"

const qint32 WSerialPort::BaudRatesArray[BAUD_RATES_COUNT] =
{
    QSerialPort::Baud1200,
    QSerialPort::Baud2400,
    QSerialPort::Baud4800,
    QSerialPort::Baud9600,
    QSerialPort::Baud19200,
    QSerialPort::Baud38400,
    QSerialPort::Baud57600,
    QSerialPort::Baud115200
};

///////////////////////////////////////////////////////////////////////////////
const QSerialPort::DataBits WSerialPort::DataBitsArray[4] =
{
    QSerialPort::Data5,
    QSerialPort::Data6,
    QSerialPort::Data7,
    QSerialPort::Data8
};

///////////////////////////////////////////////////////////////////////////////
const QSerialPort::StopBits WSerialPort::StopBitsArray[3] =
{
    QSerialPort::OneStop,
    QSerialPort::OneAndHalfStop,
    QSerialPort::TwoStop
};

///////////////////////////////////////////////////////////////////////////////
const QSerialPort::Parity WSerialPort::ParityArray[3] =
{
    QSerialPort::NoParity,
    QSerialPort::EvenParity,
    QSerialPort::OddParity
};

///////////////////////////////////////////////////////////////////////////////
const QSerialPort::FlowControl WSerialPort::FlowControlArray[3] =
{
    QSerialPort::NoFlowControl,
    QSerialPort::HardwareControl,
    QSerialPort::SoftwareControl
};

///////////////////////////////////////////////////////////////////////////////
const QString WSerialPort::ASCII_table[ASCII_TABLE_SIZE] =
{
    "NUL",  // 00
    "SOH",  // 01
    "STX",  // 02
    "ETX",  // 03
    "EOT",  // 04
    "ENQ",  // 05
    "ACK",  // 06
    "BEL",  // 07
    "BS",   // 08
    "HT",   // 09
    "LF",   // 0A
    "VT",   // 0B
    "FF",   // 0C
    "CR",   // 0D
    "SO",   // 0E
    "SI",   // 0F
    "DLE",  // 10
    "DC1",  // 11
    "DC2",  // 12
    "DC3",  // 13
    "DC4",  // 14
    "NAK",  // 15
    "SYN",  // 16
    "ETB",  // 17
    "CAN",  // 18
    "EM",   // 19
    "SUB",  // 1A
    "ESC",  // 1B
    "FS",   // 1C
    "GS",   // 1D
    "RS",   // 1E
    "US",   // 1F
};

///////////////////////////////////////////////////////////////////////////////
WSerialPort::WSerialPort()
{
}

///////////////////////////////////////////////////////////////////////////////
WSerialPort::~WSerialPort()
{
}

///////////////////////////////////////////////////////////////////////////////
bool WSerialPort::connect()
{
    #ifdef Q_OS_WIN
    QString cname(g_Settings.comPort);

    int index = cname.indexOf("(COM");

    if (index != -1)
    {
        index++;
        cname = cname.mid(index,5);
        cname.remove(')');
    }

    QString fname = "\\\\.\\";
    fname += cname;
    #else
    QString fname = "/dev/";
    fname += g_Settings.comPort;
    #endif

    setPortName     (fname);
    setBaudRate     (BaudRatesArray     [g_Settings.baudRateIndex]);
    setDataBits     (DataBitsArray      [g_Settings.dataBitsIndex]);
    setParity       (ParityArray        [g_Settings.parityIndex]);
    setStopBits     (StopBitsArray      [g_Settings.stopBitsIndex]);
    setFlowControl  (FlowControlArray   [g_Settings.flowControlIndex]);

    close();

    if (!open(QIODevice::ReadWrite))
    {
        return false;
    }

    return true;
}

///////////////////////////////////////////////////////////////////////////////
void WSerialPort::disconnect()
{
    close();
}
