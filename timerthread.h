#ifndef TIMERTHREAD_H
#define TIMERTHREAD_H

#include <QThread>

///////////////////////////////////////////////////////////////////////////////
class TimerThread : public QThread {
    Q_OBJECT

    public:
        TimerThread(QThread *parent = 0);
        ~TimerThread();
        void run();
        void terminateThread();
        void delay(qint64 milliseconds);

    private:
        bool m_Terminate;
        bool m_Started;
        quint32 m_Timer;

    signals:
        void signal_Timeout();

    private slots:
        void slot_Start(quint32 milliseconds);
        void slot_Stop();

};

#endif // TIMERTHREAD_H

