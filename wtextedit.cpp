#include <QString>
#include "wtextedit.h"

///////////////////////////////////////////////////////////////////////////////
WTextEdit::WTextEdit(QWidget *parent) : QTextEdit(parent) {
}

///////////////////////////////////////////////////////////////////////////////
void WTextEdit::keyPressEvent(QKeyEvent *event) {
    QString text = event->text();

    if (text == "\r") {
        text += "\n";
    }

    emit keyPressed(text);

    QTextEdit::keyPressEvent(event);
}
