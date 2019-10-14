/*#-------------------------------------------------
#
#    Dominant colors from image with openCV
#
#    by AbsurdePhoton - www.absurdephoton.fr
#
#                v0 - 2019/10/10
#
#-------------------------------------------------*/

#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include "opencv2/highgui.hpp"
#include "opencv2/imgproc.hpp"
#include "opencv2/imgcodecs.hpp"
#include <opencv2/ximgproc.hpp>
#include "opencv2/opencv.hpp"
#include "opencv2/core/utility.hpp"
#include <QMainWindow>
#include <QFileDialog>
#include <QTime>

#include "mat-image-tools.h"

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();

    QTime timer;

public slots:
    void ShowTimer(); // time elapsed

private slots:

    //// quit
    void on_button_quit_clicked(); // quit GUI

    //// load & save
    void ChangeBaseDir(QString filename); // set base dir and file
    void SaveDirBaseFile(); // just to keep the last open dir

    //// GUI
    void on_button_load_image_clicked(); // load image to analyze
    void on_button_compute_clicked(); // compute dominant colors
    void on_button_save_images_clicked(); // save results

private:
    Ui::MainWindow *ui;

    //// UI
    void InitializeValues(); // initialize GUI and variables

    //// Display
    void ShowResults(); // display thumbnail, quantized image, palette
    void ShowWheel(); // display color wheel
    void DrawOnWheel(const int &R, const int &G, const int &B, const int &radius, const int wheel_radius_local, const bool &border); // draw one color on wheel

    //// Mouse & Keyboard
    void mousePressEvent(QMouseEvent *eventPress); // mouse clic events

    //// General
    void Compute(); // compute dominant colors

    //// Variables

    std::string basefile, basedir, basedirinifile; // main image filename: directory and filename without extension
    bool loaded, computed; // indicators: image loaded or computed
    cv::Mat image, // main image
            thumbnail, // thumbnail of main image
            wheel, // wheel image
            quantized, // quantized image
            //classification, // classification image
            palette; // palette image
    cv::Point wheel_center; // wheel center
    int wheel_radius, wheel_radius_center; // wheel radius
    Qt::MouseButton mouseButton; // mouse button value
    QPoint mouse_pos; // mouse position
    const int palette_width = 1000; // palette image dimensions
    const int palette_height = 250;

    struct struct_palette{ // structure of a color value
        int R;
        int G;
        int B;
        int count;
        float percentage;
        static bool compOnR(const struct_palette& a,const struct_palette& b) {return a.count > b.count;} // for sort function, decreasing
    };
    struct_palette palettes[100]; // 100 is very large !
    int nb_palettes; // number of colors in palette

    struct struct_color_names {
        int R;
        int G;
        int B;
        QString name;
    };
    struct_color_names color_names[10000];
    int nb_color_names;
};

#endif // MAINWINDOW_H
