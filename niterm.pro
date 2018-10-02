#-------------------------------------------------
#
# Project created by QtCreator 2014-09-01T16:40:53
#
#-------------------------------------------------

QT       += core gui serialport xml

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = niterm
TEMPLATE = app


SOURCES += main.cpp\
		mainwindow.cpp \
	wserialport.cpp \
	settingsdialog.cpp \
	settings.cpp \
	tools.cpp \
	wtextedit.cpp \
	wcheckbox.cpp \
	macrosdialog.cpp \
	xml.cpp \
	helpdialog.cpp \
    ecr.cpp \
    timerthread.cpp

HEADERS  += mainwindow.h \
	wserialport.h \
	settingsdialog.h \
	settings.h \
	wtextedit.h \
	wcheckbox.h \
	macrosdialog.h \
	helpdialog.h \
    timerthread.h

FORMS    += mainwindow.ui \
	settingsdialog.ui \
	macrosdialog.ui \
	helpdialog.ui

RESOURCES += \
	main.qrc

RC_FILE = main.rc
