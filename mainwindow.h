/*#-------------------------------------------------
#
#    Dominant colors from image with openCV
#
#    by AbsurdePhoton - www.absurdephoton.fr
#
#                v1.0 - 2020/01/11
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

#include "color-spaces.h"

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();

public slots:
    void ShowTimer(const bool start); // elapsed time
    void SetCircleSize(int size); // called when circle size slider is moved

private slots:

    //// load & save
    void ChangeBaseDir(QString filename); // set base dir and file
    void SaveDirBaseFile(); // keep last open dir

    //// GUI
    void on_button_quit_clicked(); // quit GUI
    void on_button_load_image_clicked(); // load image to analyze
    void on_button_compute_clicked(); // compute dominant colors
    void on_button_analyze_clicked(); // analyze image to find color schemes
    void on_button_save_clicked(); // save results
    void on_button_reset_params_clicked(); // reset all filter values in GUI
    // sliders and values
    void on_horizontalSlider_nb_percentage_valueChanged(int value);
    void on_horizontalSlider_nb_blacks_valueChanged(int value);
    void on_horizontalSlider_nb_grays_valueChanged(int value);
    void on_horizontalSlider_nb_whites_valueChanged(int value);
    void on_horizontalSlider_regroup_angle_valueChanged(int value);
    void on_horizontalSlider_regroup_distance_valueChanged(int value);
    void on_horizontalSlider_mean_shift_spatial_valueChanged(int value);
    void on_horizontalSlider_mean_shift_color_valueChanged(int value);
    void on_checkBox_color_approximate_stateChanged(int state); // for analyze : auto-check other options
    void on_checkBox_color_borders_stateChanged(int state); // for analyze : auto-check other options
    void on_radioButton_mean_shift_toggled(); // mean-shift algorithm options

private:
    Ui::MainWindow *ui;

    //// UI
    void InitializeValues(); // initialize GUI and variables

    //// Display
    void ShowResults(); // display thumbnail, quantized image, palette
    void ShowWheel(); // display color wheel
    void DrawOnWheel(const int &R, const int &G, const int &B, const int &radius); // draw one color on color wheel
    void DrawOnWheelBorder(const int &R, const int &G, const int &B, const int &radius, const bool center); // draw one color on color wheel border

    //// Mouse & Keyboard
    void mousePressEvent(QMouseEvent *eventPress); // mouse clic events
    void wheelEvent(QWheelEvent *wheelEvent); // mouse wheel turned

    //// General
    void ComputePaletteValues(const int n); // compute palette values from RGB for one color
    void Compute(); // compute dominant colors

    //// Variables

    // files
    std::string basefile, basedir, basedirinifile; // main image filename: directory and filename without extension

    // compute
    bool loaded, computed; // indicators: image loaded or computed
    cv::Mat image, // main image
            thumbnail, // thumbnail of main image
            wheel, // wheel image
            quantized, // quantized image
            palette; // palette image

    // color wheel
    cv::Point wheel_center; // wheel center
    int wheel_radius, wheel_radius_center; // wheel radius

    // mouse
    Qt::MouseButton mouseButton; // mouse button value
    QPoint mouse_pos; // mouse position

    // palette
    const int palette_width = 1200; // palette image dimensions
    const int palette_height = 250;
    struct struct_palette{ // structure of a color value
        int R;
        int G;
        int B;
        long double H, S, L;
        long double distanceBlack, distanceWhite, distanceGray;
        std::string hexa;
        int count;
        float percentage;
        QString name;
    };

    // color names
    static const int colors_max = 100;
    struct_palette palettes[colors_max]; // 100 is very large !
    int nb_palettes; // number of colors in palette
    struct struct_color_names { // structure of color name
        int R;
        int G;
        int B;
        QString name;
    };
    struct_color_names color_names[10000]; // 9000+ values in CSV files
    int nb_color_names; // total number of color name values

    // analyze
    long double angles[colors_max][colors_max]; // Hue angles difference between colors in palette
    long double blacksLimit, whitesLimit, graysLimit; // limits for determining blacks, grays and whites values
    const long double blacksLimitIni = 18; // default parameters values in GUI
    const long double whitesLimitIni = 15;
    const long double graysLimitIni = 12;
    const long double regroupAngleIni = 15;
    const long double regroupDistanceIni = 10;
    const long double nbPercentageIni = 1;
    const long double nbMeanShiftSpatialIni = 4;
    const long double nbMeanShiftColorIni = 24;
    // timer
    QTime timer; // elapsed time
};

#endif // MAINWINDOW_H
