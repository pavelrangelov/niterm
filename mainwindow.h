#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QCloseEvent>
#include <QKeyEvent>
#include <QStringList>
#include <QTimer>
#include <QPlainTextEdit>
#include <QProgressDialog>

#include "wserialport.h"

#define COLOR_RED       QColor(200,0,0)
#define COLOR_GREEN     QColor(0,120,0)
#define COLOR_BLUE      QColor(0,0,200)
#define COLOR_BLACK     QColor(0,0,0)

///////////////////////////////////////////////////////////////////////////////
namespace Ui
{
    class MainWindow;
}

///////////////////////////////////////////////////////////////////////////////
class MainWindow : public QMainWindow
{
    Q_OBJECT

    public:
        explicit MainWindow(QWidget *parent = 0);
        ~MainWindow();

        QStringList getEncodingList(){ return m_encodingList; }
        WSerialPort *getSerialDevice(){ return m_serialPort; }

    private:
        Ui::MainWindow *ui;
        bool m_bConnected;
        WSerialPort *m_serialPort;
        QStringList m_encodingList;
        QTimer *m_pinoutsReadTimer;
        QChar m_previousChar;
        QProgressDialog *progress;
        bool m_bCanceled;
        bool m_bStartup;
        bool m_timeStamp;

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
        void appendText(QString &text, const QColor color);

    protected:
        virtual void closeEvent(QCloseEvent *event);

    private slots:
        void on_action_Connect_triggered();
        void on_action_Settings_triggered();
        void on_action_About_triggered();
        void on_btnOutputClear_clicked();
        void on_btnInputClear_clicked();
        void on_btnMacros_clicked();
        void on_btnSendFile_clicked();
        void on_checkDTR_stateChanged(int state);
        void on_checkRTS_stateChanged(int state);
        void slot_serialDataReady();
        void slot_keyPressed(QString text);
        void slot_pinoutReadTimerTimeout();
        void slot_writeData(QByteArray &data);
        void slot_progressCanceled();
        void slot_serialPortError(QSerialPort::SerialPortError);
        void slot_updateUI(QByteArray &data);

    signals:
        void closed();
        void signal_dataReady(QByteArray &data);
        void signal_connectStatusChanged(bool connected);
};

#endif // MAINWINDOW_H
