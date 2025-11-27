#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QCloseEvent>
#include <QKeyEvent>
#include <QStringList>
#include <QTimer>
#include <QPlainTextEdit>
#include <QProgressDialog>
#include <QThread>

#include "wserialport.h"
#include "worker.h"

#define COLOR_RED       QColor(200,0,0)
#define COLOR_GREEN     QColor(0,120,0)
#define COLOR_BLUE      QColor(0,0,200)
#define COLOR_BLACK     QColor(0,0,0)

///////////////////////////////////////////////////////////////////////////////
namespace Ui {
    class MainWindow;
}

///////////////////////////////////////////////////////////////////////////////
class MainWindow : public QMainWindow {
    Q_OBJECT

    public:
        explicit MainWindow(QWidget *parent = 0);
        ~MainWindow();

        QStringList getEncodingList(){ return m_encodingList; }
        WSerialPort *getSerialDevice(){ return m_worker->getSerialPort(); }

    private:
        Ui::MainWindow *ui;
        bool m_bConnected;
        QStringList m_encodingList;
        QChar m_previousChar;
        bool m_bCanceled;
        bool m_bStartup;
        bool m_timeStamp;
        QProgressDialog *progress;
        QTimer *m_pinoutsReadTimer;
        Worker *m_worker;

        void setConnected(bool connected);
        void setAsciiData(QByteArray &data, QColor color);
        void setHexData(QByteArray &data, QColor color);
        QString getEncoding(int index);
        void delay(qint64 milliseconds);

        void initVars();
        void loadSettings();
        void saveSettings();
        void setPinsCheckBoxes(bool state);
        void enablePinsCheckBoxes(bool enable);
        QString chooseSendFile();
        void updateSettings();
        void appendText(QByteArray &text, const QColor color);

    protected:
        virtual void closeEvent(QCloseEvent *event);

    private slots:
        void doConnect();
        void setSettings();
        void doAbout();
        void clearOutput();
        void clearInput();
        void doMacros();
        void sendFile();
        void setDTR(int state);
        void setRTS(int state);
        void keyPressed(QString text);
        void pinoutReadTimerTimeout();
        void outputTransmitedData(QByteArray data);
        void outputReceivedData(QByteArray data);
        void serialPortError(QSerialPort::SerialPortError error);
        void progressCanceled();

    signals:
        void windowClosed();
        void connectStatusChanged(bool connected);
        void writeData(QByteArray data);
};

#endif // MAINWINDOW_H
