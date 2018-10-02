#ifndef WTEXTEDIT_H
#define WTEXTEDIT_H

#include <QTextEdit>
#include <QKeyEvent>

///////////////////////////////////////////////////////////////////////////////
class WTextEdit : public QTextEdit
{
    Q_OBJECT

    public:
        WTextEdit(QWidget *parent = 0);

    protected:
        virtual void keyPressEvent(QKeyEvent *event);

    signals:
        void keyPressed(QString text);
};

#endif // WTEXTEDIT_H
