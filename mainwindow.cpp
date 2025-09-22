#include <QSerialPort>
#include <QSettings>
#include <QMessageBox>
#include <QTextCodec>
#include <QFile>
#include <QFileDialog>
#include <QProgressDialog>

#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "settings.h"
#include "settingsdialog.h"
#include "macrosdialog.h"
#include "helpdialog.h"

const QString checkStyle =
"QCheckBox{spacing: 5px;}"
"QCheckBox::indicator{width: 13px; height: 13px;}"
"QCheckBox::indicator:unchecked{image: url(:/images/check-off.png);}"
"QCheckBox::indicator:checked{image: url(:/images/check-on.png);}"
"QCheckBox::indicator::disabled{image: url(:/images/check-disabled.png);}";

///////////////////////////////////////////////////////////////////////////////
MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent), ui(new Ui::MainWindow) {
    ui->setupUi(this);

    setWindowIcon(QIcon(":/images/main.png"));

    ui->tabOutput->setStyleSheet("QTabWidget::pane{border: 0;}");
    ui->tabOutput->setCurrentIndex(0);

    ui->splitter->setChildrenCollapsible(true);
    ui->splitter->setStretchFactor(0,3);
    ui->splitter->setStretchFactor(1,1);

    ui->checkDSR->setStyleSheet(checkStyle);
    ui->checkDCD->setStyleSheet(checkStyle);
    ui->checkRNG->setStyleSheet(checkStyle);
    ui->checkCTS->setStyleSheet(checkStyle);
    ui->checkDTR->setStyleSheet(checkStyle);
    ui->checkRTS->setStyleSheet(checkStyle);

    ui->checkDSR->setReadOnly(true);
    ui->checkDCD->setReadOnly(true);
    ui->checkRNG->setReadOnly(true);
    ui->checkCTS->setReadOnly(true);
    ui->checkDTR->setReadOnly(false);
    ui->checkRTS->setReadOnly(false);

    initVars();
    loadSettings();
    updateSettings();

    m_serialPort = new WSerialPort();

    //connect(m_serialPort, SIGNAL(readyRead()), this, SLOT(slot_serialDataReady()));
    connect(m_serialPort, SIGNAL(errorOccurred(QSerialPort::SerialPortError)), this, SLOT(slot_serialPortError(QSerialPort::SerialPortError)));
    connect(ui->editInput, SIGNAL(keyPressed(QString)), this, SLOT(slot_keyPressed(QString)));
    connect(this, SIGNAL(signal_dataReady(QByteArray&)), this, SLOT(slot_updateUI(QByteArray&)));

    m_pinoutsReadTimer = new QTimer(this);
    m_pinoutsReadTimer->setInterval(500);

    connect(m_pinoutsReadTimer, SIGNAL(timeout()), this, SLOT(slot_pinoutReadTimerTimeout()));

    setConnected(false);

    ui->editInput->setFocus();
}

///////////////////////////////////////////////////////////////////////////////
MainWindow::~MainWindow() {
    if (m_serialPort != NULL) {
        m_serialPort->close();
        delete m_serialPort;
        m_serialPort = NULL;
    }
    delete ui;
}

///////////////////////////////////////////////////////////////////////////////
void MainWindow::closeEvent(QCloseEvent* event) {
    QSettings settings;
    settings.setValue(STORE_GEOMETRY, saveGeometry());

    emit closed();
    event->accept();
}

///////////////////////////////////////////////////////////////////////////////
void MainWindow::on_action_Connect_triggered() {
    if (!m_bConnected) {
        if (m_serialPort->connect()) {
            ui->editOutputAscii->clear();
            ui->editOutputHex->clear();

            m_serialPort->flush();
            m_serialPort->clearError();

            setConnected(true);
            m_pinoutsReadTimer->start();
        } else {
            QMessageBox::critical(this, APP_NAME, tr("Failed to connect: %1").arg(m_serialPort->errorString()));
        }
    } else {
        m_serialPort->disconnect();
        m_pinoutsReadTimer->stop();
        setConnected(false);
    }
}

///////////////////////////////////////////////////////////////////////////////
void MainWindow::on_action_Settings_triggered() {
    m_serialPort->disconnect();
    setConnected(false);

    SettingsDialog dialog(this);

    if (dialog.exec() == QDialog::Accepted) {
        saveSettings();
        updateSettings();
    }
}

///////////////////////////////////////////////////////////////////////////////
void MainWindow::on_action_About_triggered() {
    HelpDialog dialog(this);
    dialog.exec();
}

///////////////////////////////////////////////////////////////////////////////
void MainWindow::on_btnOutputClear_clicked() {
    ui->editOutputAscii->clear();
    ui->editOutputHex->clear();
}

///////////////////////////////////////////////////////////////////////////////
void MainWindow::on_btnInputClear_clicked() {
    ui->editInput->clear();
}

///////////////////////////////////////////////////////////////////////////////
void MainWindow::on_btnMacros_clicked() {
    MacrosDialog *dialog = new MacrosDialog(this);
    dialog->setModal(false);

    connect(dialog, SIGNAL(signal_writeData(QByteArray&)), this, SLOT(slot_writeData(QByteArray&)));
    connect(this, SIGNAL(signal_dataReady(QByteArray&)), dialog, SLOT(slot_dataReady(QByteArray&)));
    connect(this, SIGNAL(closed()), dialog, SLOT(close()));
    connect(this, SIGNAL(signal_connectStatusChanged(bool)), dialog, SLOT(slot_connectStatusChanged(bool)));

    dialog->setConnectStatus(m_bConnected);
    dialog->show();
}

///////////////////////////////////////////////////////////////////////////////
void MainWindow::on_btnSendFile_clicked() {
    QFile file;
    char ch[1];
    QByteArray tmp(4,0);

    QString fileName = chooseSendFile();

    if (!m_bConnected) {
        return;
    }

    if (fileName.isEmpty()) {
        return;
    }

    file.setFileName(fileName);

    if (!file.open(QFile::ReadOnly)) {
        QMessageBox::critical(this, APP_NAME, tr("Failed to open file: %1").arg(fileName));
        return;
    }

    progress = new QProgressDialog(this);
    connect(progress, SIGNAL(canceled()), this, SLOT(slot_progressCanceled()));

    QByteArray data = file.readAll();

    file.close();

    progress->setRange(0, data.length());
    progress->show();

    m_bCanceled = false;

    const char *ptr = data.constData();

    for (int i=0; i<data.length(); i++) {
        ch[0] = ptr[i];

        m_serialPort->write(ch, 1);

        tmp.clear();
        tmp += (char)ch[0];
        setAsciiData(tmp, COLOR_BLUE);
        setHexData(tmp, COLOR_BLUE);

        delay(g_Settings.charDelay);

        if (i%100) {
            progress->setValue(i);
        }

        if (m_bCanceled) {
            break;
        }
    }

    progress->close();
    delete progress;
    progress = 0;
}

///////////////////////////////////////////////////////////////////////////////
void MainWindow::on_checkDTR_stateChanged(int state) {
    if (m_bConnected) {
        m_serialPort->setDataTerminalReady(state == Qt::Checked);
    }
}

///////////////////////////////////////////////////////////////////////////////
void MainWindow::on_checkRTS_stateChanged(int state) {
    if (m_bConnected) {
        m_serialPort->setRequestToSend(state == Qt::Checked);
    }
}

///////////////////////////////////////////////////////////////////////////////
void MainWindow::slot_writeData(QByteArray &data) {
    m_serialPort->write(data);
    setAsciiData(data, COLOR_BLUE);
    setHexData(data, COLOR_BLUE);
}

///////////////////////////////////////////////////////////////////////////////
void MainWindow::slot_serialDataReady() {
    QByteArray data = m_serialPort->readAll();
    emit signal_dataReady(data);
}

///////////////////////////////////////////////////////////////////////////////
void MainWindow::slot_updateUI(QByteArray &data) {
    setAsciiData(data, COLOR_RED);
    setHexData(data, COLOR_RED);
}

///////////////////////////////////////////////////////////////////////////////
void MainWindow::slot_keyPressed(QString text) {
    if (m_bConnected) {
        QByteArray ba = text.toLocal8Bit();
        slot_writeData(ba);
    }
}

///////////////////////////////////////////////////////////////////////////////
void MainWindow::slot_pinoutReadTimerTimeout() {
    if (m_bConnected) {
        m_bStartup = true;

        QSerialPort::PinoutSignals pins = m_serialPort->pinoutSignals();

        ui->checkDCD->setChecked(!!(pins & QSerialPort::DataCarrierDetectSignal));
        ui->checkDSR->setChecked(!!(pins & QSerialPort::DataSetReadySignal));
        ui->checkRNG->setChecked(!!(pins & QSerialPort::RingIndicatorSignal));
        ui->checkCTS->setChecked(!!(pins & QSerialPort::ClearToSendSignal));
        //ui->checkDTR->setChecked(!!(pins & QSerialPort::DataTerminalReadySignal));
        //ui->checkRTS->setChecked(!!(pins & QSerialPort::RequestToSendSignal));
    }
}

///////////////////////////////////////////////////////////////////////////////
void MainWindow::slot_progressCanceled() {
    m_bCanceled = true;
}

///////////////////////////////////////////////////////////////////////////////
void MainWindow::slot_serialPortError(QSerialPort::SerialPortError error) {
    if (m_bConnected && m_bStartup && error == QSerialPort::ResourceError) {
        m_pinoutsReadTimer->stop();
        m_serialPort->disconnect();
        setConnected(false);
    }
}

///////////////////////////////////////////////////////////////////////////////
void MainWindow::setConnected(bool connected) {
    QString title;

    if (connected) {
        m_bConnected = true;
        ui->action_Connect->setIcon(QIcon(":/images/48x48/disconnect.png"));

        if (g_Settings.flowControlIndex != 1) {
            enablePinsCheckBoxes(true);
            m_serialPort->setRequestToSend(false);
            m_serialPort->setDataTerminalReady(false);
        }

        ui->btnSendFile->setEnabled(true);

        title = APP_NAME;
        title += " - ";
        title += "Connected";
        title += ": ";
        title += g_Settings.comPort;
        title += "; ";
        title += QString("%1").arg(m_serialPort->BaudRatesArray[g_Settings.baudRateIndex]);
        title += "; ";
        title += QString("%1").arg(m_serialPort->DataBitsArray[g_Settings.dataBitsIndex]);
        title += "; ";
        title += QString("%1").arg(m_serialPort->StopBitsArray[g_Settings.stopBitsIndex]);
        title += "; ";

        switch (g_Settings.flowControlIndex) {
            case 0: title += "None"; break;
            case 1: title += "Hardware"; break;
            case 2: title += "Software"; break;
        }

        setWindowTitle(title);
    } else {
        m_bConnected = false;
        m_bStartup = false;

        ui->action_Connect->setIcon(QIcon(":/images/48x48/connect.png"));

        enablePinsCheckBoxes(false);
        ui->btnSendFile->setEnabled(false);

        title = APP_NAME;
        title += " - ";
        title += "Disconnected";

        setWindowTitle(title);
    }

    emit signal_connectStatusChanged(m_bConnected);
}

///////////////////////////////////////////////////////////////////////////////
void MainWindow::loadSettings() {
    QSettings settings;

    restoreGeometry(settings.value(STORE_GEOMETRY).toByteArray());

    #ifdef Q_OS_WIN
    g_Settings.comPort = settings.value(STORE_COMPORT, "COM1").toString();                          // 1
    #else
    g_Settings.comPort = settings.value(STORE_COMPORT, "ttyS0").toString();                         // 1
    #endif

    g_Settings.baudRateIndex    = settings.value(STORE_BAUDRATE,    3).toInt();                     // 2
    g_Settings.dataBitsIndex    = settings.value(STORE_DATABITS,    3).toInt();                     // 3
    g_Settings.stopBitsIndex    = settings.value(STORE_STOPBITS,    0).toInt();                     // 4
    g_Settings.parityIndex      = settings.value(STORE_PARITY,      0).toInt();                     // 5
    g_Settings.flowControlIndex = settings.value(STORE_FLOWCONTROL, 0).toInt();                     // 6
    g_Settings.charDelay        = settings.value(STORE_CHARDELAY,   0).toInt();                     // 7
    g_Settings.textEncoding     = settings.value(STORE_ENCODING,    "System").toString();           // 8
    #ifdef Q_OS_WIN
    g_Settings.fontName         = settings.value(STORE_FONTNAME,    "Courier New").toString();      // 9
    #else
    g_Settings.fontName         = settings.value(STORE_FONTNAME,    "Monospace").toString();        // 9
    #endif
    g_Settings.fontSize         = settings.value(STORE_FONTSIZE,    12).toInt();                    // 10
    g_Settings.comProtocol      = settings.value(STORE_MACROSPROTO, "None").toString();             // 11
    g_Settings.repeatAll        = settings.value(STORE_MACROSREPT,  false).toBool();                // 12
    g_Settings.timeStamp        = settings.value(STORE_TIMESTAMP,   "Disable").toString();          // 13

    if (g_Settings.timeStamp != "Disable" && g_Settings.timeStamp != "Enable") {
        g_Settings.timeStamp = "Disable";
    }

    QTextCodec::setCodecForLocale(QTextCodec::codecForName(g_Settings.textEncoding.toLatin1()));

    QFont font(g_Settings.fontName, g_Settings.fontSize);
    ui->editOutputAscii->setFont(font);
    ui->editOutputHex->setFont(font);
    ui->editInput->setFont(font);

    setPinsCheckBoxes(false);
    enablePinsCheckBoxes(false);
}

///////////////////////////////////////////////////////////////////////////////
void MainWindow::saveSettings() {
    QSettings settings;

    settings.setValue(STORE_COMPORT,        g_Settings.comPort);            // 1
    settings.setValue(STORE_BAUDRATE,       g_Settings.baudRateIndex);      // 2
    settings.setValue(STORE_DATABITS,       g_Settings.dataBitsIndex);      // 3
    settings.setValue(STORE_STOPBITS,       g_Settings.stopBitsIndex);      // 4
    settings.setValue(STORE_PARITY,         g_Settings.parityIndex);        // 5
    settings.setValue(STORE_FLOWCONTROL,    g_Settings.flowControlIndex);   // 6
    settings.setValue(STORE_CHARDELAY,      g_Settings.charDelay);          // 7
    settings.setValue(STORE_ENCODING,       g_Settings.textEncoding);       // 8
    settings.setValue(STORE_FONTNAME,       g_Settings.fontName);           // 9
    settings.setValue(STORE_FONTSIZE,       g_Settings.fontSize);           // 10
    settings.setValue(STORE_MACROSPROTO,    g_Settings.comProtocol);        // 11
    settings.setValue(STORE_MACROSREPT,     g_Settings.repeatAll);          // 12
    settings.setValue(STORE_TIMESTAMP,      g_Settings.timeStamp);          // 13

    QTextCodec::setCodecForLocale(QTextCodec::codecForName(g_Settings.textEncoding.toLatin1()));

    QFont font(g_Settings.fontName, g_Settings.fontSize);
    ui->editOutputAscii->setFont(font);
    ui->editOutputHex->setFont(font);
    ui->editInput->setFont(font);

    enablePinsCheckBoxes(false);
}

///////////////////////////////////////////////////////////////////////////////
void MainWindow::updateSettings() {
    if (g_Settings.timeStamp == "Disable") {
        m_timeStamp = false;
    } else {
        m_timeStamp = true;
    }

    QTextCodec::setCodecForLocale(QTextCodec::codecForName(g_Settings.textEncoding.toLatin1()));
    #if QT_VERSION < 0x050101
    QTextCodec::setCodecForCStrings(QTextCodec::codecForName(g_Settings.textEncoding.toLatin1()));
    QTextCodec::setCodecForTr(QTextCodec::codecForName(g_Settings.textEncoding.toLatin1()));
    #endif
}

///////////////////////////////////////////////////////////////////////////////
void MainWindow::initVars() {
    m_encodingList
        << "System"
        << "IBM 850"
        << "IBM 866"
        << "CP1250"
        << "CP1251"
        << "CP1252"
        << "CP1253"
        << "CP1254"
        << "CP1255"
        << "CP1256"
        << "CP1257"
        << "CP1258"
        << "ISO 8859-1"
        << "ISO 8859-2"
        << "ISO 8859-3"
        << "ISO 8859-4"
        << "ISO 8859-5"
        << "ISO 8859-6"
        << "ISO 8859-7"
        << "ISO 8859-8"
        << "ISO 8859-9"
        << "ISO 8859-10"
        << "ISO 8859-13"
        << "ISO 8859-14"
        << "ISO 8859-15"
        << "ISO 8859-16"
        << "UTF-8"
        << "UTF-16"
        << "UTF-32";

    m_previousChar = QChar(0);
    m_timeStamp = -1;
}

///////////////////////////////////////////////////////////////////////////////
QString MainWindow::chooseSendFile() {
    QSettings	settings;
    QStringList	fileNames;
    QString		fileFilters;
    QString		dirName;
    QString     fileName;

    fileName.clear();

    fileFilters = tr("Binary files (*.bin);;All files (*.*)");
    dirName = settings.value(STORE_SENDFILEDIR, "").toString();

    QFileDialog fileDialog(this, tr("Open File"), dirName, fileFilters);
    fileDialog.setFileMode(QFileDialog::ExistingFile);
    fileDialog.setAcceptMode(QFileDialog::AcceptOpen);

    if (fileDialog.exec() == QDialog::Accepted) {
        fileNames	= fileDialog.selectedFiles();
        fileName	= fileNames[0];
        QDir dir	= fileDialog.directory();

        settings.setValue(STORE_SENDFILEDIR, dir.absolutePath());
    }

    return fileName;
}

///////////////////////////////////////////////////////////////////////////////
void MainWindow::delay(qint64 milliseconds) {
    qint64 timeToExit = QDateTime::currentMSecsSinceEpoch() + milliseconds;
    while (timeToExit > QDateTime::currentMSecsSinceEpoch()) {
        QApplication::processEvents(QEventLoop::AllEvents, 100);
    }
}
