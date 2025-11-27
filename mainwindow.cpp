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

static const QString checkStyle =
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

    QObject::connect(ui->action_Connect, &QAction::triggered, this, &MainWindow::doConnect);
    QObject::connect(ui->action_Settings, &QAction::triggered, this, &MainWindow::setSettings);
    QObject::connect(ui->action_About, &QAction::triggered, this, &MainWindow::doAbout);
    QObject::connect(ui->btnOutputClear, &QPushButton::clicked, this, &MainWindow::clearOutput);
    QObject::connect(ui->btnInputClear, &QPushButton::clicked, this, &MainWindow::clearInput);
    QObject::connect(ui->btnMacros, &QPushButton::clicked, this, &MainWindow::doMacros);
    QObject::connect(ui->btnSendFile, &QPushButton::clicked, this, &MainWindow::sendFile);
    QObject::connect(ui->checkDTR, &QCheckBox::checkStateChanged, this, &MainWindow::setDTR);
    QObject::connect(ui->checkRTS, &QCheckBox::checkStateChanged, this, &MainWindow::setRTS);

    QObject::connect(ui->editInput, &WTextEdit::keyPressed, this, &MainWindow::keyPressed);

    m_pinoutsReadTimer = new QTimer(this);
    m_pinoutsReadTimer->setInterval(500);

    QObject::connect(m_pinoutsReadTimer, &QTimer::timeout, this, &MainWindow::pinoutReadTimerTimeout);

    setConnected(false);

    ui->editInput->setFocus();

    m_worker = new Worker;
    QObject::connect(this, &MainWindow::writeData, m_worker, &Worker::writeData);
    QObject::connect(m_worker, &Worker::dataReady, this, &MainWindow::outputReceivedData);
    QObject::connect(m_worker, &Worker::serialError, this, &MainWindow::serialPortError);
    m_worker->start(QThread::HighPriority);
}

///////////////////////////////////////////////////////////////////////////////
MainWindow::~MainWindow() {
    m_worker->quit();
    m_worker->wait();

    if (m_worker) {
        delete m_worker;
    }

    delete ui;
}

///////////////////////////////////////////////////////////////////////////////
void MainWindow::closeEvent(QCloseEvent* event) {
    QSettings settings;
    settings.setValue(STORE_GEOMETRY, saveGeometry());

    emit windowClosed();
    event->accept();
}

///////////////////////////////////////////////////////////////////////////////
void MainWindow::doConnect() {
    if (!m_bConnected) {
        if (m_worker->connect()) {
            ui->editOutputAscii->clear();
            ui->editOutputHex->clear();

            m_worker->flush();
            m_worker->clearError();

            setConnected(true);
            m_pinoutsReadTimer->start();
        } else {
            QMessageBox::critical(this, APP_NAME, tr("Failed to connect: %1").arg(m_worker->errorString()));
        }
    } else {
        m_worker->disconnect();
        m_pinoutsReadTimer->stop();
        setConnected(false);
    }
}

///////////////////////////////////////////////////////////////////////////////
void MainWindow::setSettings() {
    m_worker->disconnect();
    setConnected(false);

    SettingsDialog dialog(this);

    if (dialog.exec() == QDialog::Accepted) {
        saveSettings();
        updateSettings();
    }
}

///////////////////////////////////////////////////////////////////////////////
void MainWindow::doAbout() {
    HelpDialog dialog(this);
    dialog.exec();
}

///////////////////////////////////////////////////////////////////////////////
void MainWindow::clearOutput() {
    ui->editOutputAscii->clear();
    ui->editOutputHex->clear();
}

///////////////////////////////////////////////////////////////////////////////
void MainWindow::clearInput() {
    ui->editInput->clear();
}

///////////////////////////////////////////////////////////////////////////////
void MainWindow::doMacros() {
    MacrosDialog *dialog = new MacrosDialog(this);
    dialog->setModal(false);

    QObject::connect(dialog, &MacrosDialog::writeData, this, &MainWindow::outputTransmitedData);
    QObject::connect(m_worker, &Worker::dataReady, dialog, &MacrosDialog::hasDataReady);
    QObject::connect(this, &MainWindow::windowClosed, dialog, &MacrosDialog::close);
    QObject::connect(this, &MainWindow::connectStatusChanged, dialog, &MacrosDialog::connectStatusHasChanged);

    dialog->setConnectStatus(m_bConnected);
    dialog->show();
}

///////////////////////////////////////////////////////////////////////////////
void MainWindow::sendFile() {
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
    QObject::connect(progress, &QProgressDialog::canceled, this, &MainWindow::progressCanceled);

    QByteArray data = file.readAll();

    file.close();

    progress->setRange(0, data.length());
    progress->show();

    m_bCanceled = false;

    const char *ptr = data.constData();

    for (int i=0; i<data.length(); i++) {
        ch[0] = ptr[i];

        QByteArray data = ch;
        emit writeData(data);
        qApp->processEvents();

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
void MainWindow::setDTR(int state) {
    if (m_bConnected) {
        m_worker->setDataTerminalReady(state == Qt::Checked);
    }
}

///////////////////////////////////////////////////////////////////////////////
void MainWindow::setRTS(int state) {
    if (m_bConnected) {
        m_worker->setRequestToSend(state == Qt::Checked);
    }
}

///////////////////////////////////////////////////////////////////////////////
void MainWindow::outputTransmitedData(QByteArray data) {
    setAsciiData(data, COLOR_BLUE);
    setHexData(data, COLOR_BLUE);
    emit writeData(data);
}

///////////////////////////////////////////////////////////////////////////////
void MainWindow::serialPortError(QSerialPort::SerialPortError error) {
    if (m_bConnected && m_bStartup && error == QSerialPort::ResourceError) {
        m_pinoutsReadTimer->stop();
        m_worker->disconnect();
        setConnected(false);
    }
}

///////////////////////////////////////////////////////////////////////////////
void MainWindow::outputReceivedData(QByteArray data) {
    setAsciiData(data, COLOR_RED);
    setHexData(data, COLOR_RED);
}

///////////////////////////////////////////////////////////////////////////////
void MainWindow::keyPressed(QString text) {
    if (m_bConnected) {
        QByteArray data = text.toLocal8Bit();
        outputTransmitedData(data);
    }
}

///////////////////////////////////////////////////////////////////////////////
void MainWindow::pinoutReadTimerTimeout() {
    if (m_bConnected) {
        m_bStartup = true;

        QSerialPort::PinoutSignals pins = m_worker->pinoutSignals();

        ui->checkDCD->setChecked(!!(pins & QSerialPort::DataCarrierDetectSignal));
        ui->checkDSR->setChecked(!!(pins & QSerialPort::DataSetReadySignal));
        ui->checkRNG->setChecked(!!(pins & QSerialPort::RingIndicatorSignal));
        ui->checkCTS->setChecked(!!(pins & QSerialPort::ClearToSendSignal));
        //ui->checkDTR->setChecked(!!(pins & QSerialPort::DataTerminalReadySignal));
        //ui->checkRTS->setChecked(!!(pins & QSerialPort::RequestToSendSignal));
    }
}

///////////////////////////////////////////////////////////////////////////////
void MainWindow::progressCanceled() {
    m_bCanceled = true;
}

///////////////////////////////////////////////////////////////////////////////
void MainWindow::setConnected(bool connected) {
    QString title;

    if (connected) {
        m_bConnected = true;
        ui->action_Connect->setIcon(QIcon(":/images/48x48/disconnect.png"));

        if (g_Settings.flowControl != (int)QSerialPort::HardwareControl) {
            enablePinsCheckBoxes(true);
            m_worker->setRequestToSend(false);
            m_worker->setDataTerminalReady(false);
        }

        ui->btnSendFile->setEnabled(true);

        title = APP_NAME;
        title += " - ";
        title += "Connected";
        title += ": ";
        title += g_Settings.comPort;
        title += "; ";
        title += QString::number(m_worker->baudRate());
        title += "; ";
        title += QString::number(m_worker->dataBits());
        title += "; ";
        title += QString::number(m_worker->stopBits());
        title += "; ";

        switch (m_worker->flowControl()) {
            case QSerialPort::NoFlowControl: title += "None"; break;
            case QSerialPort::HardwareControl: title += "Hardware"; break;
            case QSerialPort::SoftwareControl: title += "Software"; break;
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

    emit connectStatusChanged(m_bConnected);
}

///////////////////////////////////////////////////////////////////////////////
void MainWindow::loadSettings() {
    QSettings settings;

    restoreGeometry(settings.value(STORE_GEOMETRY).toByteArray());

    #ifdef Q_OS_WIN
    g_Settings.comPort = settings.value(STORE_COMPORT, "COM1").toString();
    #else
    g_Settings.comPort = settings.value(STORE_COMPORT, "ttyS0").toString();
    #endif

    g_Settings.baudRate = settings.value(STORE_BAUDRATE, (int)QSerialPort::Baud115200).toInt();
    g_Settings.dataBits = settings.value(STORE_DATABITS, (int)QSerialPort::Data8).toInt();
    g_Settings.stopBits = settings.value(STORE_STOPBITS, (int)QSerialPort::OneStop).toInt();
    g_Settings.parity = settings.value(STORE_PARITY, (int)QSerialPort::NoParity).toInt();
    g_Settings.flowControl = settings.value(STORE_FLOWCONTROL, (int)QSerialPort::NoFlowControl).toInt();
    g_Settings.charDelay = settings.value(STORE_CHARDELAY, 0).toInt();
    g_Settings.textEncoding = settings.value(STORE_ENCODING,"System").toString();
    #ifdef Q_OS_WIN
    g_Settings.fontName = settings.value(STORE_FONTNAME, "Courier New").toString();
    #else
    g_Settings.fontName = settings.value(STORE_FONTNAME, "Monospace").toString();
    #endif
    g_Settings.fontSize = settings.value(STORE_FONTSIZE, 12).toInt();
    g_Settings.comProtocol = settings.value(STORE_MACROSPROTO, "None").toString();
    g_Settings.repeatAll = settings.value(STORE_MACROSREPT, false).toBool();
    g_Settings.timeStamp = settings.value(STORE_TIMESTAMP, "Disable").toString();

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

    settings.setValue(STORE_COMPORT,        g_Settings.comPort);
    settings.setValue(STORE_BAUDRATE,       g_Settings.baudRate);
    settings.setValue(STORE_DATABITS,       g_Settings.dataBits);
    settings.setValue(STORE_STOPBITS,       g_Settings.stopBits);
    settings.setValue(STORE_PARITY,         g_Settings.parity);
    settings.setValue(STORE_FLOWCONTROL,    g_Settings.flowControl);
    settings.setValue(STORE_CHARDELAY,      g_Settings.charDelay);
    settings.setValue(STORE_ENCODING,       g_Settings.textEncoding);
    settings.setValue(STORE_FONTNAME,       g_Settings.fontName);
    settings.setValue(STORE_FONTSIZE,       g_Settings.fontSize);
    settings.setValue(STORE_MACROSPROTO,    g_Settings.comProtocol);
    settings.setValue(STORE_MACROSREPT,     g_Settings.repeatAll);
    settings.setValue(STORE_TIMESTAMP,      g_Settings.timeStamp);

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
