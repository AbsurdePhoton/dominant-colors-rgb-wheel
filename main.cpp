/*#-------------------------------------------------
#
#    Dominant colors from image with openCV
#
#    by AbsurdePhoton - www.absurdephoton.fr
#
#                v0.1 - 2019/10/24
#
#-------------------------------------------------*/

#include "mainwindow.h"
#include <QApplication>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    MainWindow w;
    w.show();

    return a.exec();
}
