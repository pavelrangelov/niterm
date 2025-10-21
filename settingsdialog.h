#ifndef SETTINGSDIALOG_H
#define SETTINGSDIALOG_H

#include <QDialog>
#include <QString>
#include <QSerialPortInfo>
#include <QList>
#include <QStringList>

#include "mainwindow.h"

///////////////////////////////////////////////////////////////////////////////
namespace Ui {
    class SettingsDialog;
}

///////////////////////////////////////////////////////////////////////////////
class SettingsDialog : public QDialog {
    Q_OBJECT

    public:
        explicit SettingsDialog(QWidget *parent = 0);
        ~SettingsDialog();

    private:
        Ui::SettingsDialog *ui;

        QString getPortDescripion(const QString &text);
        QString parsePort(const QString &text);
        QStringList getPortNames();

    private slots:
        void setSettings();
};

#endif // SETTINGSDIALOG_H
