/*#-------------------------------------------------
#
#    Dominant colors from image with openCV
#
#    by AbsurdePhoton - www.absurdephoton.fr
#
#                v1.0 - 2020/01/11
#
#-------------------------------------------------*/

#include "mainwindow.h"
#include "ui_mainwindow.h"

#include <QMessageBox>
#include <QSizeGrip>
#include <QGridLayout>
#include <QDesktopWidget>
#include <QCursor>
#include <QMouseEvent>
#include <QPainter>

#include <fstream>

#include "mat-image-tools.h"
#include "dominant-colors.h"
#include "color-spaces.h"
#include "angles.h"

/////////////////// Window init //////////////////////

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    // window
    setWindowFlags(Qt::Window | Qt::CustomizeWindowHint | Qt::WindowMinMaxButtonsHint); // just minimize and size buttons
    this->setWindowState(Qt::WindowMaximized); // maximize window
    setFocusPolicy(Qt::StrongFocus); // catch keyboard and mouse in priority
    //statusBar()->setVisible(false); // no status bar
    setWindowIcon(QIcon(":/icons/color.png"));

    // add size grip to wheel
    ui->label_wheel->setWindowFlags(Qt::SubWindow);
    QSizeGrip * sizeGrip = new QSizeGrip(ui->label_wheel);
    QGridLayout * layout = new QGridLayout(ui->label_wheel);
    layout->addWidget(sizeGrip, 0,0,1,1,Qt::AlignBottom | Qt::AlignRight);

    // initial variable values
    InitializeValues(); // intial variable values and GUI elements
}

MainWindow::~MainWindow()
{
    delete ui;
}

///////////////////      GUI       //////////////////////

void MainWindow::InitializeValues() // Global variables and GUI elements init
{
    loaded = false; // main image NOT loaded
    computed = false; // dominant colors NOT computed

    basedirinifile = QDir::currentPath().toUtf8().constData(); // where to store the folder ini file
    basedirinifile += "/dir.ini";
    cv::FileStorage fs(basedirinifile, cv::FileStorage::READ); // open dir ini file
    if (fs.isOpened()) {
        fs["BaseDir"] >> basedir; // load dir
    }
        else basedir = "/home/"; // default base path and file
    basefile = "example";

    // Timer
    QPalette pal = ui->timer->palette(); // use a palette
    pal.setColor(QPalette::Normal, QPalette::Light, QColor(255,0,0)); // set values for QLCDNumber
    pal.setColor(QPalette::Normal, QPalette::Foreground, QColor(255,92,92));
    pal.setColor(QPalette::Normal, QPalette::Dark, QColor(164,0,0));
    ui->timer->setPalette(pal); // set palette to QLCDNumber
    ui->timer->display("-------"); // reset timer

    // Wheel
    nb_palettes= -1; // no palette yet
    ShowWheel(); // draw empty wheel

    // limits for blacks, grays and whites, angle and distance values
    blacksLimit = blacksLimitIni;
    whitesLimit = whitesLimitIni;
    graysLimit = graysLimitIni;
    on_button_reset_params_clicked(); // set default ui slider values

    // other GUI items
    ui->frame_analysis->setVisible(false); // frames
    ui->frame_analyze->setVisible(false);
    ui->frame_rgb->setVisible(false);
    ui->frame_mean_shift_parameters->setVisible(false);

    // read color names from .csv file
    std::string line; // line to read in text file
    std::ifstream names; // file to read
    names.open("color-names.csv"); // read color names file

    if (names) { // if successfully read
        nb_color_names = -1; // index of color names array
        size_t pos; // index for find function
        std::string s; // used for item extraction
        getline(names, line); // read first line (header)
        while (getline(names, line)) { // read each line of text file: R G B name
            pos = 0; // find index at the beginning of the line
            nb_color_names++; // current index in color names array
            int pos2 = line.find(";", pos); // find first semicolon char
            s = line.substr(pos, pos2 - pos); // extract R value
            color_names[nb_color_names].R = std::stoi(s); // R in array
            pos = pos2 + 1; // next char
            pos2 = line.find(";", pos); // find second semicolon char
            s = line.substr(pos, pos2 - pos); // extract G value
            color_names[nb_color_names].G = std::stoi(s); // G in array
            pos = pos2 + 1; // next char
            pos2 = line.find(";", pos); // find third semicolon char
            s = line.substr(pos, pos2 - pos); // extract B value
            color_names[nb_color_names].B = std::stoi(s); // B in array
            s = line.substr(pos2 + 1, line.length() - pos2); // color name is at the end of the line
            color_names[nb_color_names].name = QString::fromStdString(s); // color name in array
        }

        names.close(); // close text file
    }
    else {
        QMessageBox::critical(this, "Colors CSV file not found!", "You forgot to put 'color-names.csv' in the same folder as the executable! This tool will crash as soon as you quantize an image...");
    }
}

void MainWindow::on_button_quit_clicked() // quit GUI
{
    int quit = QMessageBox::question(this, "Quit this wonderful program", "Are you sure you want to quit?", QMessageBox::Yes|QMessageBox::No); // quit, are you sure ?
    if (quit == QMessageBox::No) // don't quit !
        return;

    QCoreApplication::quit(); // quit
}

void MainWindow::on_button_compute_clicked() // compute dominant colors and result images
{
    Compute();
}

void MainWindow::on_button_reset_params_clicked() // reset all filter values in GUI
{
    ui->horizontalSlider_nb_blacks->setValue(blacksLimitIni); // sliders
    ui->horizontalSlider_nb_grays->setValue(graysLimitIni);
    ui->horizontalSlider_nb_whites->setValue(whitesLimitIni);
    ui->horizontalSlider_regroup_angle->setValue(regroupAngleIni);
    ui->horizontalSlider_regroup_distance->setValue(regroupDistanceIni);
    ui->horizontalSlider_nb_percentage->setValue(nbPercentageIni);
    ui->horizontalSlider_mean_shift_spatial->setValue(nbMeanShiftSpatialIni);
    ui->horizontalSlider_mean_shift_color->setValue(nbMeanShiftColorIni);
    ui->checkBox_regroup->setChecked(false); // checkboxes
    ui->checkBox_filter_grays->setChecked(true);
    ui->checkBox_filter_percent->setChecked(true);
}

void MainWindow::on_horizontalSlider_nb_percentage_valueChanged(int value) // update corresponding label
{
    ui->label_nb_percentage->setText(QString::number(value) + "%");
}

void MainWindow::on_horizontalSlider_nb_blacks_valueChanged(int value) // update corresponding label
{
    ui->label_nb_blacks->setText(QString::number(value) + "%");
    blacksLimit = (long double)(value);
}

void MainWindow::on_horizontalSlider_nb_grays_valueChanged(int value) // update corresponding label
{
    ui->label_nb_grays->setText(QString::number(value) + "%");
    graysLimit = (long double)(value);
}

void MainWindow::on_horizontalSlider_nb_whites_valueChanged(int value) // update corresponding label
{
    ui->label_nb_whites->setText(QString::number(value) + "%");
    whitesLimit = (long double)(value);
}

void MainWindow::on_horizontalSlider_regroup_angle_valueChanged(int value) // update corresponding label
{
    ui->label_regroup_angle->setText(QString::number(value) + "°");
}

void MainWindow::on_horizontalSlider_regroup_distance_valueChanged(int value) // update corresponding label
{
    ui->label_regroup_distance->setText(QString::number(value));
}

void MainWindow::on_horizontalSlider_mean_shift_spatial_valueChanged(int value) // update corresponding label
{
    ui->label_mean_shift_spatial->setText(QString::number(value));
}

void MainWindow::on_horizontalSlider_mean_shift_color_valueChanged(int value) // update corresponding label
{
    ui->label_mean_shift_color->setText(QString::number(value));
}

void MainWindow::on_checkBox_color_approximate_stateChanged(int state) // if limiting to 12 hues -> lines must be drawn on borders
{
    if (ui->checkBox_color_approximate->isChecked()) {
        ui->checkBox_color_borders->blockSignals(true);
        ui->checkBox_color_borders->setChecked(true);
        ui->checkBox_color_borders->blockSignals(false);
    }
    ui->checkBox_color_borders->setVisible(!ui->checkBox_color_approximate->isChecked());
}

void MainWindow::on_checkBox_color_borders_stateChanged(int state) // if limiting to 12 hues -> lines must be drawn on borders
{
    if (ui->checkBox_color_approximate->isChecked()) {
        ui->checkBox_color_borders->blockSignals(true);
        ui->checkBox_color_borders->setChecked(true);
        ui->checkBox_color_borders->blockSignals(false);
    }
}

void MainWindow::on_radioButton_mean_shift_toggled() // show or not mean-shift parameters
{
    ui->frame_mean_shift_parameters->setVisible(ui->radioButton_mean_shift->isChecked());
}

void MainWindow::on_button_analyze_clicked() // analyze image to find color schemes
{
    if (!computed) { // nothing computed yet = get out
        QMessageBox::critical(this, "Nothing to do!", "You have to load then compute before analyzing an image");
        return;
    }

    QApplication::setOverrideCursor(Qt::WaitCursor); // wait cursor
    timer.start(); // reinit timer
    ShowTimer(true); // show it
    qApp->processEvents();

    ShowWheel(); // get a fresh empty wheel

    // only keep colors in palette : no grays, no whites, no blacks + keep significant percentage
    struct_palette palet[colors_max]; // temp copy of palette
    int nb_palet = 0; // index of this copy
    for (int n = 0; n < nb_palettes; n++) // parse original palette
        if ((palettes[n].distanceBlack > blacksLimit) and (palettes[n].distanceWhite > whitesLimit) and (palettes[n].distanceGray > graysLimit)
                and (palettes[n].percentage >= double(ui->spinBox_color_percentage->value()) / 100.0)) { // test saturation, lightness and percentage
            palet[nb_palet] = palettes[n]; // copy color value
            if (ui->checkBox_color_approximate->isChecked()) { // only 12 hues if needed
                double hPrime = int(round(palet[nb_palet].H * 12.0)) % 12; // get a rounded value in [0..11]
                palet[nb_palet].H = hPrime / 12.0; // rewrite rounded value to palette
            }
            nb_palet++; // temp palette index
        }

    // draw hues on wheel external circle
    for (int n = 0; n < nb_palet; n++) { // parse temp palette
        long double S = 1; // max saturation and normal lightness
        long double L = 0.5;
        long double R, G, B;
        HSLtoRGB(palet[n].H, S, L, R, G, B); // convert maxed current hue to RGB
        DrawOnWheelBorder(int(round(R * 255.0)), int(round(G * 255.0)), int(round(B * 255.0)), 10, true); // draw a black dot on external circle of wheel
    }

    long double H_max = 0;
    // compute angles between dots
    for (int x = 0; x < nb_palet; x++) { // populate double-entry angles array
        for (int y = 0; y < nb_palet; y++)
            if (x !=y) { // no angle between a dot and itself !
                angles[x][y] = Angle::DifferenceDeg(Angle::NormalizedToDeg(palet[x].H), Angle::NormalizedToDeg(palet[y].H)); // angle between 2 dots

                if (angles[x][y] > H_max)
                        H_max = angles[x][y];
            }
            else
                angles[x][y] = -1000.0; // dummy value
    }

    bool complementaryFound = false, analogousFound = false, triadicFound = false,
            splitComplementaryFound = false, tetradicFound = false, squareFound = false; // indicators of found color schemes

    long double colorRadius1, colorRadius2, colorRadius3, colorRadius4; // radius of color dots on the wheel
    for (int x = 0; x < nb_palet; x++) { // parse the angles array
        for (int y = 0; y < nb_palet; y++) {
            if (abs(180.0 - angles[x][y]) <= 25) { // complementary : a 180° angle
                // first dot coordinates
                if (ui->checkBox_color_borders->isChecked()) // draw on dot or external circle ?
                    colorRadius1 = wheel_radius; // external circle
                else
                    colorRadius1 = (long double)(wheel_radius_center) * palet[x].L; // color distance from center
                long double angle1 = -Angle::NormalizedToRad(palet[x].H + 0.25); // angle convert normalized value to radians + shift to have red on top
                long double xOffset1 = wheel_center.x + cosf(angle1) * colorRadius1; // position from center of circle
                long double yOffset1 = wheel_center.y + sinf(angle1) * colorRadius1;
                // second dot coordinates
                if (ui->checkBox_color_borders->isChecked())
                    colorRadius2 = wheel_radius;
                else
                    colorRadius2 = (long double)(wheel_radius_center) * palet[y].L; // color distance from center
                long double angle2 = -Angle::NormalizedToRad(palet[y].H + 0.25); // angle convert normalized value to radians
                long double xOffset2 = wheel_center.x + cosf(angle2) * colorRadius2; // position from center of circle
                long double yOffset2 = wheel_center.y + sinf(angle2) * colorRadius2;

                // draw a line between the two dots : red with white inside
                cv::line(wheel, cv::Point(xOffset1, yOffset1), cv::Point(xOffset2, yOffset2), cv::Vec3b(0, 0, 255), 5, cv::LINE_AA);
                cv::line(wheel, cv::Point(xOffset1, yOffset1), cv::Point(xOffset2, yOffset2), cv::Vec3b(255, 255, 255), 1, cv::LINE_AA);
                complementaryFound = true; // color scheme found !
            }
            if ((x != y) and (abs(30.0 - angles[x][y]) < 15)) { // analogous : 3 dots, separated by ~30°
                // no need to add much comments now, it is all the same than before
                // first dot coordinates
                if (ui->checkBox_color_borders->isChecked())
                    colorRadius1 = wheel_radius;
                else
                    colorRadius1 = (long double)(wheel_radius_center) * palet[x].L; // color distance from center
                long double angle1 = -Angle::NormalizedToRad(palet[x].H + 0.25); // angle convert normalized value to radians + shift to have red on top
                long double xOffset1 = wheel_center.x + cosf(angle1) * colorRadius1; // position from center of circle
                long double yOffset1 = wheel_center.y + sinf(angle1) * colorRadius1;
                // second dot coordinates
                if (ui->checkBox_color_borders->isChecked())
                    colorRadius2 = wheel_radius;
                else
                    colorRadius2 = (long double)(wheel_radius_center) * palet[y].L; // color distance from center
                long double angle2 = -Angle::NormalizedToRad(palet[y].H + 0.25); // angle convert normalized value to radians + shift to have red on top
                long double xOffset2 = wheel_center.x + cosf(angle2) * colorRadius2; // position from center of circle
                long double yOffset2 = wheel_center.y + sinf(angle2) * colorRadius2;

                for (int z = 0; z < nb_palet; z++) // look for 3rd dot
                    if ((z != x) and (z != y) and (abs(30.0 - angles[y][z]) < 15) and (angles[x][z] > 45)) { // with the angle : ~30°
                        // 3rd dot coordinates
                        if (ui->checkBox_color_borders->isChecked())
                            colorRadius3 = wheel_radius;
                        else
                            colorRadius3 = (long double)(wheel_radius_center) * palet[z].L; // color distance from center
                        long double angle3 = -Angle::NormalizedToRad(palet[z].H + 0.25); // angle convert normalized value to radians + shift to have red on top
                        long double xOffset3 = wheel_center.x + cosf(angle3) * colorRadius3; // position from center of circle
                        long double yOffset3 = wheel_center.y + sinf(angle3) * colorRadius3;

                        // draw lines between the 3 dots : green with white inside
                        cv::line(wheel, cv::Point(xOffset1, yOffset1), cv::Point(xOffset2, yOffset2), cv::Vec3b(0, 255, 0), 5, cv::LINE_AA);
                        cv::line(wheel, cv::Point(xOffset2, yOffset2), cv::Point(xOffset3, yOffset3), cv::Vec3b(0, 255, 0), 5, cv::LINE_AA);
                        cv::line(wheel, cv::Point(xOffset1, yOffset1), cv::Point(xOffset2, yOffset2), cv::Vec3b(255, 255, 255), 1, cv::LINE_AA);
                        cv::line(wheel, cv::Point(xOffset2, yOffset2), cv::Point(xOffset3, yOffset3), cv::Vec3b(255, 255, 255), 1, cv::LINE_AA);
                        analogousFound = true;
                    }
            }
            if (abs(120.0 - angles[x][y]) <= 25) { // triadic : 3 dots equally distanced => angle = 120°
                // 1st dot coordinates
                if (ui->checkBox_color_borders->isChecked())
                    colorRadius1 = wheel_radius;
                else
                    colorRadius1 = (long double)(wheel_radius_center) * palet[x].L; // color distance from center
                long double angle1 = -Angle::NormalizedToRad(palet[x].H + 0.25); // angle convert normalized value to radians + shift to have red on top
                long double xOffset1 = wheel_center.x + cosf(angle1) * colorRadius1; // position from center of circle
                long double yOffset1 = wheel_center.y + sinf(angle1) * colorRadius1;
                // 2nd dot coordinates
                if (ui->checkBox_color_borders->isChecked())
                    colorRadius2 = wheel_radius;
                else
                    colorRadius2 = (long double)(wheel_radius_center) * palet[y].L; // color distance from center
                long double angle2 = -Angle::NormalizedToRad(palet[y].H + 0.25); // angle convert normalized value to radians + shift to have red on top
                long double xOffset2 = wheel_center.x + cosf(angle2) * colorRadius2; // position from center of circle
                long double yOffset2 = wheel_center.y + sinf(angle2) * colorRadius2;

                for (int z = 0; z < nb_palet; z++) // look for 3rd dot
                    if ((z != x) and (z != y) and (abs(120.0 - angles[y][z]) <= 25) and (angles[x][z] > 95)) { // angle = 120°
                        // 3rd dot coordinates
                        if (ui->checkBox_color_borders->isChecked())
                            colorRadius3 = wheel_radius;
                        else
                            colorRadius3 = (long double)(wheel_radius_center) * palet[z].L; // color distance from center
                        long double angle3 = -Angle::NormalizedToRad(palet[z].H + 0.25); // angle convert normalized value to radians + shift to have red on top
                        long double xOffset3 = wheel_center.x + cosf(angle3) * colorRadius3; // position from center of circle
                        long double yOffset3 = wheel_center.y + sinf(angle3) * colorRadius3;

                        // draw lines between the 3 dots : blue with white inside
                        cv::line(wheel, cv::Point(xOffset1, yOffset1), cv::Point(xOffset2, yOffset2), cv::Vec3b(255, 0, 0), 5, cv::LINE_AA);
                        cv::line(wheel, cv::Point(xOffset1, yOffset1), cv::Point(xOffset3, yOffset3), cv::Vec3b(255, 0, 0), 5, cv::LINE_AA);
                        cv::line(wheel, cv::Point(xOffset3, yOffset3), cv::Point(xOffset2, yOffset2), cv::Vec3b(255, 0, 0), 5, cv::LINE_AA);
                        cv::line(wheel, cv::Point(xOffset1, yOffset1), cv::Point(xOffset2, yOffset2), cv::Vec3b(255, 255, 255), 1, cv::LINE_AA);
                        cv::line(wheel, cv::Point(xOffset1, yOffset1), cv::Point(xOffset3, yOffset3), cv::Vec3b(255, 255, 255), 1, cv::LINE_AA);
                        cv::line(wheel, cv::Point(xOffset3, yOffset3), cv::Point(xOffset2, yOffset2), cv::Vec3b(255, 255, 255), 1, cv::LINE_AA);
                        triadicFound = true;
                    }
            }
            if (abs(60.0 - angles[x][y]) <= 25) { // split-complementary : one dot with two opposites separated by ~60°
                // 1st dot coordinates
                if (ui->checkBox_color_borders->isChecked())
                    colorRadius1 = wheel_radius;
                else
                    colorRadius1 = (long double)(wheel_radius_center) * palet[x].L; // color distance from center
                long double angle1 = -Angle::NormalizedToRad(palet[x].H + 0.25); // angle convert normalized value to radians + shift to have red on top
                long double xOffset1 = wheel_center.x + cosf(angle1) * colorRadius1; // position from center of circle
                long double yOffset1 = wheel_center.y + sinf(angle1) * colorRadius1;
                // 2nd dot coordinates
                if (ui->checkBox_color_borders->isChecked())
                    colorRadius2 = wheel_radius;
                else
                    colorRadius2 = (long double)(wheel_radius_center) * palet[y].L; // color distance from center
                long double angle2 = -Angle::NormalizedToRad(palet[y].H + 0.25); // angle convert normalized value to radians + shift to have red on top
                long double xOffset2 = wheel_center.x + cosf(angle2) * colorRadius2; // position from center of circle
                long double yOffset2 = wheel_center.y + sinf(angle2) * colorRadius2;

                for (int z = 0; z < nb_palet; z++) // looi for 3rd dot
                    if ((z != x) and (z != y) and (abs(150.0 - angles[y][z]) <= 15) and (angles[x][z] > 130)) { // this time with angle ~150°
                        // 3rd dot coordinates
                        if (ui->checkBox_color_borders->isChecked())
                            colorRadius3 = wheel_radius;
                        else
                            colorRadius3 = (long double)(wheel_radius_center) * palet[z].L; // color distance from center
                        long double angle3 = -Angle::NormalizedToRad(palet[z].H + 0.25); // angle convert normalized value to radians + shift to have red on top
                        long double xOffset3 = wheel_center.x + cosf(angle3) * colorRadius3; // position from center of circle
                        long double yOffset3 = wheel_center.y + sinf(angle3) * colorRadius3;

                        // draw lines between the dots : cyan with black inside
                        cv::line(wheel, cv::Point(xOffset1, yOffset1), cv::Point(xOffset2, yOffset2), cv::Vec3b(255, 255, 0), 5, cv::LINE_AA);
                        cv::line(wheel, cv::Point(xOffset1, yOffset1), cv::Point(xOffset3, yOffset3), cv::Vec3b(255, 255, 0), 5, cv::LINE_AA);
                        cv::line(wheel, cv::Point(xOffset3, yOffset3), cv::Point(xOffset2, yOffset2), cv::Vec3b(255, 255, 0), 5, cv::LINE_AA);
                        cv::line(wheel, cv::Point(xOffset1, yOffset1), cv::Point(xOffset2, yOffset2), cv::Vec3b(0, 0, 0), 1, cv::LINE_AA);
                        cv::line(wheel, cv::Point(xOffset1, yOffset1), cv::Point(xOffset3, yOffset3), cv::Vec3b(0, 0, 0), 1, cv::LINE_AA);
                        cv::line(wheel, cv::Point(xOffset3, yOffset3), cv::Point(xOffset2, yOffset2), cv::Vec3b(0, 0, 0), 1, cv::LINE_AA);
                        splitComplementaryFound = true;
                    }
            }
            if (abs(60.0 - angles[x][y]) <= 25) { // tetradic : a rectangle of 2 pairs of complementary dots separated by ~60°
                // 1st dot coordinates
                if (ui->checkBox_color_borders->isChecked())
                    colorRadius1 = wheel_radius;
                else
                    colorRadius1 = (long double)(wheel_radius_center) * palet[x].L; // color distance from center
                long double angle1 = -Angle::NormalizedToRad(palet[x].H + 0.25); // angle convert normalized value to radians + shift to have red on top
                long double xOffset1 = wheel_center.x + cosf(angle1) * colorRadius1; // position from center of circle
                long double yOffset1 = wheel_center.y + sinf(angle1) * colorRadius1;
                // 2nd dot coordinatescv::LINE_AA
                if (ui->checkBox_color_borders->isChecked())
                    colorRadius2 = wheel_radius;
                else
                    colorRadius2 = (long double)(wheel_radius_center) * palet[y].L; // color distance from center
                long double angle2 = -Angle::NormalizedToRad(palet[y].H + 0.25); // angle convert normalized value to radians + shift to have red on top
                long double xOffset2 = wheel_center.x + cosf(angle2) * colorRadius2; // position from center of circle
                long double yOffset2 = wheel_center.y + sinf(angle2) * colorRadius2;

                for (int z = 0; z < nb_palet; z++) // look for 3rd dot
                    if ((z != x) and (z != y) and (abs(120.0 - angles[y][z]) <= 25) and (angles[z][x] > 65)) { // with an angle ~120°
                        // 3rd dot coordinates
                        if (ui->checkBox_color_borders->isChecked())
                            colorRadius3 = wheel_radius;
                        else
                            colorRadius3 = (long double)(wheel_radius_center) * palet[z].L; // color distance from center
                        long double angle3 = -Angle::NormalizedToRad(palet[z].H + 0.25); // angle convert normalized value to radians + shift to have red on top
                        long double xOffset3 = wheel_center.x + cosf(angle3) * colorRadius3; // position from center of circle
                        long double yOffset3 = wheel_center.y + sinf(angle3) * colorRadius3;

                        for (int w = 0; w < nb_palet; w++) // look for 4th dot
                            if ((w != x) and (w != y) and (w != z) and (abs(60.0 - angles[z][w]) <= 25) and (angles[x][w] > 65)) { // with an angle ~60°
                                // 4th dot coordinates
                                if (ui->checkBox_color_borders->isChecked())
                                    colorRadius4 = wheel_radius;
                                else
                                    colorRadius4 = (long double)(wheel_radius_center) * palet[w].L; // color distance from center
                                long double angle4 = -Angle::NormalizedToRad(palet[w].H + 0.25); // angle convert normalized value to radians + shift to have red on top
                                long double xOffset4 = wheel_center.x + cosf(angle4) * colorRadius4; // position from center of circle
                                long double yOffset4 = wheel_center.y + sinf(angle4) * colorRadius4;

                                // draw lines between the dots : violet with white inside
                                cv::line(wheel, cv::Point(xOffset1, yOffset1), cv::Point(xOffset2, yOffset2), cv::Vec3b(255, 0, 255), 5, cv::LINE_AA);
                                cv::line(wheel, cv::Point(xOffset2, yOffset2), cv::Point(xOffset3, yOffset3), cv::Vec3b(255, 0, 255), 5, cv::LINE_AA);
                                cv::line(wheel, cv::Point(xOffset3, yOffset3), cv::Point(xOffset4, yOffset4), cv::Vec3b(255, 0, 255), 5, cv::LINE_AA);
                                cv::line(wheel, cv::Point(xOffset4, yOffset4), cv::Point(xOffset1, yOffset1), cv::Vec3b(255, 0, 255), 5, cv::LINE_AA);
                                cv::line(wheel, cv::Point(xOffset1, yOffset1), cv::Point(xOffset2, yOffset2), cv::Vec3b(255, 255, 255), 1, cv::LINE_AA);
                                cv::line(wheel, cv::Point(xOffset2, yOffset2), cv::Point(xOffset3, yOffset3), cv::Vec3b(255, 255, 255), 1, cv::LINE_AA);
                                cv::line(wheel, cv::Point(xOffset3, yOffset3), cv::Point(xOffset4, yOffset4), cv::Vec3b(255, 255, 255), 1, cv::LINE_AA);
                                cv::line(wheel, cv::Point(xOffset4, yOffset4), cv::Point(xOffset1, yOffset1), cv::Vec3b(255, 255, 255), 1, cv::LINE_AA);
                                tetradicFound = true;
                            }
                    }
            }
            if (abs(90.0 - angles[x][y]) <= 25) { // square : almost the same as before, but all angles are equal ~90°
                // 1st dot coordinates
                if (ui->checkBox_color_borders->isChecked())
                    colorRadius1 = wheel_radius;
                else
                    colorRadius1 = (long double)(wheel_radius_center) * palet[x].L; // color distance from center
                long double angle1 = -Angle::NormalizedToRad(palet[x].H + 0.25); // angle convert normalized value to radians + shift to have red on top
                long double xOffset1 = wheel_center.x + cosf(angle1) * colorRadius1; // position from center of circle
                long double yOffset1 = wheel_center.y + sinf(angle1) * colorRadius1;
                // 2nd dot coordinates
                if (ui->checkBox_color_borders->isChecked())
                    colorRadius2 = wheel_radius;
                else
                    colorRadius2 = (long double)(wheel_radius_center) * palet[y].L; // color distance from center
                long double angle2 = -Angle::NormalizedToRad(palet[y].H + 0.25); // aangle convert normalized value to radians + shift to have red on top
                long double xOffset2 = wheel_center.x + cosf(angle2) * colorRadius2; // position from center of circle
                long double yOffset2 = wheel_center.y + sinf(angle2) * colorRadius2;

                for (int z = 0; z < nb_palet; z++) // look for 3rd dot
                    if ((z != x) and (z != y) and (abs(90.0 - angles[y][z]) <= 25) and (angles[z][x] > 65)) { // angle ~90°
                        // 3rd dot coordinates
                        if (ui->checkBox_color_borders->isChecked())
                            colorRadius3 = wheel_radius;
                        else
                            colorRadius3 = (long double)(wheel_radius_center) * palet[z].L; // color distance from center
                        long double angle3 = -Angle::NormalizedToRad(palet[z].H + 0.25); // angle convert normalized value to radians + shift to have red on top
                        long double xOffset3 = wheel_center.x + cosf(angle3) * colorRadius3; // position from center of circle
                        long double yOffset3 = wheel_center.y + sinf(angle3) * colorRadius3;

                        for (int w = 0; w < nb_palet; w++) // look for 4th dot
                            if ((w != x) and (w != y) and (w != z) and (abs(90.0 - angles[z][w]) <= 25) and (angles[x][w] > 65)) { // angle ~90°
                                // 4th dot coordinates
                                if (ui->checkBox_color_borders->isChecked())
                                    colorRadius4 = wheel_radius;
                                else
                                    colorRadius4 = (long double)(wheel_radius_center) * palet[w].L; // color distance from center
                                long double angle4 = -Angle::NormalizedToRad(palet[w].H + 0.25); // angle convert normalized value to radians + shift to have red on top
                                long double xOffset4 = wheel_center.x + cosf(angle4) * colorRadius4; // position from center of circle
                                long double yOffset4 = wheel_center.y + sinf(angle4) * colorRadius4;

                                // draw lines between the dots : orange with white inside
                                cv::line(wheel, cv::Point(xOffset1, yOffset1), cv::Point(xOffset2, yOffset2), cv::Vec3b(0, 127, 255), 5, cv::LINE_AA);
                                cv::line(wheel, cv::Point(xOffset2, yOffset2), cv::Point(xOffset3, yOffset3), cv::Vec3b(0, 127, 255), 5, cv::LINE_AA);
                                cv::line(wheel, cv::Point(xOffset3, yOffset3), cv::Point(xOffset4, yOffset4), cv::Vec3b(0, 127, 255), 5, cv::LINE_AA);
                                cv::line(wheel, cv::Point(xOffset4, yOffset4), cv::Point(xOffset1, yOffset1), cv::Vec3b(0, 127, 255), 5, cv::LINE_AA);
                                cv::line(wheel, cv::Point(xOffset1, yOffset1), cv::Point(xOffset2, yOffset2), cv::Vec3b(0, 0, 0), 1, cv::LINE_AA);
                                cv::line(wheel, cv::Point(xOffset2, yOffset2), cv::Point(xOffset3, yOffset3), cv::Vec3b(0, 0, 0), 1, cv::LINE_AA);
                                cv::line(wheel, cv::Point(xOffset3, yOffset3), cv::Point(xOffset4, yOffset4), cv::Vec3b(0, 0, 0), 1, cv::LINE_AA);
                                cv::line(wheel, cv::Point(xOffset4, yOffset4), cv::Point(xOffset1, yOffset1), cv::Vec3b(0, 0, 0), 1, cv::LINE_AA);
                                squareFound = true;
                            }
                    }
            }
        }
    }

    // check found color schemes in UI
    ui->pushButton_color_analogous->setChecked(analogousFound);
    ui->pushButton_color_complementary->setChecked(complementaryFound);
    ui->pushButton_color_split_complementary->setChecked(splitComplementaryFound);
    ui->pushButton_color_square->setChecked(squareFound);
    ui->pushButton_color_tetradic->setChecked(tetradicFound);
    ui->pushButton_color_triadic->setChecked(triadicFound);
    ui->label_color_analogous->setVisible(analogousFound);
    ui->label_color_complementary->setVisible(complementaryFound);
    ui->label_color_split_complementary->setVisible(splitComplementaryFound);
    ui->label_color_square->setVisible(squareFound);
    ui->label_color_tetradic->setVisible(tetradicFound);
    ui->label_color_triadic->setVisible(triadicFound);
    if (!(analogousFound or complementaryFound or splitComplementaryFound or squareFound or tetradicFound or triadicFound)) {
        if (H_max <= 40)
            ui->pushButton_color_monochromatic->setChecked(true);
    }

    // cold and warm colors, blacks whites and grays, color stats. Here we work on the original image without filters
    cv::Vec3b RGB;
    long double H, S, L;
    int countCold = 0; // number of "cold" pixels
    int countWarm = 0; // number of "warm" pixels
    int countNeutralPlus = 0; // number of "neutral+" pixels (neutral but a bit "warm")
    int countNeutralMinus = 0; // number of "neutral-" pixels (neutral but a bit "warm")
    int countColors = 0; // number of "colored" pixels
    int countBlack = 0; // number of "black" pixels
    int countGray = 0; // number of "gray" pixels
    int countWhite = 0; // number of "white" pixels
    double countP = 0; // sum of perceived brightness
    int countAll = image.rows * image.cols; // total number of pixels in image
    int stats[12] = {0}; // count of 12 principal hues in wheel

    for (int x = 0; x < image.cols; x++) // parse image
        for (int y = 0; y < image.rows; y++) {
            RGB = image.at<cv::Vec3b>(y, x); // get current color
            HSLfromRGB((long double)(RGB[2]) / 255.0, (long double)(RGB[1]) / 255.0, (long double)(RGB[0]) / 255.0, H, S, L); // HSL pixel value
            int hPrime = int(round(H * 12.0)) % 12; // get a value in [0..11]
            H = Angle::NormalizedToDeg(H); // in degrees
            double P = PerceivedBrightnessRGB(double(RGB[2]) / 255.0, double(RGB[1]) / 255.0, double(RGB[0]) / 255.0); // percieved brightness
            countP += P; // total Perceived brightness
            long double dBlack = DistanceFromBlackRGB((long double)(RGB[2]) / 255.0, (long double)(RGB[1]) / 255.0, (long double)(RGB[0]) / 255.0); // distanes from black, white and gray
            long double dWhite = DistanceFromWhiteRGB((long double)(RGB[2]) / 255.0, (long double)(RGB[1]) / 255.0, (long double)(RGB[0]) / 255.0);
            long double dGray = DistanceFromGrayRGB((long double)(RGB[2]) / 255.0, (long double)(RGB[1]) / 255.0, (long double)(RGB[0]) / 255.0);

            if (dBlack < blacksLimit) { // black is considered cold
                countCold++;
            }
            else { // color is not in blacks
                if ((dWhite > whitesLimit) and (dGray > graysLimit)) { // is it really a color ?
                    countColors++; // one more color
                    stats[hPrime]++; // one more color in 12 colors stats
                }
                // cold/warm
                if ((dWhite > 5) and (dGray > 5)) { // nor too gray or too white
                    if ((H > 80) and (H <= 150)) // neutral+
                        countNeutralPlus++;
                    else
                    if ((H > 150) and (H <= 270)) // cold
                        countCold++;
                    else
                    if ((H > 270) and (H <= 330)) // neutral-
                        countNeutralMinus++;
                    else
                        countWarm++;
                }
                else { // too white or too gray
                    countCold++;
                }
            }

            // increase black or white or gray counter
            if (dBlack < blacksLimit) // blacks
                countBlack++;
            else if (dWhite < whitesLimit) // whites
                countWhite++;
            else if (dGray < graysLimit) // neutrals
                countGray++;
        }

    // cold/warm : compared to colored pixels only
    int maximum=std::max(std::max(std::max(countWarm,countCold), countNeutralPlus),countNeutralMinus); // which cold/warm value is the highest ?
    QString coldAndWarm; // for display
    if (countCold == maximum) { // more cold colors : cyan on dark blue background
        if (double(countCold) / countAll * 100 > 70)
            coldAndWarm = "Cold ";
        else
            coldAndWarm = "Cool ";
        ui->label_color_cold_warm->setStyleSheet("QLabel{color:cyan;background-color:rgb(0,0,128);border: 2px inset #8f8f91;}");
        ui->label_color_cold_warm->setText(coldAndWarm + QString::number(double(countCold) / countAll * 100, 'f', 1) + "%");

    }
    else if (countWarm == maximum) { // more warm colors : orange on dark red background
        if (double(countWarm) / countAll * 100 > 70)
            coldAndWarm = "Hot ";
        else
            coldAndWarm = "Warm ";
        ui->label_color_cold_warm->setStyleSheet("QLabel{color:orange;background-color:rgb(128,0,0);border: 2px inset #8f8f91;}");
        ui->label_color_cold_warm->setText(coldAndWarm + QString::number(double(countWarm) / countAll * 100, 'f', 1) + "%");
    }
    else if (countNeutralPlus == maximum) { // more neutral+ colors : yellow on gray background
        coldAndWarm = "Neutral+ ";
        ui->label_color_cold_warm->setStyleSheet("QLabel{color:yellow;background-color:rgb(128,128,128);border: 2px inset #8f8f91;}");
        ui->label_color_cold_warm->setText(coldAndWarm + QString::number(double(countNeutralPlus) / countAll * 100, 'f', 1) + "%");
    }
    else { // more neutral- colors : purple on gray background
        coldAndWarm = "Neutral- ";
        ui->label_color_cold_warm->setStyleSheet("QLabel{color:purple;background-color:rgb(128,128,128);border: 2px inset #8f8f91;}");
        ui->label_color_cold_warm->setText(coldAndWarm + QString::number(double(countNeutralMinus) / countAll * 100, 'f', 1) + "%");
    }

    // grays : compared to whole picture colors (colors + grays)
    maximum=std::max(std::max(countGray,countBlack), countGray); // maximum value of grays
    if (maximum == 0) { // no gray or black or white colors in image : red on gray background
        ui->label_color_blacks->setStyleSheet("QLabel{color:rgb(255,0,0);background-color:rgb(64,64,64);border: 2px inset #8f8f91;text-align: center;}");
        ui->label_color_blacks->setText("- - - - - -"); // no blacks
        ui->label_color_whites->setStyleSheet("QLabel{color:rgb(255,0,0);background-color:rgb(64,64,64);border: 2px inset #8f8f91;text-align: center;}");
        ui->label_color_whites->setText("- - - - - -"); // no whites
        ui->label_color_grays->setStyleSheet("QLabel{color:rgb(255,0,0);background-color:rgb(64,64,64);border: 2px inset #8f8f91;text-align: center;}");
        ui->label_color_grays->setText("- - - - - -"); // no grays
    }
    else { // blacks whites and gray values
        ui->label_color_blacks->setStyleSheet("QLabel{color:rgb(224,224,224);background-color:rgb(64,64,64);border: 2px inset #8f8f91;text-align: center;}");
        ui->label_color_blacks->setText("Dark " + QString::number(double(countBlack) / countAll * 100.0, 'f', 1) + "%");
        ui->label_color_whites->setStyleSheet("QLabel{color:grey;background-color:rgb(255,255,255);border: 2px inset #8f8f91;text-align: center;}");
        ui->label_color_whites->setText("Bright " + QString::number(double(countWhite) / countAll * 100.0, 'f', 1) + "%");
        ui->label_color_grays->setStyleSheet("QLabel{color:rgb(224,224,224);background-color:rgb(128,128,128);border: 2px inset #8f8f91;text-align: center;}");
        ui->label_color_grays->setText("Neutral " + QString::number(double(countGray) / countAll * 100.0, 'f', 1) + "%");
    }

    // colors
    if (countColors > 0) { // colors found ?
        cv::Mat tempQuantized = ImgRGBtoLab(palette);
        std::vector<cv::Vec3f> palette_vec = DominantColorsEigenCIELab(tempQuantized, 1, tempQuantized); // get dominant palette, palette image and quantized image for 1 color only
        long double L = palette_vec[0][0]; // CIELab value of global color
        long double a = palette_vec[0][1];
        long double b = palette_vec[0][2];
        long double X, Y, Z, r, g;
        // convert CIELAb value to RGB
        LABtoXYZ(L, a, b, X, Y, Z);
        XYZtoRGB(X, Y, Z, r, g, b);
        int R = round(r * 255.0);
        int G = round(g * 255.0);
        int B = round(b * 255.0);

        cv::Vec3b textColor;
        if (IsRGBColorDark(R, G, B)) // text color and background must be in accordance
            textColor = cv::Vec3b(255, 255, 255); // dark -> white text
        else
            textColor = cv::Vec3b(0, 0, 0); // bright -> dark text
        ui->label_color_colored->setStyleSheet("QLabel{background-color:rgb("
                                               + QString::number(R) + "," + QString::number(G) + "," + QString::number(B) +
                                               ");color:rgb("
                                               + QString::number(textColor[2]) + "," + QString::number(textColor[1]) + "," + QString::number(textColor[0])
                                               +");border: 2px inset #8f8f91;}"); // background = global color
        ui->label_color_colored->setText("Colors " + QString::number(double(countColors) / countAll * 100.0, 'f', 1) + "%"); // colors value
    }
    else { // no colors to display
        ui->label_color_colored->setStyleSheet("QLabel{background-color:rgb(64,64,64);color:rgb(255,0,0);border: 2px inset #8f8f91;}"); // red on dark gray background
        ui->label_color_colored->setText("- - - - - -"); // no colors
    }

    // perceived brightness
    QString brightness;
    if (countP / countAll > 0.6) { // is the image bright ?
        brightness = "Bright ";
        ui->label_color_brightness->setStyleSheet("QLabel{background-color:rgb(255,255,255);color:rgb(128,128,128);border: 2px inset #8f8f91;}"); // gray on white background
    } else
        if (countP / countAll < 0.20) { // or is it dark ?
            brightness = "Dark ";
            ui->label_color_brightness->setStyleSheet("QLabel{background-color:rgb(0,0,0);color:rgb(164,164,164);border: 2px inset #8f8f91;}"); // gray on black background
        } else {
            brightness = "Normal "; // if not, it is normally bright
            ui->label_color_brightness->setStyleSheet("QLabel{background-color:rgb(128,128,128);color:rgb(224,224,224);border: 2px inset #8f8f91;}"); // whitish on gray background
        }
    ui->label_color_brightness->setText(brightness + QString::number(countP / countAll * 100.0, 'f', 1) + "%"); // show result

    // graph
    if (countColors > 0) { // colors found ?
        ui->color_graph->setVisible(true); // graph is visible
        int w = ui->color_graph->width(); // width and height of graph
        int h = ui->color_graph->height(); // vertical scale for bars
        int margin = 5; // graph variables
        int zero = h - margin;
        int size_h = h - 4 * margin;
        QPixmap pic(w, h); // surface on which to draw
        pic.fill(Qt::lightGray); // fill it with ligh gray first
        QPainter painter(&pic); // painter
        painter.setRenderHint(QPainter::Antialiasing); // with antialias

        double stat_max = double(*std::max_element(stats, stats + 12)); // vertical scale
        for (int n = 0; n < 12; n++) { // foir each principal color
            long double r, g, b;
            HSLtoRGB(n / 12.0, 1, 0.5, r, g, b); // get the hue
            painter.fillRect(QRectF(margin + 7 + n * 13, zero - 1, 12, -(round(double(stats[n]) / stat_max * double(size_h)))),
                             QColor(round(r * 255.0), round(g * 255.0), round(b * 255.0))); // draw color bar
        }

        // draw axes
        painter.setPen(QPen(Qt::black, 2)); // in black
        painter.drawLine(QPointF(margin, zero), QPointF(w - margin, zero));
        painter.drawLine(QPointF(margin, h - margin), QPointF(margin, margin));

        ui->color_graph->setPixmap(pic); // show graph
    }
    else { // no colors to display
        ui->color_graph->setVisible(false); // no graph
    }

    // show Analyze frame and Color wheel
    ui->frame_analysis->setVisible(true); // show analysis frame
    ui->label_wheel->setPixmap(Mat2QPixmap(wheel)); // update wheel view

    ShowTimer(false); // show elapsed time
    QApplication::restoreOverrideCursor(); // Restore cursor
}

/////////////////// Mouse events //////////////////////

void MainWindow::mousePressEvent(QMouseEvent *eventPress) // event triggered by a mouse click
{
    mouseButton = eventPress->button(); // mouse button value

    bool color_found = false; // valid color found ?
    cv::Vec3b color; // BGR values of picked color

    if (ui->label_wheel->underMouse()) { // mouse over color wheel ?
        mouse_pos = ui->label_wheel->mapFromGlobal(QCursor::pos()); // mouse position

        if ((mouseButton == Qt::LeftButton) and (!wheel.empty())) { // mouse left button clicked
            color = wheel.at<cv::Vec3b>(mouse_pos.y(), mouse_pos.x()); // get BGR "color" under mouse cursor
            color_found = true; // found !
        }
    }

    if (ui->label_palette->underMouse()) { // mouse over palette ?
        mouse_pos = ui->label_palette->mapFromGlobal(QCursor::pos()); // mouse position

        if ((mouseButton == Qt::LeftButton) and (!palette.empty())) { // mouse left button clicked
            const QPixmap* q = ui->label_palette->pixmap(); // stored reduced quantized image in GUI
            int x = round(palette.cols * double(mouse_pos.x() - (ui->label_palette->width() - q->width()) / 2) / double(q->width())); // real x position in palette image
            int y = round(palette.rows * double(mouse_pos.y() - (ui->label_palette->height() - q->height()) / 2) / double(q->height())); // real y position in palette image
            if ((x > 0) and (x < palette.cols) and (y > 0) and (y < palette.rows)) { // not off-limits ?
                color = palette.at<cv::Vec3b>(0, x); // pick color in palette image
                color_found = true; // found !
            }
        else
            color_found = false; // no color under mouse
        }
    }

    if (ui->label_quantized->underMouse()) { // mouse over quantized image ?
        mouse_pos = ui->label_quantized->mapFromGlobal(QCursor::pos()); // mouse position

        if ((mouseButton == Qt::LeftButton) and (!quantized.empty())) { // mouse left button clicked
            const QPixmap* q = ui->label_quantized->pixmap(); // stored reduced quantized image in GUI

            double percentX = double(mouse_pos.x() - (ui->label_quantized->width() - q->width()) / 2) / double(q->width()); // real x and y position in quantized image
            double percentY = double(mouse_pos.y() - (ui->label_quantized->height() - q->height()) / 2) / double(q->height());

            if ((percentX >= 0) and (percentX < 1) and (percentY >= 0) and (percentY < 1)) { // not off-limits ?
                color = quantized.at<cv::Vec3b>(round(percentY * quantized.rows), round(percentX * quantized.cols)); // pick color in quantized image at x,y
                color_found = true; // found !
            }
        }
    }

    if (color_found) { // color picked ?
        // RGB values
        int R = color[2];
        int G = color[1];
        int B = color[0];

        // find color in palette
        bool found = false; // picked color found in palette ?
        for (int n = 0; n < nb_palettes; n++) { // search in palette
            if ((palettes[n].R == R) and (palettes[n].G == G) and (palettes[n].B == B)) { // identical RGB values found ?
                QString value = QString::number(palettes[n].percentage * 100, 'f', 2) + "%"; // picked color percentage in quantized image
                ui->label_color_percentage->setText(value); // display percentage
                ui->label_color_name->setText(palettes[n].name); // display name
                ui->label_color_hex->setText(QString::fromStdString(palettes[n].hexa)); // show hexa

                // display color, RGB values
                cv::Mat bar = cv::Mat::zeros(cv::Size(1,1), CV_8UC3); // 1 pixel image
                bar = cv::Vec3b(B,G,R); // set it to picked color
                ui->label_color_bar->setPixmap(Mat2QPixmapResized(bar, ui->label_color_bar->width(), ui->label_color_bar->height(), false)); // show picked color
                ui->label_color_r->setText(QString::number(R)); // show RGB values
                ui->label_color_g->setText(QString::number(G));
                ui->label_color_b->setText(QString::number(B));

                found = true; // color found in palette
                break; // get out of loop
            }
        }
    }
}

void MainWindow::wheelEvent(QWheelEvent *wheelEvent) // mouse wheel turned
{
    if (!computed)
        return;// get out

    int n = wheelEvent->delta(); // amount of wheel turn
    if (n > 0) { // positive = circle size up
        ui->verticalSlider_circle_size->setValue(ui->verticalSlider_circle_size->value() + 1);
    }
    if (n < 0) { // negative = circle size down
        ui->verticalSlider_circle_size->setValue(ui->verticalSlider_circle_size->value() - 1);
    }
}

/////////////////// Save and load //////////////////////

void MainWindow::SaveDirBaseFile() // write current folder name in ini file
{
    cv::FileStorage fs(basedirinifile, cv::FileStorage::WRITE); // open dir ini file for writing
    fs << "BaseDir" << basedir; // write folder name
    fs.release(); // close file
}

void MainWindow::ChangeBaseDir(QString filename) // set base dir and file
{
    basefile = filename.toUtf8().constData(); // base file name and dir are used after to save other files

    // Remove extension if present
    size_t period_idx = basefile.rfind('.');
    if (std::string::npos != period_idx)
        basefile.erase(period_idx);

    basedir = basefile;
    size_t found = basefile.find_last_of("\\/"); // find last directory
    std::string separator = basefile.substr(found,1); // copy path separator (Linux <> Windows)
    basedir = basedir.substr(0,found) + separator; // define base path
    basefile = basefile.substr(found+1); // delete path in base file

    SaveDirBaseFile(); // Save current path to ini file
}

void MainWindow::on_button_load_image_clicked() // load image to analyze
{
    QString filename = QFileDialog::getOpenFileName(this, "Load image...", QString::fromStdString(basedir),
                                                    "Images (*.jpg *.JPG *.jpeg *.JPEG *.jp2 *.JP2 *.png *.PNG *.tif *.TIF *.tiff *.TIFF *.bmp *.BMP)"); // image file name

    if (filename.isNull() || filename.isEmpty()) // cancel ?
        return;

    ChangeBaseDir(filename); // save current path to ini file

    std::string filesession = filename.toUtf8().constData(); // base file name
    image = cv::imread(filesession); // load image
    if (image.empty()) {
        QMessageBox::critical(this, "File error", "There was a problem reading the image file");
        return;
    }

    loaded = true; // loaded successfully !
    computed = false; // but not yet computed

    ui->label_filename->setText(filename); // display file name in ui

    if (ui->checkBox_gaussian_blur->isChecked()) // gaussian blur ?
        cv::GaussianBlur(image, image, cv::Size(3,3), 0, 0); // blur image
    if (ui->checkBox_reduce_size->isChecked()) // reduce size ?
        if ((image.rows > 512) or (image.cols > 512)) image = ResizeImageAspectRatio(image, cv::Size(512,512)); // resize image

    thumbnail = ResizeImageAspectRatio(image, cv::Size(ui->label_thumbnail->width(),ui->label_thumbnail->height())); // create thumbnail
    quantized.release(); // no quantized image yet
    palette.release(); // no palette image yet

    ShowResults(); // show images in GUI
    nb_palettes = -1; // no palette to show
    ShowWheel(); // display wheel

    // reset GUI elements
    ui->timer->display("-------"); // reset timer
    ui->pushButton_color_analogous->setChecked(false);
    ui->pushButton_color_complementary->setChecked(false);
    ui->pushButton_color_split_complementary->setChecked(false);
    ui->pushButton_color_square->setChecked(false);
    ui->pushButton_color_tetradic->setChecked(false);
    ui->pushButton_color_triadic->setChecked(false);
    ui->pushButton_color_monochromatic->setChecked(false);
    ui->label_color_analogous->setVisible(false);
    ui->label_color_complementary->setVisible(false);
    ui->label_color_split_complementary->setVisible(false);
    ui->label_color_square->setVisible(false);
    ui->label_color_tetradic->setVisible(false);
    ui->label_color_triadic->setVisible(false);
    ui->label_color_cold_warm->setText("");
    ui->label_color_cold_warm->setStyleSheet("QLabel{color:blue;background-color:rgb(220,220,220);border: 2px inset #8f8f91;}");
    ui->label_color_grays->setText("");
    ui->label_color_grays->setStyleSheet("QLabel{color:blue;background-color:rgb(220,220,220);border: 2px inset #8f8f91;}");
    ui->spinBox_nb_palettes->setStyleSheet("QSpinBox{color:black;background-color: white;}"); // show number of colors in black (in case it was red before)
    ui->label_color_bar->setPixmap(QPixmap()); // reset picked color
    ui->label_color_bar->setText("Pick\nColor");
    ui->label_color_r->setText("R"); // show RGB values
    ui->label_color_g->setText("G");
    ui->label_color_b->setText("B");
    ui->label_color_hex->setText("Hex");
    ui->label_color_percentage->setText("");
    ui->label_color_name->setText("");
    ui->frame_analyze->setVisible(false);
    ui->frame_analysis->setVisible(false);
    ui->frame_rgb->setVisible(false);
}

void MainWindow::on_button_save_clicked() // save dominant colors results
{
    if (!computed) { // nothing loaded yet = get out
        QMessageBox::critical(this, "Nothing to do!", "You have to load then compute before saving the images");
        return;
    }

    QString filename = QFileDialog::getSaveFileName(this, "Save image file", QString::fromStdString(basedir + basefile + ".png"), "PNG (*.png *.PNG)"); // image filename
    if (filename.isNull() || filename.isEmpty()) // cancel ?
        return;

    ChangeBaseDir(filename); // save current path to ini file

    // images : if not empty save with base name + type + .png
    if (!quantized.empty())
        cv::imwrite(basedir + basefile + "-quantized.png", quantized);
    if (!palette.empty())
        cv::imwrite(basedir + basefile + "-palette.png", palette);
    if (!wheel.empty())
        cv::imwrite(basedir + basefile + "-wheel.png", wheel);

    // palette .CSV file (text)
    std::ofstream saveCSV; // file to save
    saveCSV.open(basedir + basefile + "-palette.csv"); // save palette file
    if (saveCSV) { // if successfully open
        saveCSV << "Name (string);R (byte);G (byte);B (byte);hexa;percentage\n"; //header
        for (int n = 0; n < nb_palettes; n++) { // read entire palette
            saveCSV << palettes[n].name.toUtf8().constData() << ";"; // color name
            saveCSV << palettes[n].R << ";"; // save RGB values
            saveCSV << palettes[n].G << ";";
            saveCSV << palettes[n].B << ";";
            saveCSV << palettes[n].hexa << ";"; // save hexa
            saveCSV << palettes[n].percentage << "\n"; // save percentage
        }
        saveCSV.close(); // close text file
    }

    // palette .ACT file (Adobe Photoshop and Illustrator)
    char buffer[771] = {0}; // .ACT files are 772 bytes long
    std::ofstream saveACT (basedir + basefile + "-palette-adobe.act", std::ios::out | std::ios::binary); // open stream

    for (int n = 0; n < nb_palettes; n++) { // all palette values to buffer
        buffer[n * 3 + 0] = palettes[n].R;
        buffer[n * 3 + 1] = palettes[n].G;
        buffer[n * 3 + 2] = palettes[n].B;
    }
    buffer[768] = (unsigned short) nb_palettes; // second last 16-bit value : number of colors in palette
    buffer[770] = (unsigned short) 255; // last 16-bit value : which color is transparency
    saveACT.write(buffer, 772); // write 772 bytes from buffer to file
    saveACT.close(); // close binary file

    // palette .PAL file (text JASC-PAL for PaintShop Pro)
    std::ofstream saveJASC; // file to save
    saveJASC.open(basedir + basefile + "-palette-paintshopro.pal"); // save palette file
    if (saveJASC) { // if successfully open
        saveJASC << "JASC-PAL\n0100\n"; // header
        saveJASC << nb_palettes << "\n"; // number of colors
        for (int n = 0; n < nb_palettes; n++) { // read all palette and write it to file
            saveJASC << palettes[n].R << " ";
            saveJASC << palettes[n].G << " ";
            saveJASC << palettes[n].B << "\n";
        }
        saveJASC.close(); // close text file
    }

    // palette .PAL file (text with CMYK values for CorelDraw)
    std::ofstream saveCOREL; // file to save
    saveCOREL.open(basedir + basefile + "-palette-coreldraw.pal"); // save palette file
    if (saveCOREL) { // if successfully open
        long double C, M, Y, K;
        for (int n = 0; n < nb_palettes; n++) { // read all palette
            RGBtoCMYK(palettes[n].R / 255.0, palettes[n].G / 255.0, palettes[n].B / 255.0, C, M, Y, K); // convert to CMYK
            saveCOREL << '"' << palettes[n].name.toUtf8().constData() << '"' << " "
                      << int(round(C * 100.0)) << " " << int(round(M * 100.0)) << " " << int(round(Y * 100.0)) << " " << int(round(K * 100.0))
                      << "\n"; // write values to file
        }
        saveCOREL.close(); // close text file
    }

    QMessageBox::information(this, "Results saved", "Your results were saved with base file name:\n" + QString::fromStdString(basedir + basefile));
}

/////////////////// Core functions //////////////////////

void MainWindow::ComputePaletteValues(const int n) // compute palette values from RGB for one color
{
    HSLfromRGB((long double)(palettes[n].R / 255.0), (long double)(palettes[n].G / 255.0), (long double)(palettes[n].B / 255.0),
               palettes[n].H, palettes[n].S, palettes[n].L); // get H, S and L

    // hexadecimal value
    QString hex;
    if (palettes[n].R == -1) // not a plaette color, set it to 000000 because some filters use this to take out this color
        hex = "000000";
    else
        hex = QString(" %1").arg(((palettes[n].R & 0xff) << 16)
                                   + ((palettes[n].G & 0xff) << 8)
                                   +  (palettes[n].B & 0xff)
                                     , 6, 16, QChar('0')).trimmed(); // compute hexa RGB value
    hex = "#" + hex; // add a hash character
    palettes[n].hexa = hex.toUpper().toUtf8().constData(); // save hex value

    // distances to black, white and gray points, computed with CIEDE2000 distance algorithm
    palettes[n].distanceBlack = DistanceFromBlackRGB(palettes[n].R / 255.0, palettes[n].G / 255.0, palettes[n].B / 255.0);
    palettes[n].distanceWhite = DistanceFromWhiteRGB(palettes[n].R / 255.0, palettes[n].G / 255.0, palettes[n].B / 255.0);
    palettes[n].distanceGray = DistanceFromGrayRGB(palettes[n].R / 255.0, palettes[n].G / 255.0, palettes[n].B / 255.0);
}

void MainWindow::Compute() // analyze image dominant colors
{
    if (!loaded) { // nothing loaded yet = get out
        return;
    }

    QApplication::setOverrideCursor(Qt::WaitCursor); // wait cursor
    timer.start(); // reinit timer
    ShowTimer(true); // show it
    qApp->processEvents();

    cv::Mat imageCopy; // work on a copy of the image, because gray colors can be filtered
    image.copyTo(imageCopy);

    long double H, S, L;
    if (ui->checkBox_filter_grays->isChecked()) { // filter whites, blacks and greys if option is set
        cv::Vec3b RGB;
        for (int x = 0; x < imageCopy.cols; x++) // parse temp image
            for  (int y = 0; y < imageCopy.rows; y++) {
                RGB = imageCopy.at<cv::Vec3b>(y, x); // current pixel color
                HSLfromRGB(double(RGB[2] / 255.0), double(RGB[1] / 255.0), double(RGB[0] / 255.0), H, S, L); // get HSL values
                long double dBlack = DistanceFromBlackRGB((long double)(RGB[2]) / 255.0, (long double)(RGB[1]) / 255.0, (long double)(RGB[0]) / 255.0); // compute distances from black, white and gray points
                long double dWhite = DistanceFromWhiteRGB((long double)(RGB[2]) / 255.0, (long double)(RGB[1]) / 255.0, (long double)(RGB[0]) / 255.0);
                long double dGray = DistanceFromGrayRGB((long double)(RGB[2]) / 255.0, (long double)(RGB[1]) / 255.0, (long double)(RGB[0]) / 255.0);

                if ((dGray < graysLimit) or (dBlack < blacksLimit) or (dWhite < whitesLimit)) // white or black or grey pixel ?
                    imageCopy.at<cv::Vec3b>(y, x) = cv::Vec3b(0, 0, 0); // replace it with black in temp image
            }
    }

    nb_palettes= ui->spinBox_nb_palettes->value(); // how many dominant colors
    int nb_palettes_asked = nb_palettes; // save asked number of colors for later
    ui->spinBox_nb_palettes->setStyleSheet("QSpinBox{color:black;background-color: white;}"); // show number of colors in black (in case it was red before)

    if (ui->checkBox_filter_grays->isChecked()) { // if grays and blacks and whites are filtered
        cv::Mat1b black_mask;
        cv::inRange(imageCopy, cv::Vec3b(0, 0, 0), cv::Vec3b(0, 0, 0), black_mask); // extract black pixels from image (= whites and blacks and grays)
        if ((cv::sum(black_mask) != cv::Scalar(0,0,0))) // image contains black pixels ?
                nb_palettes++; // add one color to asked number of colors in palette, to remove it later and only get colors
    }

    // set all palette values to dummy values
    for (int n = 0; n < nb_palettes; n++) {
        palettes[n].R = -1;
        palettes[n].G = -1;
        palettes[n].B = -1;
        palettes[n].count = -1;
        palettes[n].percentage = -1;
        palettes[n].distanceBlack = 0;
        palettes[n].distanceWhite = 100;
        palettes[n].distanceGray = 100;
    }

    int totalMeanShift = 0; // number of colors obtained with Mean-shift algorithm
    if (ui->radioButton_mean_shift->isChecked()) { // mean-shift algorithm checked
        cv::Mat temp = ImgRGBtoLab(imageCopy); // convert image to CIELab

        MeanShift MSProc(ui->horizontalSlider_mean_shift_spatial->value(), ui->horizontalSlider_mean_shift_color->value()); // create instance of Mean-shift
        MSProc.MeanShiftFilteringCIELab(temp); // Mean-shift filtering
        MSProc.MeanShiftSegmentationCIELab(temp); // Mean-shift segmentation
        quantized = ImgLabToRGB(temp); // convert image back to RGB

        // palette from quantized image
        struct struct_colors { // color index and count
            cv::Vec3b RGB;
            int count;
        };
        int nb_count = CountRGBUniqueValues(quantized); // number of colors in quantized image : we don't know how many
        struct_colors color[nb_count]; // temp palette

        int nbColor = 0;
        for (int x = 0; x < quantized.cols; x++)
            for (int y = 0; y < quantized.rows; y++) { // parse quantized image
                cv::Vec3b col = quantized.at<cv::Vec3b>(y, x);
                bool found = false; // indicates if color was known
                for (int i = 0; i < nbColor; i++) // parse colors index
                    if (col == color[i].RGB) { // if color already registered
                        found = true; // we found it !
                        color[i].count++; // add 1 pixel to the count of this color
                        break; // stop
                    }
                if (!found) { // if color was not found
                    color[nbColor].RGB = col; // add it to the index
                    color[nbColor].count = 1; // 1 pixel found for the moment
                    nbColor++; // increase number of colors found
                }
            }
        std::sort(color, color + nbColor,
                  [](const struct_colors& a, const struct_colors& b) {return a.count > b.count;}); // sort colors by count

        int total = quantized.rows * quantized.cols; // number of pixels in image
        while ((nbColor > 1) and (double(color[nbColor - 1].count) / total < 0.001)) // is this color an insignificant value ?
            nbColor--; // one less color to consider

        if (nbColor > nb_palettes) // number of colors must not be superior to asked number of colors
            nbColor = nb_palettes;
        if (nbColor > nb_count) // number of asked colors could be inferior to the real number of colors in quantized image
            nbColor = nb_count;

        for (int n = 0; n < nbColor; n++) { // for all colors in Mean-shift palette
            palettes[n].R = color[n].RGB[2]; // copy RGB values to global palette
            palettes[n].G = color[n].RGB[1];
            palettes[n].B = color[n].RGB[0];
            totalMeanShift += color[n].count; // compute total number of pixels for this color
        }
    }
    else if (ui->radioButton_eigenvectors->isChecked()) { // eigen method
        cv::Mat conv = ImgRGBtoLab(imageCopy); // convert image to CIELab
        cv::Mat result;
        DominantColorsEigenCIELab(conv, nb_palettes, result); // get dominant palette, palette image and quantized image

        quantized = ImgLabToRGB(result); // convert result back to RGB

        // palette from quantized image
        cv::Vec3b color[nb_palettes]; // temp palette
        int nbColor = 0; // current color
        for (int x = 0; x < quantized.cols; x++) // parse entire image
            for (int y = 0; y < quantized.rows; y++) {
                cv::Vec3b col = quantized.at<cv::Vec3b>(y, x); // current pixel color
                bool found = false;
                for (int i = 0; i < nbColor; i++) // look into temp palette
                    if (col == color[i]) { // if color already exists
                        found = true; // found, don't add it
                        break;
                    }
                if (!found) { // color not already in temp palette
                    color[nbColor] = col; // save new color
                    palettes[nbColor].R = col[2]; // copy RGB values to global palette
                    palettes[nbColor].G = col[1];
                    palettes[nbColor].B = col[0];
                    nbColor++; // one more color
                }
            }
    }
    else if (ui->radioButton_k_means->isChecked()) { // K-means algorithm
        cv::Mat1f colors; // store palette from K-means
        quantized = DominantColorsKMeansCIELAB(imageCopy, nb_palettes, colors); // get quantized image and palette

        // palette from quantized image
        cv::Vec3b color[nb_palettes]; // temp palette
        int nbColor = 0; // current color
        for (int x = 0; x < quantized.cols; x++) // parse entire image
            for (int y = 0; y < quantized.rows; y++) {
                cv::Vec3b col = quantized.at<cv::Vec3b>(y, x); // current pixel color
                bool found = false;
                for (int i = 0; i < nbColor; i++) // look into temp palette
                    if (col == color[i]) { // if color already exists
                        found = true; // found, don't add it
                        break;
                    }
                if (!found) { // color not already in temp palette
                    color[nbColor] = col; // save new color
                    palettes[nbColor].R = col[2]; // copy RGB values to global palette
                    palettes[nbColor].G = col[1];
                    palettes[nbColor].B = col[0];
                    nbColor++; // one more color
                }
            }
    }

    // compute HSL values from RGB + hexa + distances
    for (int n = 0; n < nb_palettes; n++) // for each color in palette
        ComputePaletteValues(n); // compute values other than RGB

    // clean palette : number of asked colors may be superior to number of colors found
    int n = CountRGBUniqueValues(quantized); // how many colors in quantized image, really ?
    if (n < nb_palettes) { // if asked number of colors exceeds total number of colors in image
        std::sort(palettes, palettes + nb_palettes,
                  [](const struct_palette& a, const struct_palette& b) {return a.hexa > b.hexa;}); // sort palette by hexa value, decreasing values
        if (((palettes[0].R == palettes[1].R)
                and (palettes[0].G == palettes[1].G)
                and (palettes[0].B == palettes[1].B)) or (palettes[0].R == -1)) // if first color in palette is equal to second or it's a dummy color -> we have to reverse sort
            std::sort(palettes, palettes + nb_palettes,
                      [](const struct_palette& a, const struct_palette& b) {return a.hexa > b.hexa;}); // sort the palette, this time by increasing hexa values
        nb_palettes = n; // new number of colors in palette
        ui->spinBox_nb_palettes->setValue(nb_palettes); // show new number of colors
    }

    int total; // total number of pixels to consider for percentages
    if (ui->radioButton_mean_shift->isChecked()) // particular case of Mean-shift
        total = totalMeanShift; // total is the Mean-shift total computed before
    else // not Mean-shift
        total= quantized.rows * quantized.cols; // total is the size of quantized image in pixels

    // delete blacks in palette if "filter grays" enabled because there really can be one blackish color in the quantized image that could have been mixed with others
    if (ui->checkBox_filter_grays->isChecked()) { // delete last "black" values in palette
        bool black_found = false;
        std::sort(palettes, palettes + nb_palettes,
              [](const struct_palette& a, const struct_palette& b) {return a.distanceBlack > b.distanceBlack;}); // sort palette by distance from black
        while ((nb_palettes > 1) and (palettes[nb_palettes - 1].distanceBlack < blacksLimit)) { // at the end of palette, find black colors
            cv::Mat1b black_mask;
            cv::inRange(quantized, cv::Vec3b(palettes[nb_palettes - 1].B, palettes[nb_palettes - 1].G, palettes[nb_palettes - 1].R),
                               cv::Vec3b(palettes[nb_palettes - 1].B, palettes[nb_palettes - 1].G, palettes[nb_palettes - 1].R),
                               black_mask); // extract this black color from image in a mask
            int c = countNonZero(black_mask); // how many pixels are black ?
            total = total - c; // update total pixel count
            nb_palettes--; // exclude this black color from palette
            if (c > 0) // really found black color ?
                black_found = true; // black color found !
        }
        if (black_found) { // if black color found
            ui->spinBox_nb_palettes->setValue(nb_palettes); // show new number of colors without black values
        }
    }

    // compute percentages
    for (int n = 0; n < nb_palettes; n++) { // for each color in palette
        cv::Mat1b mask; // current color mask
        cv::inRange(quantized, cv::Vec3b(palettes[n].B, palettes[n].G, palettes[n].R),
                           cv::Vec3b(palettes[n].B, palettes[n].G, palettes[n].R),
                           mask); // create mask for current color
        palettes[n].count = cv::countNonZero(mask); // count pixels in this mask
        palettes[n].percentage = (long double)(palettes[n].count) / (long double)(total); // compute color percentage in image
    }

    // regroup near colors
    if (ui->checkBox_regroup->isChecked()) { // is "regroup colors" enabled ?
        bool regroup = false; // if two colors are regrouped this will be true
        for (int n = 0; n < nb_palettes; n++) // parse palette
            for (int i = 0; i < nb_palettes; i++) { // parse the same palette to compare values
                if ((n !=i) and (palettes[n].R + palettes[n].G + palettes[n].B != 0) and (palettes[i].R + palettes[i].G + palettes[i].B != 0)
                        and (palettes[n].R > 0) and (palettes[i].R > 0)) { // exlude same color index and black values and dummy colors
                    long double angle = Angle::DifferenceDeg(Angle::NormalizedToDeg(palettes[n].H), Angle::NormalizedToDeg(palettes[i].H)); // compute angle between the two colors n and i

                    if ((angle < ui->horizontalSlider_regroup_angle->value())
                            and (DistanceRGB((long double)palettes[n].R / 255.0, (long double)palettes[n].G / 255.0, (long double)palettes[n].B / 255.0,
                                    (long double)palettes[i].R / 255.0, (long double)palettes[i].G / 255.0, (long double)palettes[i].B / 255.0) < ui->horizontalSlider_regroup_distance->value())
                            ) { // check if the two colors meet regroup filter values
                        long double R, G, B;
                        RGBMean((long double)palettes[n].R / 255.0, (long double)palettes[n].G / 255.0, (long double)palettes[n].B / 255.0,
                                (long double)palettes[i].R / 255.0, (long double)palettes[i].G / 255.0, (long double)palettes[i].B / 255.0,
                                R, G, B); // the new color is the RGB mean of the two colors

                        // change quantized image
                        cv::Mat mask1;
                        cv::inRange(quantized, cv::Vec3b(palettes[n].B, palettes[n].G, palettes[n].R),
                                           cv::Vec3b(palettes[n].B, palettes[n].G, palettes[n].R),
                                           mask1); // extract first color n from image
                        quantized.setTo(cv::Vec3b(round(B * 255.0), round(G * 255.0), round(R * 255.0)), mask1); // replace pixels with new coolor
                        cv::Mat mask2; // do the same for the second i color
                        cv::inRange(quantized, cv::Vec3b(palettes[i].B, palettes[i].G, palettes[i].R),
                                           cv::Vec3b(palettes[i].B, palettes[i].G, palettes[i].R),
                                           mask2); // extract second color i from image
                        quantized.setTo(cv::Vec3b(round(B * 255.0), round(G * 255.0), round(R * 255.0)), mask2); // replace pixels with new coolor

                        // new palette values
                        palettes[n].R = round(R * 255.0); // replace colors in palette n with new color values
                        palettes[n].G = round(G * 255.0);
                        palettes[n].B = round(B * 255.0);
                        palettes[n].count += palettes[i].count; // merge the two colors count
                        palettes[n].percentage += palettes[i].percentage; // and merge the percentage too
                        ComputePaletteValues(n); // compute new palette values other than RGB

                        // indicate palette has changed
                        palettes[i].R = -1; // dummy value (important, it excludes this color now from the algorithm)
                        regroup = true; // at least one color regroup was found
                    }
                }
            }
        if (regroup) { // at least one color regroup was found so palette has changed
            std::sort(palettes, palettes + nb_palettes,
                      [](const struct_palette& a, const struct_palette& b) {return a.R > b.R;}); // sort palette by hexa value, decreasing values
            while (palettes[nb_palettes - 1].R == -1) // look for excluded colors
                nb_palettes--; // update palette count
        }
    }

    // delete non significant values in palette by percentage
    if (ui->checkBox_filter_percent->isChecked()) { // filter by x% enabled ?
        bool cleaning_found = false; // indicator
        std::sort(palettes, palettes + nb_palettes,
              [](const struct_palette& a, const struct_palette& b) {return a.percentage > b.percentage;}); // sort palette by decreasing percentage
        while ((nb_palettes > 1) and (palettes[nb_palettes - 1].percentage * 100 < ui->horizontalSlider_nb_percentage->value())) { // at the end of palette, find colors < x% of image
            cv::Mat1b cleaning_mask;
            cv::inRange(quantized, cv::Vec3b(palettes[nb_palettes - 1].B, palettes[nb_palettes - 1].G, palettes[nb_palettes - 1].R),
                               cv::Vec3b(palettes[nb_palettes - 1].B, palettes[nb_palettes - 1].G, palettes[nb_palettes - 1].R),
                               cleaning_mask); // extract this color from image
            int c = cv::countNonZero(cleaning_mask); // count occurences of this color
            total = total - c; // update total pixel count
            nb_palettes--; // exclude this color from palette
            if (c > 0) // really found this color ?
                cleaning_found = true; // palettes count has changed
        }
        if (cleaning_found) { // if cleaning found
            ui->spinBox_nb_palettes->setValue(nb_palettes); // show new number of colors without cleaned values
            // re-compute percentages
            for (int n = 0;n < nb_palettes; n++) // for each color in palette
                palettes[n].percentage = (long double)(palettes[n].count) / (long double)(total); // update percentage with new total
        }
    }

    // find color name by CIEDE2000 distance
    for (int n = 0; n < nb_palettes; n++) { // for each color in palette
        bool found = false;
        long double distance = 1000000; // distance from nearest color
        int index = 0; // to keep nearest color index in color names table

        for (int c = 0; c < nb_color_names; c++) { // search in color names table
            if ((palettes[n].R == color_names[c].R) and (palettes[n].G == color_names[c].G) and (palettes[n].B == color_names[c].B)) { // same RGB values found
                palettes[n].name = color_names[c].name; // assign color name to color in palette
                found = true; // color found in color names database
                break; // get out of loop
            }
            else { // exact color not found
                long double d = DistanceRGB((long double)(palettes[n].R) / 255.0, (long double)(palettes[n].G) / 255.0, (long double)(palettes[n].B) / 255.0,
                                            (long double)(color_names[c].R) / 255.0, (long double)(color_names[c].G) / 255.0, (long double)(color_names[c].B) / 255.0); // CIEDE2000 distance
                if (d < distance) { // if distance is smaller than before
                    distance = d; // new distance
                    index = c; // keep index
                }
            }
        }
        if (!found) // picked color not found in palette so display nearest color
            palettes[n].name = color_names[index].name; // assign color name
    }

    // sort palettes by count, biggest first
    std::sort(palettes, palettes + nb_palettes,
          [](const struct_palette& a, const struct_palette& b) {return a.count > b.count;}); // sort palette by percentage

    // create palette image
    palette = cv::Mat::zeros(cv::Size(palette_width, palette_height), CV_8UC3); // create blank image
    double offset = 0; // current x position in palette
    for (int n = 0;n < nb_palettes; n++) // for each color in palette
    {
        cv::rectangle(palette, cv::Rect(round(offset), 0,
                                    round(palettes[n].percentage * double(palette_width)), palette_height),
                                    cv::Vec3b(palettes[n].B, palettes[n].G, palettes[n].R), -1); // rectangle of current color
        offset += round(palettes[n].percentage * double(palette_width)); // next x position in palette
    }

    ShowWheel(); // display color wheel
    ShowResults(); // show result images

    ShowTimer(false); // show elapsed time
    QApplication::restoreOverrideCursor(); // Restore cursor

    computed = true; // success !

    // reset UI elements
    ui->pushButton_color_analogous->setChecked(false);
    ui->pushButton_color_complementary->setChecked(false);
    ui->pushButton_color_split_complementary->setChecked(false);
    ui->pushButton_color_square->setChecked(false);
    ui->pushButton_color_tetradic->setChecked(false);
    ui->pushButton_color_triadic->setChecked(false);
    ui->pushButton_color_monochromatic->setChecked(false);
    ui->label_color_analogous->setVisible(false);
    ui->label_color_complementary->setVisible(false);
    ui->label_color_split_complementary->setVisible(false);
    ui->label_color_square->setVisible(false);
    ui->label_color_tetradic->setVisible(false);
    ui->label_color_triadic->setVisible(false);
    ui->label_color_cold_warm->setText("");
    ui->label_color_cold_warm->setStyleSheet("QLabel{color:blue;background-color:rgb(220,220,220);border: 2px inset #8f8f91;}");
    ui->label_color_grays->setText("");
    ui->label_color_grays->setStyleSheet("QLabel{color:blue;background-color:rgb(220,220,220);border: 2px inset #8f8f91;}");
    ui->spinBox_nb_palettes->setStyleSheet("QSpinBox{color:black;background-color: white;}"); // show number of colors in black (in case it was red before)
    ui->label_color_bar->setPixmap(QPixmap()); // reset picked color
    ui->label_color_bar->setText("Pick\nColor");
    ui->label_color_r->setText("R"); // show RGB values
    ui->label_color_g->setText("G");
    ui->label_color_b->setText("B");
    ui->label_color_hex->setText("Hex");
    ui->label_color_percentage->setText("");
    ui->label_color_name->setText("");
    if (nb_palettes < nb_palettes_asked) { // more colors asked than were really found ?
        ui->spinBox_nb_palettes->setStyleSheet("QSpinBox{color:red;background-color: white;}"); // show new number of colors in red
        ui->spinBox_nb_palettes->setValue(nb_palettes); // show new number of colors
    }
    ui->frame_analyze->setVisible(true);
    ui->frame_analysis->setVisible(false);
    ui->frame_rgb->setVisible(true);
}

void MainWindow::ShowResults() // display result images in GUI
{
    if (!thumbnail.empty())
        ui->label_thumbnail->setPixmap(Mat2QPixmap(thumbnail)); // thumbnail
        else ui->label_thumbnail->setPixmap(QPixmap());
    if (!quantized.empty()) // quantized image
        ui->label_quantized->setPixmap(Mat2QPixmapResized(quantized, ui->label_quantized->width(), ui->label_quantized->height(), false)); // quantized image
        else ui->label_quantized->setPixmap(QPixmap());
    if (!palette.empty()) // palette image
        ui->label_palette->setPixmap(Mat2QPixmapResized(palette, ui->label_palette->width(), ui->label_palette->height(), false)); // palette image
        else ui->label_palette->setPixmap(QPixmap());
}

void MainWindow::DrawOnWheelBorder(const int &R, const int &G, const int &B, const int &radius, const bool center) // draw one color disk on the wheel border
{
    long double H, S, L; // HSL values
    HSLfromRGB(double(R) / 255.0, double(G) / 255.0, double(B) / 255.0, H, S, L); // convert RGB to HSL values (L and S are from CIELab)

    long double angle = -Angle::NormalizedToRad(H + 0.25); // angle convert normalized value to degrees + shift to have red on top
    long double xOffset = cosf(angle) * wheel_radius; // position from center of circle
    long double yOffset = sinf(angle) * wheel_radius;

    cv::circle(wheel, cv::Point(wheel_center.x + xOffset, wheel_center.y + yOffset), radius, cv::Vec3b(B, G, R), -1, cv::LINE_AA); // draw color disk
    cv::circle(wheel, cv::Point(wheel_center.x + xOffset, wheel_center.y + yOffset), radius, cv::Vec3b(255,255,255), 2, cv::LINE_AA); // draw white border
    if (center) // draw black center ?
        cv::circle(wheel, cv::Point(wheel_center.x + xOffset, wheel_center.y + yOffset), radius, cv::Vec3b(0,0,0), -1, cv::LINE_AA); // draw black center disk
}

void MainWindow::DrawOnWheel(const int &R, const int &G, const int &B, const int &radius) // draw one color disk on the wheel
{
    long double H, S, L; // HSL values
    HSLfromRGB(double(R) / 255.0, double(G) / 255.0, double(B) / 255.0, H, S, L); // convert RGB to HSL values (L and S are from CIELab)

    long double colorRadius = (long double)(wheel_radius_center) * L; // color distance from center with L value
    long double angle = -Angle::NormalizedToRad(H + 0.25); // angle convert normalized value to degrees + shift to have red on top
    long double xOffset = cosf(angle) * colorRadius; // position from center of circle
    long double yOffset = sinf(angle) * colorRadius;

    cv::circle(wheel, cv::Point(wheel_center.x + xOffset, wheel_center.y + yOffset), round((long double)(radius) * (long double)(ui->verticalSlider_circle_size->value()) / 4), cv::Vec3b(B, G, R), -1, cv::LINE_AA); // draw color disk
}

void MainWindow::ShowWheel() // display color wheel
{
    wheel = cv::Mat::zeros(ui->label_wheel->height(), ui->label_wheel->width(), CV_8UC3); // empty wheel image

    wheel = cv::Vec3b(192,192,192); // background is light gray

    // size and center
    wheel_center = cv::Point(wheel.cols / 2, wheel.rows / 2); // center of wheel
    wheel_radius = wheel.cols / 2 - 50; // radius of outer circle (Primary Secondary and Tertiary colors)
    wheel_radius_center = wheel_radius - 70; // inner circles for image palette

    // circles
    cv::circle(wheel, wheel_center, wheel_radius, cv::Vec3b(255, 255, 255), 2,  cv::LINE_AA); // outer circle
    cv::circle(wheel, wheel_center, wheel_radius_center, cv::Vec3b(200, 200, 200), 2,  cv::LINE_AA); // inner circle
    // circle center (a cross)
    cv::line(wheel, cv::Point(wheel_center.x, wheel_center.y - 10), cv::Point(wheel_center.x, wheel_center.y + 10), cv::Vec3b(255,255,255), 1);
    cv::line(wheel, cv::Point(wheel_center.x - 10, wheel_center.y), cv::Point(wheel_center.x + 10, wheel_center.y), cv::Vec3b(255,255,255), 1);

    // draw Primary, Secondary and Tertiary color disks on wheel
    // Primary = biggest circles
    DrawOnWheelBorder(255,0,0,40,false); // red
    DrawOnWheelBorder(0,255,0,40,false); // green
    DrawOnWheelBorder(0,0,255,40,false); // blue
    // Secondary
    DrawOnWheelBorder(255,255,0,30,false); // yellow
    DrawOnWheelBorder(255,0,255,30,false); // magenta
    DrawOnWheelBorder(0,255,255,30,false); // blue
    // Tertiary
    DrawOnWheelBorder(255,127,0,20,false); // orange
    DrawOnWheelBorder(255,0,127,20,false); // rose
    DrawOnWheelBorder(127,0,255,20,false); // violet
    DrawOnWheelBorder(0,127,255,20,false); // azure
    DrawOnWheelBorder(0,255,127,20,false); // aquamarine
    DrawOnWheelBorder(127,255,0,20,false); // chartreuse

    // Draw palette disks : size = percentage of use in quantized image
    for (int n = 0; n < nb_palettes;n++) // for each color in palette
        DrawOnWheel(palettes[n].R, palettes[n].G,palettes[n].B,
                    int(palettes[n].percentage * 100.0)); // draw color disk

    ui->label_wheel->setPixmap(Mat2QPixmap(wheel)); // update view
}

void MainWindow::ShowTimer(const bool start) // time elapsed
{
    if (start) // something is beeing computed
        ui->timer->display(".BUSY..."); // wait message
    else { // show elapsed time
        int milliseconds = int(timer.elapsed()%1000); // milliseconds
        int seconds = int(timer.elapsed()/1000%60); // seconds
        ui->timer->display(QString("%1").arg(seconds, 3, 10, QChar('0'))
                          + "."
                          + QString("%1").arg(milliseconds, 3, 10, QChar('0'))); // show value in LCD timer
    }
}

void MainWindow::SetCircleSize(int size) // called when circle size slider is moved
{
    ShowWheel();
}
