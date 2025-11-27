#ifndef WSERIALPORT_H
#define WSERIALPORT_H

#include <QSerialPort>

#define BAUD_RATES_COUNT    8
#define ASCII_TABLE_SIZE    32

///////////////////////////////////////////////////////////////////////////////
class WSerialPort : public QSerialPort {
    public:
        WSerialPort(QObject *parent = nullptr);
        ~WSerialPort();

        static const qint32 BaudRatesArray[BAUD_RATES_COUNT];
        static const QSerialPort::DataBits DataBitsArray[4];
        static const QSerialPort::StopBits StopBitsArray[3];
        static const QSerialPort::Parity ParityArray[3];
        static const QSerialPort::FlowControl FlowControlArray[3];
        static const QString ASCII_table[32];

        bool connect();
        void disconnect();
};

#endif // WSERIALPORT_H
