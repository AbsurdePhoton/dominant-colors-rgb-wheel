/*#-------------------------------------------------
#
#    Dominant colors from image with openCV
#
#    by AbsurdePhoton - www.absurdephoton.fr
#
#                v0 - 2019/10/10
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

#include "mat-image-tools.h"
#include "dominant-colors.h"

using namespace cv;
using namespace cv::ximgproc;
using namespace std;

/////////////////// Window init //////////////////////

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    // window
    setWindowFlags((((windowFlags() | Qt::CustomizeWindowHint)
                            & ~Qt::WindowCloseButtonHint) | Qt::WindowMinMaxButtonsHint)); // don't show buttons in title bar
    this->setWindowState(Qt::WindowMaximized); // maximize window
    setFocusPolicy(Qt::StrongFocus); // catch keyboard and mouse in priority
    statusBar()->setVisible(false); // no status bar

    // initial variable values
    InitializeValues();
}

MainWindow::~MainWindow()
{
    delete ui;
}

///////////////////      GUI       //////////////////////

void MainWindow::InitializeValues() // Global variables init
{

    loaded = false; // main image NOT loaded
    computed = false; // dominant colors NOT computed

    basedirinifile = QDir::currentPath().toUtf8().constData(); // where to store the folder ini file
    basedirinifile += "/dir.ini";
    cv::FileStorage fs(basedirinifile, FileStorage::READ); // open dir ini file
    if (fs.isOpened()) {
        fs["BaseDir"] >> basedir; // load dir
    }
        else basedir = "/home/"; // default base path and file
    basefile = "example";

    // Timer
    ui->timer->setPalette(Qt::red);

    // Wheel
    wheel = Mat::zeros(ui->label_wheel->width(), ui->label_wheel->height(), CV_8UC3); // wheel image
    wheel_center = cv::Point(wheel.cols / 2, wheel.rows / 2); // center of wheel
    wheel_radius = wheel.cols / 2 - 50; // radius of outer circle (Primary Secondary and Tertiary colors)
    wheel_radius_center = wheel_radius - 70; // inner circles for image palette
    nb_palettes= -1; // no palette yet
    ShowWheel(); // draw empty wheel

    // Color names
    std::string line; // line to read in text file
    ifstream names; // file to read
    names.open("color-names.txt"); // read color names file

    if (names) { // if successfully read
        nb_color_names = -1; // index of color names array
        size_t pos; // index for find function
        std::string s; // used for item extraction
        while (getline(names, line)) { // read each line of text file: R G B name
            pos = 0; // find index at the beginning of the line
            nb_color_names++; // current index in color names array
            int pos2 = line.find(" ", pos); // find first space char
            s = line.substr(pos, pos2 - pos); // extract R value
            color_names[nb_color_names].R = std::stoi(s); // R in array
            pos = pos2 + 1; // next char
            pos2 = line.find(" ", pos); // find second space char
            s = line.substr(pos, pos2 - pos); // extract G value
            color_names[nb_color_names].G = std::stoi(s); // G in array
            pos = pos2 + 1; // next char
            pos2 = line.find(" ", pos); // find third space char
            s = line.substr(pos, pos2 - pos); // extract B value
            color_names[nb_color_names].B = std::stoi(s); // B in array
            s = line.substr(pos2 + 1, line.length() - pos2); // color name is at the end of the line
            color_names[nb_color_names].name = QString::fromStdString(s); // color name in array
        }

        names.close(); // close text file
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

void MainWindow::on_button_save_images_clicked() // save dominant colors results
{
    if (!computed) { // nothing loaded yet = get out
        QMessageBox::critical(this, "Nothing to do!", "You have to load then compute before saving the images");
        return;
    }

    // if image not empty save it with base name + type .PNG
    if (!quantized.empty())
        cv::imwrite(basedir + basefile + "-quantized.png", quantized);
    if (!palette.empty())
        cv::imwrite(basedir + basefile + "-palette.png", palette);
    if (!wheel.empty())
        cv::imwrite(basedir + basefile + "-wheel.png", wheel);
    /*if (!classification.empty())
        cv::imwrite(basedir + basefile + "-classification.png", classification);*/

    QMessageBox::information(this, "Images saved", "Your images were saved with the base file name:\n" + QString::fromStdString(basedir + basefile));
}

/////////////////// Mouse events //////////////////////

void MainWindow::mousePressEvent(QMouseEvent *eventPress) // event triggered by a mouse click
{
    mouseButton = eventPress->button(); // mouse button value

    bool color_found = false; // valid color found ?
    Vec3b color; // BGR values of picked color

    if (ui->label_wheel->underMouse()) { // mouse over wheel ?
        mouse_pos = ui->label_wheel->mapFromGlobal(QCursor::pos()); // mouse position

        if ((mouseButton == Qt::LeftButton) and (!wheel.empty())) { // mouse left button clicked
            color = wheel.at<Vec3b>(mouse_pos.y(), mouse_pos.x()); // get BGR "color" under mouse cursor
            color_found = true; // found !
        }
    }

    if (ui->label_palette->underMouse()) { // mouse over palette ?
        mouse_pos = ui->label_palette->mapFromGlobal(QCursor::pos()); // mouse position

        if ((mouseButton == Qt::LeftButton) and (!palette.empty())) { // mouse left button clicked
            int x = round(float(palette_width) * mouse_pos.x() / ui->label_palette->width()); // x position in palette
            color = palette.at<Vec3b>(0, x); // pick color in palette
            color_found = true; // found !
        }
    }

    if (ui->label_quantized->underMouse()) { // mouse over quantized image ?
        mouse_pos = ui->label_quantized->mapFromGlobal(QCursor::pos()); // mouse position

        if ((mouseButton == Qt::LeftButton) and (!quantized.empty())) { // mouse left button clicked
            const QPixmap* q = ui->label_quantized->pixmap(); // stored quantized image in GUI
            Mat img = QPixmap2Mat(*q); // convert it to openCV Mat
            color = img.at<Vec3b>(mouse_pos.y() - (ui->label_quantized->height() - img.rows) / 2,
                                  mouse_pos.x() - (ui->label_quantized->width() - img.cols) / 2); // pick color at x,y
            color_found = true; // found !
        }
    }

    if (color_found) { // color picked ?
        // RGB values
        int R = color[2];
        int G = color[1];
        int B = color[0];

        // display color, RGB values
        Mat bar = cv::Mat::zeros(cv::Size(1,1), CV_8UC3); // 1 pixel image
        bar = Vec3b(B,G,R); // set it to picked color
        ui->label_color_bar->setPixmap(Mat2QPixmapResized(bar, ui->label_color_bar->width(), ui->label_color_bar->height(), false)); // show picked color
        ui->label_color_r->setText(QString::number(R)); // show RGB values
        ui->label_color_g->setText(QString::number(G));
        ui->label_color_b->setText(QString::number(B));
        QString hex = QString("%1").arg(((R & 0xff) << 16) + ((G & 0xff) << 8) + (B & 0xff), 6, 16, QChar('0')); // hexa RGB value
        ui->label_color_hex->setText("#" + hex.toUpper()); // show it

        // find color in palette
        bool found = false; // picked color found in palette ?
        for (int n = 0; n < nb_palettes; n++) { // search in palette
            if ((palettes[n].R == R) and (palettes[n].G == G) and (palettes[n].B == B)) { // identical RGB values found
                QString value = QString::number(palettes[n].percentage * 100, 'f', 2) + "%"; // picked color percentage in quantized image
                ui->label_color_percentage->setText(value); // display percentage
                found = true; // color found in palette
                break; // get out of loop
            }
        }
        if (!found) // picked color not found in palette
            ui->label_color_percentage->setText(""); // no percentage displayed

        // find color name by euclidian distance
        found = false;
        int distance = 1000000; // distance formula can never reach this high
        int index; // to keep nearest color index in color names table

        for (int n = 0; n < nb_color_names; n++) { // search in color names table
            int d = pow(R - color_names[n].R, 2) + pow(G - color_names[n].G, 2) + pow(B - color_names[n].B, 2); // euclidian distance
            if (d == 0) { // exact RGB values found
                ui->label_color_name->setText(color_names[n].name); // display color name
                found = true; // color found in palette
                break; // get out of loop
            }
            else {
                if (d < distance) { // if distance is smaller
                    distance = d; // new distance
                    index = n; // keep index
                }
            }
        }
        if (!found) { // picked color not found in palette so display nearest color
            ui->label_color_name->setText("nearest: "
                                          + color_names[index].name
                                          + " (" + QString::number(color_names[index].R) + ","
                                          + QString::number(color_names[index].G) + ","
                                          + QString::number(color_names[index].B) + ")");
        }
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

    if (ui->checkBox_gaussian_blur->isChecked()) cv::GaussianBlur(image, image, Size(3,3), 0, 0); // gaussian blur
    if (ui->checkBox_reduce_size->isChecked()) {
        if ((image.rows > 512) or (image.cols > 512)) image = ResizeImageAspectRatio(image, cv::Size(512,512)); // resize image
    }

    loaded = true; // loaded successfully !

    ui->label_filename->setText(filename); // display file name in ui

    thumbnail = ResizeImageAspectRatio(image, cv::Size(ui->label_thumbnail->width(),ui->label_thumbnail->height())); // create thumbnail
    quantized.release(); // no quantized image yet
    palette.release(); // no palette image yet
    //classification.release();

    ShowResults(); // show images in GUI
    nb_palettes = -1; // no palette to show
    ShowWheel(); // display wheel

    ui->timer->display("0"); // reset timer
}

/////////////////// RGB & HSL //////////////////////

float RGBMin(const float &fR, const float &fG, const float &fB) // max value in RGB triplet
{
    float fMin = fR;
    if (fG < fMin)
        fMin = fG;
    if (fB < fMin)
        fMin = fB;
    return fMin;
}

float RGBMax(const float &fR, const float &fG, const float &fB) // min value in RGB triplet
{
    float fMax = fR;
    if (fG > fMax)
        fMax = fG;
    if (fB > fMax)
        fMax = fB;
    return fMax;
}

void RGBToHSL(const int &R, const int &G, const int &B, float& H, float& S, float& L) // convert RGB value to HSL
{
    // R, G, B values divided by 255 for a range in [0..1]
    float r = R / 255.0;
    float g = G / 255.0;
    float b = B / 255.0;

    float h, s, v; // h, s, v = hue, saturation, value

    float cmax = RGBMax(r, g, b);    // maximum of r, g, b
    float cmin = RGBMin(r, g, b);    // minimum of r, g, b
    float diff = cmax-cmin;       // diff of cmax and cmin.

    if (cmax == cmin) // cmin = cmax
        h = 0;

    else if (cmax == r) // cmax = r
        h = int(60 * ((g - b) / diff) + 360) % 360;

    else if (cmax == g) // cmax = g
        h = int(60 * ((b - r) / diff) + 120) % 360;

    else if (cmax == b) // cmax = b
        h = int(60 * ((r - g) / diff) + 240) % 360;

    v = cmax * 100.0; // compute v

    // compute s
    if (cmax == 0) // 0 is a particular value
        s = cmax;
    else if ((r == g) and (g == b)) // grey => center of the wheel
    {
        s = r;
        v = 0;
    }
    else
        s = (r + g + b) / 3.0; // average of R G B

    // Final results are in range [0..1]
    H = (h + 90) / 360.0; // was in degrees
    S = s; // percentage
    L = v / 100.0; // percentage
}

/////////////////// Core functions //////////////////////

void MainWindow::Compute() // analyze image dominant colors
{
    if (!loaded) { // nothing loaded yet = get out
        return;
    }

    QApplication::setOverrideCursor(Qt::WaitCursor); // wait cursor
    timer.start();
    qApp->processEvents();


    nb_palettes= ui->spinBox_nb_palettes->value(); // how many dominant colors

    if (ui->radioButton_eigenvectors->isChecked()) { // eigen method
        std::vector<cv::Vec3b> palette_vec = DominantColorsEigen(image, nb_palettes, &quantized); // get dominant palette, palette image and quantized image

        for (int n = 0;n < nb_palettes; n++) // store palette in structured array
        {
            palettes[n].R = palette_vec[n][2];
            palettes[n].G = palette_vec[n][1];
            palettes[n].B = palette_vec[n][0];
        }
    }
    else if (ui->radioButton_k_means->isChecked()) { // K-means method
        cv::Mat1f colors; // to store palette from K-means
        quantized = DominantColorsKMeans(image, ui->spinBox_nb_palettes->value(), &colors); // get quantized image and palette

        for (int n = 0;n < nb_palettes; n++) // store palette in structured array
        {
            palettes[n].R = round(colors(n, 2));
            palettes[n].G = round(colors(n, 1));
            palettes[n].B = round(colors(n, 0));
        }

        //classification.release();
    }

    // count occurences of colors in quantized image
    int total = quantized.rows * quantized.cols; // size of quantized image in pixels
    for (int n = 0;n < nb_palettes; n++) // for each color in palette
    {
        Mat1b mask; // current color mask
        inRange(quantized, Vec3b(palettes[n].B, palettes[n].G, palettes[n].R), Vec3b(palettes[n].B, palettes[n].G, palettes[n].R), mask); // create mask for current color
        palettes[n].count = cv::countNonZero(mask); // count pixels in mask
        palettes[n].percentage = float(palettes[n].count) / float(total); // color use percentage
    }

    // sort palettes by count, biggest first
    std::sort(palettes,palettes+nb_palettes,struct_palette::compOnR); // use sort function in palette struct

    // create palette image
    palette = Mat::zeros(cv::Size(palette_width, palette_height), CV_8UC3); // create blank image
    float offset = 0; // current x position in palette
    for (int n = 0;n < nb_palettes; n++) // for each color in palette
    {
        cv::rectangle(palette, Rect(round(offset), 0,
                                    round(palettes[n].percentage * float(palette_width)), palette_height),
                                    Vec3b(palettes[n].B, palettes[n].G, palettes[n].R), -1); // rectangle of current color
        offset += round(palettes[n].percentage * float(palette_width)); // next x position in palette
    }

    ShowWheel(); // display color wheel
    ShowResults(); // show result images

    ShowTimer();
    QApplication::restoreOverrideCursor(); // Restore cursor

    computed = true; // success !
}

void MainWindow::ShowResults() // display result images in GUI
{
    if (!thumbnail.empty())
        ui->label_thumbnail->setPixmap(Mat2QPixmap(thumbnail)); // thumbnail
        else ui->label_thumbnail->setPixmap(QPixmap());
    if (!quantized.empty())
        ui->label_quantized->setPixmap(Mat2QPixmapResized(quantized, ui->label_quantized->width(), ui->label_quantized->height(), true)); // quantized image
        else ui->label_quantized->setPixmap(QPixmap());
    if (!palette.empty())
        ui->label_palette->setPixmap(Mat2QPixmapResized(palette, ui->label_palette->width(), ui->label_palette->height(), true)); // palette image
        else ui->label_palette->setPixmap(QPixmap());
    /*if (!classification.empty())
        ui->label_classification->setPixmap(Mat2QPixmapResized(classification, ui->label_classification->width(), ui->label_classification->height(), true)); // classification image
        else ui->label_classification->setPixmap(QPixmap());*/
}

void MainWindow::DrawOnWheel(const int &R, const int &G, const int &B, const int &radius, const int wheel_radius_local, const bool &border = true) // draw one color circle on the wheel
{
    float H, S, L; // HSL values
    RGBToHSL(R, G, B, H, S, L); // convert RGB to HSL values

    int r = radius; // radius of color circle
    if (radius == 0) r = 40; // no radius specified = minimum radius
    float colorRadius = float(wheel_radius_local) * L; // color distance from center
    float angle = (1 - H) * (2 * Pi); // angle convert degrees to Pi value
    float xOffset = cos(angle) * colorRadius; // position from center of circle
    float yOffset = sin(angle) * colorRadius;

    cv::circle(wheel, cv::Point(wheel_center.x + xOffset, wheel_center.y + yOffset), r, Vec3b(B, G, R), -1, cv::LINE_AA); // draw color disk
    if (border) // draw border ?
        cv::circle(wheel, cv::Point(wheel_center.x + xOffset, wheel_center.y + yOffset), r, Vec3b(255,255,255), 2, cv::LINE_AA); // white border
}

void MainWindow::ShowWheel() // display color wheel
{
    // background is light gray
    wheel = Vec3b(192,192,192);

    // circles
    cv::circle(wheel, wheel_center, wheel_radius, Vec3b(255, 255, 255), 2,  cv::LINE_AA); // outer circle
    cv::circle(wheel, wheel_center, wheel_radius_center, Vec3b(200, 200, 200), 2,  cv::LINE_AA); // inner circle
    // circle center (a cross)
    cv::line(wheel, cv::Point(wheel_center.x, wheel_center.y - 10), cv::Point(wheel_center.x, wheel_center.y + 10), Vec3b(255,255,255), 1);
    cv::line(wheel, cv::Point(wheel_center.x - 10, wheel_center.y), cv::Point(wheel_center.x + 10, wheel_center.y), Vec3b(255,255,255), 1);

    // draw Primary, Secondary and Tertiary colors on wheel
    // Primary = biggest circles
    DrawOnWheel(255,0,0,40, wheel_radius); // red
    DrawOnWheel(0,255,0,40, wheel_radius); // green
    DrawOnWheel(0,0,255,40, wheel_radius); // blue
    // Secondary
    DrawOnWheel(255,255,0,30, wheel_radius); // yellow
    DrawOnWheel(255,0,255,30, wheel_radius); // magenta
    DrawOnWheel(0,255,255,30, wheel_radius); // blue
    // Tertiary
    DrawOnWheel(255,127,0,20, wheel_radius); // orange
    DrawOnWheel(255,0,127,20, wheel_radius); // rose
    DrawOnWheel(127,0,255,20, wheel_radius); // violet
    DrawOnWheel(0,127,255,20, wheel_radius); // azure
    DrawOnWheel(0,255,127,20, wheel_radius); // aquamarine
    DrawOnWheel(127,255,0,20, wheel_radius); // chartreuse

    // Draw palette disks - size = percentage of use in quantized image
    for (int n = 0; n < nb_palettes;n++) // for each color in palette
        DrawOnWheel(palettes[n].R, palettes[n].G,palettes[n].B, int(palettes[n].percentage * 100) + 10, wheel_radius_center, false); // draw color disk

    ui->label_wheel->setPixmap(Mat2QPixmap(wheel)); // update view
}

void MainWindow::ShowTimer() // time elapsed
{
    int milliseconds = int(timer.elapsed()%1000);
    int seconds = int(timer.elapsed()/1000%60);
    ui->timer->display(QString("%1").arg(seconds, 3, 10, QChar('0'))
                      + "."
                      + QString("%1").arg(milliseconds, 3, 10, QChar('0')));
}
