#-------------------------------------------------
#
# Project created by QtCreator 2016-12-05T09:51:45
#
#-------------------------------------------------

QT       += core gui

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = squanthrough
TEMPLATE = app


SOURCES += main.cpp\
        mainwindow.cpp \
    additional_classes.cpp

HEADERS  += mainwindow.h \
    additional_classes.h

FORMS    += mainwindow.ui \
    script.ui

#IMBLEXEC = $$system(command -v imblgui)
#IMBLORIGIN = $$dirname(IMBLEXEC)
#IMBLRPATH = $$system(objdump -x $${IMBLEXEC} | grep RPATH | sed -e \"s/RPATH//g\" -e \"s/^ *//g\" -e \"s \\\$ORIGIN $${IMBLORIGIN} g\" | cut -d':' -f1 )

#QMAKE_LFLAGS += -Wl,-rpath,$$IMBLRPATH
LIBS += -lqtpv \
    -lqtpvwidgets \
    -lqcamotor \
    -lqcamotorgui
#    -L$$IMBLRPATH \
#    -lshutter1A \
#    -lcomponent

