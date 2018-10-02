#include <QApplication>
#include <QDateTime>

#include "timerthread.h"

///////////////////////////////////////////////////////////////////////////////
TimerThread::TimerThread(QThread *parent) : QThread(parent)
{
    m_Terminate = false;
    m_Started = false;
    m_Timer = 0;
}

///////////////////////////////////////////////////////////////////////////////
TimerThread::~TimerThread()
{
}

///////////////////////////////////////////////////////////////////////////////
void TimerThread::run()
{
    while (!m_Terminate)
    {
        while (!m_Terminate && m_Started && m_Timer > 0)
        {
            delay(1);
            m_Timer--;
        }

        if (!m_Terminate && m_Started)
        {
            slot_Stop();
            emit signal_Timeout();
        }

        qApp->processEvents();
    }
}

///////////////////////////////////////////////////////////////////////////////
void TimerThread::terminateThread()
{
    m_Terminate = true;
}

///////////////////////////////////////////////////////////////////////////////
void TimerThread::slot_Start(quint32 milliseconds)
{
    m_Timer = milliseconds;
    m_Started = true;
}

///////////////////////////////////////////////////////////////////////////////
void TimerThread::slot_Stop()
{
    m_Started = false;
    m_Timer = 0;
}

///////////////////////////////////////////////////////////////////////////////
void TimerThread::delay(qint64 milliseconds)
{
    qint64 timeToExit = QDateTime::currentMSecsSinceEpoch() + milliseconds;
    while (timeToExit > QDateTime::currentMSecsSinceEpoch())
    {
        QApplication::processEvents();
    }
}
