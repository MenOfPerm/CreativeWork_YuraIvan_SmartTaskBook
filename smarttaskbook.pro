QT += core gui widgets network openglwidgets

RC_FILE = resources.rc

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = SmartTaskBook
TEMPLATE = app

SOURCES += main.cpp \
    smarttaskbook.cpp \
    taskwidget.cpp \

HEADERS += task.h \
    smarttaskbook.h \
    taskwidget.h \
