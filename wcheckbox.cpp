#include "wcheckbox.h"

///////////////////////////////////////////////////////////////////////////////
WCheckBox::WCheckBox(QWidget *parent) : QCheckBox(parent) {
    m_breadOnly = false;
}

///////////////////////////////////////////////////////////////////////////////
void WCheckBox::mousePressEvent(QMouseEvent *event) {
    if (m_breadOnly) {
        return;
    }

    QCheckBox::mousePressEvent(event);
}
