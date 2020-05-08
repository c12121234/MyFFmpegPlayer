QT       += core gui opengl multimedia

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

CONFIG += c++11

#Release:DEFINES += QT_NO_WARNING_OUTPUT\


# The following define makes your compiler emit warnings if you use
# any Qt feature that has been marked deprecated (the exact warnings
# depend on your compiler). Please consult the documentation of the
# deprecated API in order to know how to port your code away from it.
DEFINES += QT_DEPRECATED_WARNINGS #QT_NO_DEBUG_OUTPUT

# You can also make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
# You can also select to disable deprecated APIs only up to a certain version of Qt.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

SOURCES += \
    ffmpegaudio.cpp \
    ffmpegaudioplay.cpp \
    ffmpegdecode.cpp \
    ffmpegdecodethread.cpp \
    ffmpegdemux.cpp \
    ffmpegdemuxthread.cpp \
    ffmpegnv12widget.cpp \
    ffmpegresample.cpp \
    ffmpegslider.cpp \
    ffmpegvideo.cpp \
    ffmpegvideowidget.cpp \
    main.cpp \
    widget.cpp

HEADERS += \
    IVideoCall.h \
    ffmpegaudio.h \
    ffmpegaudioplay.h \
    ffmpegdecode.h \
    ffmpegdecodethread.h \
    ffmpegdemux.h \
    ffmpegdemuxthread.h \
    ffmpegnv12widget.h \
    ffmpegresample.h \
    ffmpegslider.h \
    ffmpegvideo.h \
    ffmpegvideowidget.h \
    widget.h

FORMS += \
    widget.ui

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target

win32:CONFIG(release, debug|release): LIBS += -L$$PWD/../../FFmpegDemux/FFmpegDemux/lib/release/ -lavformat
else:win32:CONFIG(debug, debug|release): LIBS += -L$$PWD/../../FFmpegDemux/FFmpegDemux/lib/debug/ -lavformat
else:unix: LIBS += -L$$PWD/../../FFmpegDemux/FFmpegDemux/lib/ -lavformat

win32:CONFIG(release, debug|release): LIBS += -L$$PWD/../../FFmpegDemux/FFmpegDemux/lib/release/ -lavcodec
else:win32:CONFIG(debug, debug|release): LIBS += -L$$PWD/../../FFmpegDemux/FFmpegDemux/lib/debug/ -lavcodec
else:unix: LIBS += -L$$PWD/../../FFmpegDemux/FFmpegDemux/lib/ -lavcodec

win32:CONFIG(release, debug|release): LIBS += -L$$PWD/../../FFmpegDemux/FFmpegDemux/lib/release/ -lavutil
else:win32:CONFIG(debug, debug|release): LIBS += -L$$PWD/../../FFmpegDemux/FFmpegDemux/lib/debug/ -lavutil
else:unix: LIBS += -L$$PWD/../../FFmpegDemux/FFmpegDemux/lib/ -lavutil

win32:CONFIG(release, debug|release): LIBS += -L$$PWD/lib/release/ -lswresample
else:win32:CONFIG(debug, debug|release): LIBS += -L$$PWD/lib/debug/ -lswresample
else:unix: LIBS += -L$$PWD/lib/ -lswresample

INCLUDEPATH += $$PWD/include
DEPENDPATH += $$PWD/include
