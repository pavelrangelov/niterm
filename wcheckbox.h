#ifndef WCHECKBOX_H
#define WCHECKBOX_H

#include <QWidget>
#include <QCheckBox>
#include <QMouseEvent>

///////////////////////////////////////////////////////////////////////////////
class WCheckBox : public QCheckBox {
    public:
        WCheckBox(QWidget *parent = 0);

        void setReadOnly(bool readonly){ m_breadOnly = readonly; }

    private:
        bool m_breadOnly;

    protected:
        virtual void mousePressEvent(QMouseEvent *event);
};

#endif // WCHECKBOX_H
