#include <QPushButton>

#include "macrosdialog.h"
#include "ui_macrosdialog.h"
#include "settings.h"
#include "wserialport.h"
#include "mainwindow.h"

#define ERR_NO              0
#define ERR_COMM            1
#define ERR_PARSE_FRAME     2
#define ERR_PARSE_LENGTH    3
#define ERR_PARSE_CSUM      4

///////////////////////////////////////////////////////////////////////////////
QByteArray MacrosDialog::makeEcrProto(quint8 command, QByteArray data) {
    QByteArray arr;
    QByteArray tmp;
    quint16 len;

    arr.clear();
    tmp.clear();

    tmp += '\x00'; // len
    tmp += getSeqNum();
    tmp += command;
    tmp += data;
    tmp += ASCII_ENQ;

    len = (quint16)tmp.length() + 0x20;

    if (len > 0xff) {
        return arr;
    }

    tmp.data()[0] += (quint8)len;

    arr += ASCII_SOH;
    arr += tmp;
    arr += checkSum(tmp);
    arr += ASCII_ETX;

    return arr;
}

///////////////////////////////////////////////////////////////////////////////
quint8 MacrosDialog::getSeqNum() {
    if (++m_SeqNum < 0x20) {
        m_SeqNum = 0x20;
    }
    return m_SeqNum;
}

///////////////////////////////////////////////////////////////////////////////
QByteArray MacrosDialog::checkSum(const QByteArray data) {
    char buff[10];
    const char* ptr;
    int i;
    unsigned short sum;
    QByteArray arr;

    ptr = data.data();

    for (i=0,sum=0; i<data.length(); i++) {
        sum += (unsigned char)ptr[i];
    }

    sprintf(buff, "%04X", sum);

    // 'A'->0x3A, 'B'->0x3B, ...

    for (i=0; i<4; i++) {
        if (buff[i] > '9') {
            buff[i] -= 0x07;
        }
    }

    arr = buff;

    return arr;
}

///////////////////////////////////////////////////////////////////////////////
void MacrosDialog::startEcrCom(int row) {
    quint8 command = convertCommand(ui->table->item(row, COL_MACRO_COMMAND)->text().toLatin1());
    QByteArray data = convertData(ui->table->item(row, COL_MACRO_DATA)->text().toLocal8Bit());

    ui->table->item(row, COL_MACRO_RESPONSE)->setText("");

    QByteArray temp = makeEcrProto(command, data);

    emit signal_StartTimer(ECR_SYN_TOUT);
    m_EcrRun = true;
    m_EcrIdx = 0;
    m_EcrResult = ERR_COMM;
    m_Data.clear();

    if (!temp.isEmpty()) {
        emit signal_writeData(temp);
    }

    for (int row=0; row<ui->table->rowCount(); row++) {
        QPushButton *button = qobject_cast<QPushButton*>(ui->table->cellWidget(row, COL_MACRO_BUTTON));
        button->setEnabled(false);
    }

    while (m_EcrRun == true && m_Cancel == false) {
        qApp->processEvents();
    }
}

///////////////////////////////////////////////////////////////////////////////
void MacrosDialog::stopEcrCom() {
    if (m_EcrResult != ERR_NO) {
        ui->table->item(m_SenderRow, COL_MACRO_RESPONSE)->setText(getErrorText(m_EcrResult));
    }

    m_Data.clear();
    m_EcrRun = false;

    for (int row=0; row<ui->table->rowCount(); row++) {
        QPushButton *button = qobject_cast<QPushButton*>(ui->table->cellWidget(row, COL_MACRO_BUTTON));
        button->setEnabled(true);
    }
}

///////////////////////////////////////////////////////////////////////////////
bool MacrosDialog::parseEcrFrame(QByteArray data) {
    int beg;
    int end;

    beg = data.indexOf(ASCII_SOH);
    if (beg == -1) {
        return false;
    }

    end = data.indexOf(ASCII_ETX);
    if (end == -1) {
        return false;
    }

    if (end <= beg) {
        return false;
    }

    return true;
}

///////////////////////////////////////////////////////////////////////////////
int MacrosDialog::parseEcrAnswer(QByteArray data) {
    m_EcrAnswer.clear();

    int beg = data.indexOf(ASCII_SOH);
    int end = data.indexOf(ASCII_ETX);

    data.remove(0, beg+1);
    data.truncate(end-1);

    QByteArray csum = data.right(4);
    data.chop(4);

    if (!data.endsWith(ASCII_ENQ)) {
        return ERR_PARSE_FRAME;
    }

    quint8 len = (quint8)data.at(0) - 0x20;

    if ((quint8)data.length() != len || len < 10) {
        return ERR_PARSE_LENGTH;
    }

    if (csum != checkSum(data)) {
        return ERR_PARSE_CSUM;
    }

    data.chop(1); // ENQ

    QByteArray status = data.right(6);
    data.chop(6);

    if (!data.endsWith(ASCII_EOT)) {
        return ERR_PARSE_FRAME;
    }

    data.chop(1);
    data.remove(0,3); // LEN, SEQ, CMD

    m_EcrAnswer = data;
    m_EcrAnswer += " [";
    m_EcrAnswer += status.toHex();
    m_EcrAnswer += "]";

    return 0;
}

///////////////////////////////////////////////////////////////////////////////
QString MacrosDialog::getErrorText(int error) {
    QString errorText = "Unknown Error";

    switch (error) {
        case ERR_NO:            errorText = "OK"; break;
        case ERR_COMM:          errorText = "Communication Error"; break;
        case ERR_PARSE_FRAME:   errorText = "Frame Error"; break;
        case ERR_PARSE_LENGTH:  errorText = "Length Error"; break;
        case ERR_PARSE_CSUM:    errorText = "Checksum Error"; break;
    }

    return errorText;
}
