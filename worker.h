#ifndef WORKER_H
#define WORKER_H

#include <QObject>
#include <QThread>
#include <QSerialPort>

#include "wserialport.h"

class Worker : public QThread {
    Q_OBJECT
    public:
        explicit Worker(QObject *parent = nullptr);
        ~Worker();

        WSerialPort *getSerialPort() { return m_serialPort; }
        bool connect() { return m_serialPort->connect(); }
        bool flush() { return m_serialPort->flush(); }
        void clearError() { m_serialPort->clearError(); }
        void disconnect() { m_serialPort->disconnect(); }
        const QString errorString() { return m_serialPort->errorString(); }
        bool setDataTerminalReady(bool set) { return m_serialPort->setDataTerminalReady(set); }
        bool setRequestToSend(bool set) { return m_serialPort->setRequestToSend(set); }
        QSerialPort::PinoutSignals pinoutSignals() { return m_serialPort->pinoutSignals(); }
        qint32 baudRate(QSerialPort::Directions directions = QSerialPort::AllDirections) { return m_serialPort->baudRate(directions); }
        QSerialPort::DataBits dataBits() { return m_serialPort->dataBits(); }
        QSerialPort::StopBits stopBits() { return m_serialPort->stopBits(); }
        QSerialPort::FlowControl flowControl() { return m_serialPort->flowControl(); }

    private:
        WSerialPort *m_serialPort;

    protected:
        void run() override;

    public slots:
        void serialDataReady();
        void serialPortError(QSerialPort::SerialPortError);
        void writeData(QByteArray data);

    signals:
        void dataReady(QByteArray data);
        void serialError(QSerialPort::SerialPortError error);

};

#endif // WORKER_H
