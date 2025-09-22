#include <QTextCursor>
#include <QDateTime>

#include "mainwindow.h"
#include "ui_mainwindow.h"

///////////////////////////////////////////////////////////////////////////////
void MainWindow::setAsciiData(QByteArray &data, QColor color) {
    QByteArray text;
    static bool crlf = false;

    for (int i=0; i<data.length(); i++) {
        QChar ch = data.at(i);

        if (m_timeStamp && crlf) {
            crlf = false;

            text += "[";
            text += QDateTime::currentDateTime().toString("hh:mm:ss.zzz").toLatin1();
            text += "]";

            appendText(text, COLOR_BLACK);

            if (text.length()) {
                appendText(text, color);
            }
        }

        uint8_t t = ch.toLatin1();

        if (t < 0x20) {
            if (text.length()) {
                appendText(text, color);
            }

            text += "<";
            text += WSerialPort::ASCII_table[(int)ch.toLatin1()].toLatin1();
            text += ">";

            if (ch == '\n') {
                text += '\r';
                crlf = true;
            }

            appendText(text, COLOR_GREEN);
        } else
        if (t <= 0x80) {
            text += t;
        } else {
            text += t;
        }

        m_previousChar = ch;
    }

    if (text.length()) {
        appendText(text, color);
    }
}

///////////////////////////////////////////////////////////////////////////////
void MainWindow::setHexData(QByteArray &data, QColor color) {
    QString text;

    QTextCharFormat format;
    format.setForeground(QBrush(color));
    ui->editOutputAscii->textCursor().setCharFormat(format);

    for (int i=0; i<data.length(); i++) {
        uint8_t t = (quint8)data.at(i);
        text += QString("%1 ").arg(t, 2, 16, QChar('0'));
    }

    ui->editOutputHex->moveCursor(QTextCursor::End);
    ui->editOutputHex->textCursor().insertText(text);
    ui->editOutputHex->moveCursor(QTextCursor::End);
}

///////////////////////////////////////////////////////////////////////////////
void MainWindow::appendText(QByteArray &text, const QColor color)
{
    QTextCharFormat format;
    format.setForeground(QBrush(color));

    ui->editOutputAscii->moveCursor(QTextCursor::End);
    QTextCursor textCursor = ui->editOutputAscii->textCursor();
    textCursor.setCharFormat(format);
    textCursor.insertText(text);
    ui->editOutputAscii->moveCursor(QTextCursor::End);
    text.clear();
}

///////////////////////////////////////////////////////////////////////////////
void MainWindow::setPinsCheckBoxes(bool state)
{
    ui->checkDSR->setChecked(state);
    ui->checkDCD->setChecked(state);
    ui->checkRNG->setChecked(state);
    ui->checkCTS->setChecked(state);
    ui->checkDTR->setChecked(state);
    ui->checkRTS->setChecked(state);
}

///////////////////////////////////////////////////////////////////////////////
void MainWindow::enablePinsCheckBoxes(bool enable)
{
    ui->checkDSR->setEnabled(enable);
    ui->checkDCD->setEnabled(enable);
    ui->checkRNG->setEnabled(enable);
    ui->checkCTS->setEnabled(enable);
    ui->checkDTR->setEnabled(enable);
    ui->checkRTS->setEnabled(enable);
}
