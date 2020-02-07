#-------------------------------------------------
#
#   Dominant colors from image with openCV
#
#    by AbsurdePhoton - www.absurdephoton.fr
#
#                v2.0 - 2019/02/06
#
#-------------------------------------------------

QT       += core gui

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = dominant-colors-rgb-wheel
TEMPLATE = app

INCLUDEPATH += /usr/local/include/opencv4/opencv2

LIBS += -L/usr/local/lib

SOURCES += main.cpp\
        mainwindow.cpp \
        mat-image-tools.cpp \
        dominant-colors.cpp \
        color-spaces.cpp \
        angles.cpp

HEADERS  += mainwindow.h \
            mat-image-tools.h \
            dominant-colors.h \
            color-spaces.h \
            angles.h

FORMS    += mainwindow.ui

# we add the package opencv to pkg-config
CONFIG += link_pkgconfig
PKGCONFIG += opencv4

# icons
RESOURCES += resources.qrc

CONFIG += c++11
