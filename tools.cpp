#include <QTextCursor>
#include <QDateTime>

#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "settings.h"

///////////////////////////////////////////////////////////////////////////////
void MainWindow::setAsciiData(QByteArray &data, QColor color)
{
    QString text;
    static bool crlf = false;

    for (int i=0; i<data.length(); i++)
    {
        QChar ch = data.at(i);

        if (m_timeStamp && crlf)
        {
            crlf = false;

            text += "[";
            text += QDateTime::currentDateTime().toString("hh:mm:ss.zzz");
            text += "]";

            appendText(text, COLOR_BLACK);

            if (text.length())
            {
                appendText(text, color);
            }
        }

        if (ch >= 0x20)
        {
            text += data.at(i);
        }
        else
        {
            if (text.length())
            {
                appendText(text, color);
            }

            text += "<";
            text += WSerialPort::ASCII_table[(int)ch.toLatin1()];
            text += ">";

            if (ch == '\n')
            {
                text += '\r';
                crlf = true;
            }

            appendText(text, COLOR_GREEN);
        }

        m_previousChar = ch;
    }

    if (text.length())
    {
        appendText(text, color);
    }
}

///////////////////////////////////////////////////////////////////////////////
void MainWindow::setHexData(QByteArray &data, QColor color)
{
    QString text;

    QTextCharFormat format;
    format.setForeground(QBrush(color));
    ui->editOutputAscii->textCursor().setCharFormat(format);

    for (int i=0; i<data.length(); i++)
    {
        text += QString("%1 ").arg((quint8)data.at(i), 2, 16, QChar('0'));
    }

    ui->editOutputHex->moveCursor(QTextCursor::End);
    ui->editOutputHex->textCursor().insertText(text);
    ui->editOutputHex->moveCursor(QTextCursor::End);
}

///////////////////////////////////////////////////////////////////////////////
void MainWindow::appendText(QString &text, const QColor color)
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
