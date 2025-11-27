#ifndef MACROSDIALOG_H
#define MACROSDIALOG_H

#include <QDialog>
#include <QResizeEvent>
#include <QCloseEvent>
#include <QtXml>
#include <QThread>

#include "wserialport.h"
#include "timerthread.h"

#define ASCII_SOH   0x01
#define ASCII_ETX   0x03
#define ASCII_EOT   0x04
#define ASCII_ENQ   0x05
#define ASCII_LF    0x0a
#define ASCII_CR    0x0d
#define ASCII_SYN   0x16
#define ASCII_QT    0x22

#define COL_MACRO_BUTTON    0
#define COL_MACRO_COMMAND   1
#define COL_MACRO_DATA      2
#define COL_MACRO_RESPONSE  3
#define COL_MACRO_COMMENT   4
#define COL_MACRO_LAST      5

#define ECR_SYN_TOUT        500

class ItemClass {
    public:
        QString command;
        QString data;
        QString response;
        QString comment;

        ItemClass() {
            command.clear();
            data.clear();
            response.clear();
            comment.clear();
        }
};

///////////////////////////////////////////////////////////////////////////////
namespace Ui {
    class MacrosDialog;
}

///////////////////////////////////////////////////////////////////////////////
class MacrosDialog : public QDialog {
    Q_OBJECT

    public:
        explicit MacrosDialog(QWidget *parent = 0);
        ~MacrosDialog();
        void setConnectStatus(bool connected);

        static const quint8 syntaxArray[5][5];

    private:
        Ui::MacrosDialog *ui;
        QByteArray m_Data;
        quint8 m_SeqNum;
        quint16 m_RowId;
        int m_SenderRow;
        QByteArray m_EcrAnswer;
        bool m_EcrRun;
        int m_EcrIdx;
        int m_EcrResult;
        bool m_Cancel;
        bool m_Connected;
        WSerialPort *wsp;
        TimerThread *m_SynTimer;

        int insertTableRow(int row);
        QDomElement paramToNode(QDomDocument &d, const ItemClass &ni);
        int getSelectedRow();
        bool saveDocument(const QString &filename);
        bool loadDocument(const QString &filename);
        void clearDocument();
        QString chooseSaveFile();
        QString chooseOpenFile();

        quint8 convertCommand(QByteArray data);
        QByteArray convertData(QByteArray data);
        QByteArray convertFunction(QByteArray data);
        bool checkSyntax(QByteArray data);
        int getIndexByCharType(QChar ch);
        QByteArray removeSpaces(QByteArray data);
        int getRowByProperty(int value);
        void delay(qint64 milliseconds);

        quint8 getSeqNum();
        QByteArray makeEcrProto(quint8 command, QByteArray data);
        QByteArray checkSum(const QByteArray data);
        void startEcrCom(int row);
        void stopEcrCom();
        bool parseEcrFrame(QByteArray data);
        int parseEcrAnswer(QByteArray data);
        QString getErrorText(int error);

    protected:
        virtual void resizeEvent(QResizeEvent *event);
        virtual void closeEvent(QCloseEvent* event);

    public slots:
        void save();
        void open();
        void insertAbove();
        void insertBellow();
        void remove();
        void sendAll();
        void setProtocolMode(int index);
        void sendCurrentRow();
        void hasDataReady(QByteArray data);
        void cancelSending();
        void connectStatusHasChanged(bool connected);
        void synTout();

    signals:
        void writeData(QByteArray &data);
        void cancel();
        void startThreadTimer(quint32 milliseconds);
        void stopThreadTimer();
};

#endif // MACROSDIALOG_H
