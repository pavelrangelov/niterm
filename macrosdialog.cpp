#include <QTableWidget>
#include <QTableWidgetItem>
#include <QStringList>
#include <QPushButton>
#include <QVariant>
#include <QFileDialog>
#include <QSettings>
#include <QFont>
#include <QMessageBox>
#include <QTime>
#include <QRandomGenerator>

#include "macrosdialog.h"
#include "ui_macrosdialog.h"
#include "settings.h"
#include "mainwindow.h"

#define MACRO_PROTO_NONE    "None"
#define MACRO_PROTO_RESP    "Auto Response"
#define MACRO_PROTO_CREG    "Cash Register"

///////////////////////////////////////////////////////////////////////////////
MacrosDialog::MacrosDialog(QWidget *parent) : QDialog(parent), ui(new Ui::MacrosDialog) {
    QSettings settings;

    ui->setupUi(this);

    wsp = ((MainWindow*)parent)->getSerialDevice();

    ui->table->setHorizontalHeaderLabels(QStringList() << tr("") << tr("Command") << tr("Data") << tr("Response") << tr("Comment") );

    ui->table->horizontalHeader()->setSectionResizeMode(COL_MACRO_BUTTON,   QHeaderView::Fixed);
    ui->table->horizontalHeader()->setSectionResizeMode(COL_MACRO_COMMAND,  QHeaderView::Interactive);
    ui->table->horizontalHeader()->setSectionResizeMode(COL_MACRO_DATA,     QHeaderView::Interactive);
    ui->table->horizontalHeader()->setSectionResizeMode(COL_MACRO_RESPONSE, QHeaderView::Interactive);
    ui->table->horizontalHeader()->setSectionResizeMode(COL_MACRO_COMMENT,  QHeaderView::Stretch);

    ui->table->setColumnWidth(COL_MACRO_BUTTON,     50);
    ui->table->setColumnWidth(COL_MACRO_COMMAND,    100);
    ui->table->setColumnWidth(COL_MACRO_DATA,       200);
    ui->table->setColumnWidth(COL_MACRO_RESPONSE,   200);
    ui->table->setColumnWidth(COL_MACRO_COMMENT,    100);

    restoreGeometry(settings.value(STORE_MACROSGEO).toByteArray());

    ui->comboProto->addItems(QStringList() << MACRO_PROTO_NONE << MACRO_PROTO_RESP << MACRO_PROTO_CREG);

    ui->comboProto->setCurrentText(g_Settings.comProtocol);
    ui->checkRepeat->setChecked(g_Settings.repeatAll);

    QFont font(g_Settings.fontName, g_Settings.fontSize);
    ui->table->setFont(font);

    QRandomGenerator rg(QTime::currentTime().msecsSinceStartOfDay());
    m_SeqNum = (quint8)(rg.generate() % (0xff-0x20) + 0x20);

    m_RowId = 0;
    m_SenderRow = -1;
    m_EcrRun = false;
    m_EcrIdx = 0;
    m_Cancel = false;
    m_Connected = false;

    QObject::connect(ui->btnSave, &QToolButton::clicked, this, &MacrosDialog::save);
    QObject::connect(ui->btnOpen, &QToolButton::clicked, this, &MacrosDialog::open);
    QObject::connect(ui->btnInsertAbove, &QToolButton::clicked, this, &MacrosDialog::insertAbove);
    QObject::connect(ui->btnInsertBellow, &QToolButton::clicked, this, &MacrosDialog::insertBellow);
    QObject::connect(ui->btnRemove, &QToolButton::clicked, this, &MacrosDialog::remove);
    QObject::connect(ui->comboProto, &QComboBox::currentIndexChanged, this, &MacrosDialog::setProtocolMode);
    QObject::connect(ui->btnSendAll, &QToolButton::clicked, this, &MacrosDialog::sendAll);

    ui->spinDelay->setValue(0);
    setProtocolMode(0);

    QObject::connect(this, &MacrosDialog::cancel, this, &MacrosDialog::cancelSending);

    m_SynTimer = new TimerThread;
    QObject::connect(m_SynTimer, &TimerThread::timeout, this, &MacrosDialog::synTout);
    QObject::connect(this, &MacrosDialog::startThreadTimer, m_SynTimer, &TimerThread::startTimer);
    QObject::connect(this, &MacrosDialog::stopThreadTimer, m_SynTimer, &TimerThread::stopTimer);
    m_SynTimer->start();
}

///////////////////////////////////////////////////////////////////////////////
MacrosDialog::~MacrosDialog() {
    if (m_SynTimer != NULL) {
        m_SynTimer->terminateThread();

        while (!m_SynTimer->isFinished()) {
            qApp->processEvents();
        }
    }

    delete ui;
}

///////////////////////////////////////////////////////////////////////////////
void MacrosDialog::resizeEvent(QResizeEvent *event) {
    Q_UNUSED(event);
}

///////////////////////////////////////////////////////////////////////////////
void MacrosDialog::closeEvent(QCloseEvent* event) {
    QSettings settings;

    emit cancel();

    g_Settings.comProtocol = ui->comboProto->currentText();
    g_Settings.repeatAll = ui->checkRepeat->isChecked();

    settings.setValue(STORE_MACROSGEO, saveGeometry());

    settings.setValue(STORE_MACROSPROTO, g_Settings.comProtocol);
    settings.setValue(STORE_MACROSREPT, g_Settings.repeatAll);

    event->accept();
}

///////////////////////////////////////////////////////////////////////////////
void MacrosDialog::setProtocolMode(int index) {
    Q_UNUSED(index);

    if (ui->comboProto->currentText() == MACRO_PROTO_CREG || ui->comboProto->currentText() == MACRO_PROTO_RESP) {
        ui->table->showColumn(COL_MACRO_COMMAND);
        ui->table->showColumn(COL_MACRO_RESPONSE);
    } else {
        ui->table->hideColumn(COL_MACRO_COMMAND);
        ui->table->hideColumn(COL_MACRO_RESPONSE);
    }
}

///////////////////////////////////////////////////////////////////////////////
void MacrosDialog::insertAbove() {
    int row = ui->table->currentRow();

    if (row == -1 || row == 0) {
        row = 0;
    }

    insertTableRow(row);
}

///////////////////////////////////////////////////////////////////////////////
void MacrosDialog::insertBellow() {
    int row = ui->table->currentRow();

    if (row == -1) {
        row = 0;
    } else {
        row ++;
    }

    insertTableRow(row);
}

///////////////////////////////////////////////////////////////////////////////
void MacrosDialog::save() {
    QString fileName = chooseSaveFile();

    if (!fileName.isEmpty()) {
        saveDocument(fileName);
    }
}

///////////////////////////////////////////////////////////////////////////////
void MacrosDialog::open() {
    QString fileName = chooseOpenFile();

    if (!fileName.isEmpty()) {
        loadDocument(fileName);
    }
}

///////////////////////////////////////////////////////////////////////////////
void MacrosDialog::remove() {
    int row = getSelectedRow();

    if (row == -1) {
        QMessageBox::information(this, APP_NAME, tr("No row is selected."));
        return;
    }

    ui->table->removeRow(row);
}

///////////////////////////////////////////////////////////////////////////////
void MacrosDialog::sendCurrentRow() {
    QPushButton *button = qobject_cast<QPushButton*>(sender());

    if (!m_Connected) {
        QMessageBox::information(this, APP_NAME, tr("Not connected."));
        return;
    }

    if (!button) {
        return;
    }

    QVariant var = button->property("id");

    if (!var.isValid()) {
        return;
    }

    m_SenderRow = getRowByProperty(var.toInt());

    if (ui->comboProto->currentText() == MACRO_PROTO_NONE || ui->comboProto->currentText() == MACRO_PROTO_RESP) {
        QByteArray data = convertData(ui->table->item(m_SenderRow, COL_MACRO_DATA)->text().toLocal8Bit());
        emit writeData(data);
    } else
    if (ui->comboProto->currentText() == MACRO_PROTO_CREG) {
        startEcrCom(m_SenderRow);
    }
}

///////////////////////////////////////////////////////////////////////////////
void MacrosDialog::sendAll() {
    if (!m_Connected) {
        QMessageBox::information(this, APP_NAME, tr("Not connected."));
        return;
    }

    int t = ui->spinDelay->value();
    if (t < 20) {
        t = 20;
    }

    m_Cancel = false;

    do {
        for (int row=0; row<ui->table->rowCount(); row++) {
            if (ui->comboProto->currentText() == MACRO_PROTO_NONE || ui->comboProto->currentText() == MACRO_PROTO_RESP) {
                QByteArray data = convertData(ui->table->item(row, COL_MACRO_DATA)->text().toLocal8Bit());
                emit writeData(data);
            } else
            if (ui->comboProto->currentText() == MACRO_PROTO_CREG) {
                m_SenderRow = row;
                startEcrCom(m_SenderRow);
            }
            if (m_Cancel) {
                break;
            }
            delay(t);
        }
    } while (ui->checkRepeat->isChecked() && !m_Cancel);
}

///////////////////////////////////////////////////////////////////////////////
void MacrosDialog::synTout() {
    stopEcrCom();
}

///////////////////////////////////////////////////////////////////////////////
int MacrosDialog::getRowByProperty(int value) {
    bool ok;

    for (int row=0; row<ui->table->rowCount(); row++) {
        QPushButton *button = qobject_cast<QPushButton*>(ui->table->cellWidget(row, COL_MACRO_BUTTON));

        QVariant var = button->property("id");

        if (!var.isValid()) {
            return 0;
        }

        int id = var.toInt(&ok);

        if (!ok) {
            return 0;
        }

        if (id == value) {
            return row;
        }
    }

    return 0;
}

///////////////////////////////////////////////////////////////////////////////
int MacrosDialog::insertTableRow(int row) {
    if (row == -1) {
        return 0;
    }

    ui->table->insertRow(row);

    QPushButton *button = new QPushButton(this);
    button->setIcon(QIcon(":/images/play.png"));
    button->setIconSize(QSize(32,32));
    button->setProperty("id", ++m_RowId);
    QObject::connect(button, &QPushButton::clicked, this, &MacrosDialog::sendCurrentRow);

    ui->table->setItem(row, COL_MACRO_COMMAND,  new QTableWidgetItem(""));
    ui->table->setItem(row, COL_MACRO_DATA,     new QTableWidgetItem(""));
    ui->table->setItem(row, COL_MACRO_RESPONSE, new QTableWidgetItem(""));
    ui->table->setItem(row, COL_MACRO_COMMENT,  new QTableWidgetItem(""));

    ui->table->setCellWidget(row, COL_MACRO_BUTTON, button);
    //ui->table->item(row, COL_MACRO_RESPONSE)->setFlags(Qt::ItemIsEnabled|Qt::ItemIsSelectable);

    return row;
}

///////////////////////////////////////////////////////////////////////////////
int MacrosDialog::getSelectedRow() {
    for (int row=0; row<ui->table->rowCount(); row++) {
        for (int col=1; col<COL_MACRO_LAST; col++) {
            if (ui->table->item(row,col)->isSelected()) {
                return row;
            }
        }
    }

    return -1;
}

///////////////////////////////////////////////////////////////////////////////
QString MacrosDialog::chooseSaveFile() {
    QSettings	settings;
    QStringList	fileNames;
    QString		fileFilters;
    QString		dirName;
    QString     fileName;

    fileName.clear();

    fileFilters = tr("XML files (*.xml);;All files (*.*)");
    dirName = settings.value(STORE_MACROSDIR, "").toString();

    QFileDialog fileDialog(this, tr("Save Macros"), dirName, fileFilters);
    fileDialog.setFileMode(QFileDialog::AnyFile);
    fileDialog.setAcceptMode(QFileDialog::AcceptSave);

    if (fileDialog.exec() == QDialog::Accepted) {
        fileNames	= fileDialog.selectedFiles();
        fileName	= fileNames[0];
        QDir dir	= fileDialog.directory();

        settings.setValue(STORE_MACROSDIR, dir.absolutePath());

        int pos = fileName.indexOf('.');

        if (pos == -1) {
            fileName += ".xml";
        }
    }

    return fileName;
}

///////////////////////////////////////////////////////////////////////////////
QString MacrosDialog::chooseOpenFile() {
    QSettings	settings;
    QStringList	fileNames;
    QString		fileFilters;
    QString		dirName;
    QString     fileName;

    fileName.clear();

    fileFilters = tr("XML files (*.xml);;All files (*.*)");
    dirName = settings.value(STORE_MACROSDIR, "").toString();

    QFileDialog fileDialog(this, tr("Open File"), dirName, fileFilters);
    fileDialog.setFileMode(QFileDialog::ExistingFile);
    fileDialog.setAcceptMode(QFileDialog::AcceptOpen);

    if (fileDialog.exec() == QDialog::Accepted) {
        fileNames	= fileDialog.selectedFiles();
        fileName	= fileNames[0];
        QDir dir	= fileDialog.directory();

        settings.setValue(STORE_MACROSDIR, dir.absolutePath());
    }

    return fileName;
}

///////////////////////////////////////////////////////////////////////////////
quint8 MacrosDialog::convertCommand(QByteArray data)
{
    quint8 cmd = 0;
    bool ok;

    data = data.trimmed();

    if (data[0] == '0' && (data[1] == 'x' || data[1] == 'X')) {
        data = data.remove(0,2);
        cmd = data.toInt(&ok, 16);
    } else {
        cmd = data.toInt(&ok, 10);
    }

    return cmd;
}

///////////////////////////////////////////////////////////////////////////////
QByteArray MacrosDialog::convertData(QByteArray data) {
    QByteArray ba;
    quint8 c, d;
    bool ok;

    while (!data.isEmpty()) {
        c = data.at(0);
        data.remove(0,1);

        if (c == '\\') {
            if (!data.isEmpty())
            {
                c = data.at(0);
                data.remove(0,1);

                switch (c) {
                    case 'r':
                        ba += '\r';
                        break;
                    case 'n':
                        ba += '\n';
                        break;
                    case 't':
                        ba += '\t';
                        break;
                    case 'x':
                        if (data.length() >= 2)
                        {
                            d = data.left(2).toUShort(&ok,16);
                            data.remove(0,2);
                            ba += d;
                        }
                        else
                        {
                            ba += '\\';
                            ba += c;
                        }
                        break;
                    default :
                        ba += '\\';
                        ba += c;
                        break;
                }
            } else {
                ba += c;
            }
        } else {
            ba += c;
        }
    }

    ba = convertFunction(ba);

    return ba;
}

///////////////////////////////////////////////////////////////////////////////
QByteArray MacrosDialog::convertFunction(QByteArray data) {
    QByteArray tmp = data.toUpper();

    int i1 = tmp.indexOf("#XOR",0);

    if (i1 != -1) {
        int i2 = tmp.indexOf('(',i1);

        if (i2 != -1) {
            int i3 = tmp.indexOf(')',i2);

            if (i3 != -1) {
                QByteArray token = removeSpaces(tmp.mid(i1, i3-i1+1));
                token.remove(0,4);

                if (checkSyntax(token)) {
                    token.remove(0,1);
                    token.chop(1);

                    int i4 = token.indexOf('-');

                    if (i4 != -1) {
                        QByteArray tmp1 = token.left(i4);
                        QByteArray tmp2 = token.remove(0,i4+1);

                        bool ok = true;

                        int from = tmp1.toInt(&ok);

                        if (ok) {
                            int to = tmp2.toInt(&ok);

                            if (ok) {
                                if (from < to && to < tmp.length()) {
                                    quint8 sum = 0;

                                    for (int i=from; i<=to; i++) {
                                        sum ^= (quint8)data.at(i);
                                    }

                                    tmp1.clear();
                                    tmp1.append(sum);

                                    data.remove(i1, i3-i1+1);
                                    data.insert(i1, tmp1);
                                }
                            }
                        }
                    }
                }
            }
        }
    }

    return data;
}

///////////////////////////////////////////////////////////////////////////////
void MacrosDialog::hasDataReady(QByteArray data) {
    for (int i=0; i<data.length(); i++) {
        if (data.at(i) != ASCII_SYN) {
            m_Data += data.at(i);
        }
    }

    if (ui->comboProto->currentText() == MACRO_PROTO_RESP) {
        for (int row=0; row<ui->table->rowCount(); row++) {
            QString cmd = ui->table->item(row, COL_MACRO_COMMAND)->text();

            if (cmd.toUpper() == "#ON_RECEIVED") {
                QByteArray onrecv = convertData(ui->table->item(row, COL_MACRO_DATA)->text().toLocal8Bit());
                QByteArray onsend = convertData(ui->table->item(row, COL_MACRO_RESPONSE)->text().toLocal8Bit());

                if (m_Data.indexOf(onrecv) != -1) {
                    emit writeData(onsend);
                    m_Data.clear();
                }
            }
        }
    } else
    if (ui->comboProto->currentText() == MACRO_PROTO_CREG) {
        emit stopThreadTimer();

        if (parseEcrFrame(m_Data)) {
            m_EcrResult = parseEcrAnswer(m_Data);
            stopEcrCom();

            if (m_EcrResult == 0) {
                ui->table->item(m_SenderRow, COL_MACRO_RESPONSE)->setText(m_EcrAnswer);
            } else {
                ui->table->item(m_SenderRow, COL_MACRO_RESPONSE)->setText(getErrorText(m_EcrResult));
            }
            return;
        }

        emit startThreadTimer(ECR_SYN_TOUT);
    }
}

///////////////////////////////////////////////////////////////////////////////
void MacrosDialog::cancelSending() {
    m_Cancel = true;
}

///////////////////////////////////////////////////////////////////////////////
void MacrosDialog::connectStatusHasChanged(bool connected) {
    m_Connected = connected;
}

///////////////////////////////////////////////////////////////////////////////
void MacrosDialog::setConnectStatus(bool connected) {
    m_Connected = connected;
}

/*
+--------+-------+-------+-------+-------+-------+
|        | Digit |   +   |   -   |   (   |   )   |
+--------+-------+-------+-------+-------+-------+
| Digit  |   1   |   1   |   1   |   0   |   1   |
+--------+---------------------------------------+
|   +    |   1   |   0   |   0   |   1   |   0   |
+--------+---------------------------------------+
|   -    |   1   |   0   |   0   |   1   |   0   |
+--------+---------------------------------------+
|   (    |   1   |   1   |   1   |   0   |   0   |
+--------+---------------------------------------+
|   )    |   0   |   1   |   1   |   0   |   0   |
+--------+---------------------------------------+
*/
const quint8 MacrosDialog::syntaxArray[5][5] = {
    {1,1,1,0,1},
    {1,0,0,1,0},
    {1,0,0,1,0},
    {1,1,1,0,0},
    {0,1,1,0,0}
};

#define IS_DIGIT        0
#define IS_PLUS         1
#define IS_MINUS        2
#define IS_LEFTBRACE    3
#define IS_RIGHTBRACE   4

///////////////////////////////////////////////////////////////////////////////
bool MacrosDialog::checkSyntax(QByteArray data) {
    int len = data.length();

    for (int i=0; i<len-1; i++) {
        int curr = getIndexByCharType(data.at(i));
        int next = getIndexByCharType(data.at(i+1));

        if (curr == -1 || next == -1) {
            return false;
        }

        if (syntaxArray[curr][next] == 0) {
            return false;
        }
    }

    return true;
}

///////////////////////////////////////////////////////////////////////////////
int MacrosDialog::getIndexByCharType(QChar ch) {
    if (ch.isDigit())  return IS_DIGIT;
    if (ch.isLetter()) return (-1);
    if (ch == '+')     return IS_PLUS;
    if (ch == '-')     return IS_MINUS;
    if (ch == '(')     return IS_LEFTBRACE;
    if (ch == ')')     return IS_RIGHTBRACE;

    return (-1);
}

///////////////////////////////////////////////////////////////////////////////
QByteArray MacrosDialog::removeSpaces(QByteArray data) {
    QByteArray ba;

    for (int i=0; i<data.length(); i++) {
        quint8 ch = (quint8)data.at(i);

        if (ch != '\x20') {
            ba.append(ch);
        }
    }

    return ba;
}

///////////////////////////////////////////////////////////////////////////////
void MacrosDialog::delay(qint64 milliseconds) {
    qint64 timeToExit = QDateTime::currentMSecsSinceEpoch() + milliseconds;
    while (timeToExit > QDateTime::currentMSecsSinceEpoch()) {
        QApplication::processEvents(QEventLoop::AllEvents);
    }
}
