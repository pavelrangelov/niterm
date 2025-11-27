#include "wserialport.h"
#include "worker.h"

//-----------------------------------------------------------------------------
Worker::Worker(QObject *parent) : QThread{parent} {
    m_serialPort = new WSerialPort(this);
    QObject::connect(m_serialPort, &QIODevice::readyRead, this, &Worker::serialDataReady);
    QObject::connect(m_serialPort, &QSerialPort::errorOccurred, this, &Worker::serialPortError);
}

///////////////////////////////////////////////////////////////////////////////
Worker::~Worker() {
    if (m_serialPort != NULL) {
        m_serialPort->close();
        delete m_serialPort;
        m_serialPort = NULL;
    }
}

///////////////////////////////////////////////////////////////////////////////
void Worker::serialDataReady() {
    QByteArray data = m_serialPort->readAll();
    emit dataReady(data);
}

///////////////////////////////////////////////////////////////////////////////
void Worker::serialPortError(QSerialPort::SerialPortError error) {
    emit serialError(error);
}

///////////////////////////////////////////////////////////////////////////////
void Worker::writeData(QByteArray data) {
    m_serialPort->write(data);
}

///////////////////////////////////////////////////////////////////////////////
void Worker::run() {
    msleep(1000);
}
