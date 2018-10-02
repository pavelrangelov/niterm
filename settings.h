#ifndef SETTINGS_H
#define SETTINGS_H

#include <QString>

#define APP_NAME "NiTerm"
#define ORG_NAME "Nifelheim"
#define APP_VERS "1.01"

#define STORE_GEOMETRY      "APP/Geometry"
#define STORE_ENCODING      "APP/Encoding"
#define STORE_FONTNAME      "APP/FontName"
#define STORE_FONTSIZE      "APP/FontSize"
#define STORE_TIMESTAMP     "APP/Timestamp"
#define STORE_SENDFILEDIR   "APP/SendFileDir"
#define STORE_COMPORT       "COM/ComPort"
#define STORE_BAUDRATE      "COM/BaudRate"
#define STORE_DATABITS      "COM/DataBits"
#define STORE_STOPBITS      "COM/StopBits"
#define STORE_PARITY        "COM/Parity"
#define STORE_FLOWCONTROL   "COM/FlowControl"
#define STORE_CHARDELAY     "COM/CharDelay"
#define STORE_MACROSDIR     "MACROS/Dir"
#define STORE_MACROSGEO     "MACROS/Geometry"
#define STORE_MACROSPROTO   "MACROS/Protocol"
#define STORE_MACROSREPT    "MACROS/Repeat"

///////////////////////////////////////////////////////////////////////////////
typedef struct
{
    QString comPort;
    int baudRateIndex;
    int dataBitsIndex;
    int stopBitsIndex;
    int parityIndex;
    int flowControlIndex;
    int charDelay;
    QString textEncoding;
    QString fontName;
    int fontSize;
    QString comProtocol;
    bool repeatAll;
    QString sendFileDir;
    QString timeStamp;
} SETTINGS_t;

extern SETTINGS_t g_Settings;

#endif // SETTINGS_H
