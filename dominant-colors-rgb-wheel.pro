#-------------------------------------------------
#
#   Dominant colors from image with openCV
#
#    by AbsurdePhoton - www.absurdephoton.fr
#
#                v0.1 - 2019/10/24
#
#-------------------------------------------------

QT       += core gui

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = dominant-colors-rgb-wheel
TEMPLATE = app

INCLUDEPATH += /usr/local/include/opencv2

LIBS += -L/usr/local/lib -lopencv_core -lopencv_imgcodecs -lopencv_highgui

SOURCES += main.cpp\
        mainwindow.cpp \
        mat-image-tools.cpp \
        dominant-colors.cpp

HEADERS  += mainwindow.h \
            mat-image-tools.h \
            dominant-colors.h

FORMS    += mainwindow.ui

# we add the package opencv to pkg-config
CONFIG += link_pkgconfig
PKGCONFIG += opencv4

QMAKE_CXXFLAGS += -std=c++11

# icons
RESOURCES += resources.qrc
