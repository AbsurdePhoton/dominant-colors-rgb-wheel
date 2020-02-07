/*#-------------------------------------------------
#
#    Dominant colors from image with openCV
#
#    by AbsurdePhoton - www.absurdephoton.fr
#
#                v2.0 - 2020/02/06
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
    void on_button_zoom_image_clicked(); // zoom on/off
    void on_button_load_image_clicked(); // load image to analyze
    void on_button_compute_clicked(); // compute dominant colors
    void on_button_analyze_clicked(); // analyze image to find color schemes and other information
    void on_button_save_clicked(); // save results
    void on_button_save_graph_clicked(); // save graph image only
    void on_button_save_quantized_clicked(); // save quantized image only
    void on_button_save_wheel_clicked(); // save wheel image only
    void on_button_save_palette_clicked(); // save palette image only
    void on_button_reset_params_clicked(); // reset all filter values in GUI
    void on_checkBox_palette_scale_stateChanged(int state); // scale or not the palette colored zones
    void on_button_palette_plus_clicked(); // add one color to palette image in the limits of found colors
    void on_button_palette_minus_clicked(); // delete one color from palette image
    void on_comboBox_sort_currentIndexChanged(int index); // sort palette
    // sliders and values
    void on_verticalSlider_circle_size_valueChanged(int value); // change color base circle size in wheel
    void on_horizontalSlider_filter_percentage_valueChanged(int value);
    void on_horizontalSlider_nb_blacks_valueChanged(int value);
    void on_horizontalSlider_nb_grays_valueChanged(int value);
    void on_horizontalSlider_nb_whites_valueChanged(int value);
    void on_horizontalSlider_regroup_distance_valueChanged(int value);
    void on_horizontalSlider_mean_shift_spatial_valueChanged(int value);
    void on_horizontalSlider_mean_shift_color_valueChanged(int value);
    void on_horizontalSlider_sectored_means_levels_valueChanged(int value);
    void on_checkBox_color_approximate_stateChanged(int state); // for analyze : auto-check other options
    void on_checkBox_color_borders_stateChanged(int state); // for analyze : auto-check other options
    void on_radioButton_mean_shift_toggled(); // mean-shift algorithm options
    void on_radioButton_sectored_means_toggled(); // sectored-means algorithm options
    void on_pushButton_color_complementary_clicked(); // hide/show color scheme : complementary
    void on_pushButton_color_split_complementary_clicked(); // hide/show color scheme : split-complementary
    void on_pushButton_color_analogous_clicked(); // hide/show color scheme : analogous
    void on_pushButton_color_triadic_clicked(); // hide/show color scheme : triadic
    void on_pushButton_color_tetradic_clicked(); // hide/show color scheme : tetradic
    void on_pushButton_color_square_clicked(); // hide/show color scheme : square

private:
    Ui::MainWindow *ui;

    //// UI
    void InitializeValues(); // initialize GUI and variables

    //// Display
    void ShowResults(); // display thumbnail, quantized image, palette
    void ShowWheel(); // display color wheel
    void OverlayWheel(); // draw layers on wheel
    void DrawOnWheel(const int &R, const int &G, const int &B, const int &radius, const bool &border); // draw one color on color wheel
    void DrawOnWheelBorder(const int &R, const int &G, const int &B, const int &radius, const bool &center); // draw one color on color wheel border

    //// Mouse & Keyboard
    void mousePressEvent(QMouseEvent *eventPress); // mouse clic events
    void wheelEvent(QWheelEvent *wheelEvent); // mouse wheel turned

    //// General
    void ComputePaletteValues(const int n); // compute palette values from RGB for one color
    void ComputePaletteImage(); // compute palette image from palettes values
    void SortPalettes(); // sort palette values
    void ResetSort(); // reset combo box to default (percentage) without activating it
    void FindColorName(const int &n_palette); // find color name for one palette item
    void Compute(); // compute dominant colors

    //// Variables

    // files
    std::string basefile, basedir, basedirinifile; // main image filename: directory and filename without extension

    // compute
    bool loaded, computed; // indicators: image loaded or computed
    cv::Mat image, // main image
            //thumbnail, // thumbnail of main image
            wheel, // wheel image
            wheel_result, // wheel image with layers
            quantized, // quantized image
            palette, // palette image
            graph; // graph image
    cv::Mat wheel_mask_complementary,
            wheel_mask_split_complementary,
            wheel_mask_analogous,
            wheel_mask_triadic,
            wheel_mask_tetradic,
            wheel_mask_square;

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
        int R; // RGB in [0..55]
        int G;
        int B;
        long double H, S, L, C, h; // in [0..1]
        long double distanceBlack, distanceWhite, distanceGray; // distance from gray values
        std::string hexa; // hexadecimal RGB
        int count; // number of pixels of this RGB color in image
        float percentage; // percentge of use in image
        QString name; // color name
    };
    static const int nb_palettes_max = 500;
    struct_palette palettes[nb_palettes_max]; // palette
    int nb_palettes, nb_palettes_found; // number of colors in palette
    cv::Vec3b pickedColor; // clicked color in palette

    // color names
    struct struct_color_names { // structure of color name
        int R; // RGB values in [0..255]
        int G;
        int B;
        QString name; // color name
    };
    struct_color_names color_names[10000]; // 9000+ values in CSV files
    int nb_color_names; // total number of color name values

    // analyze
    long double angles[nb_palettes_max][nb_palettes_max]; // Hue angles difference between colors in palette
    long double blacksLimit, whitesLimit, graysLimit; // limits for determining blacks, grays and whites values
    const long double blacksLimitIni = 18; // default parameters values in GUI
    const long double graysLimitIni = 9;
    const long double whitesLimitIni = 18;
    const long double regroupDistanceIni = 15;
    const long double filterPercentageIni = 1;
    const long double nbMeanShiftSpatialIni = 4;
    const long double nbMeanShiftColorIni = 12;
    const long double nbSectoredMeansLevels = 3;

    // timer
    QTime timer; // elapsed time

    // other items
    bool zoom; // for source and quantized images
};

#endif // MAINWINDOW_H
