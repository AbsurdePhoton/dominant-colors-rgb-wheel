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
    else {
        QMessageBox::critical(this, "Colors text file not found!", "You forgot to put 'color-names.txt' in the same folder as the executable! This tool will crash if you click on a color...");
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

    // if image not empty save it with base name + type .PNG
    if (!quantized.empty())
        cv::imwrite(basedir + basefile + "-quantized.png", quantized);
    if (!palette.empty())
        cv::imwrite(basedir + basefile + "-palette.png", palette);
    if (!wheel.empty())
        cv::imwrite(basedir + basefile + "-wheel.png", wheel);
    /*if (!classification.empty())
        cv::imwrite(basedir + basefile + "-classification.png", classification);*/

    // Palette
    std::string line; // line to write in text file
    ofstream save; // file to save
    save.open(basedir + basefile + "-palette.csv"); // save palette file

    if (save) { // if successfully open
        //std::string s; // line to save
        for (int n = 0; n < nb_palettes; n++) { // read palette
            save << palettes[n].R << ";";
            save << palettes[n].G << ";";
            save << palettes[n].B << ";";
            QString hex = QString("%1").arg(((palettes[n].R & 0xff) << 16) + ((palettes[n].G & 0xff) << 8) + (palettes[n].B & 0xff), 6, 16, QChar('0')); // hexa from RGB value
            save << "#" << hex.toUpper().toUtf8().constData() << ";";
            save << palettes[n].percentage << "\n";
        }

        save.close(); // close text file
    }

    QMessageBox::information(this, "Results saved", "Your results were saved with base file name:\n" + QString::fromStdString(basedir + basefile));
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

void MainWindow::DrawOnWheelBorder(const int &R, const int &G, const int &B, const int &radius) // draw one color circle on the wheel
{
    float H, S, L, C; // HSL values
    RGBtoHSL(float(R) / 255.0f, float(G) / 255.0f, float(B) / 255.0f, H, S, L, C); // convert RGB to HSL values

    float angle = -(H + 0.25) * 2.0f * Pi; // angle convert degrees to Pi value + shift to have red on top
    float xOffset = cosf(angle) * wheel_radius; // position from center of circle
    float yOffset = sinf(angle) * wheel_radius;

    cv::circle(wheel, cv::Point(wheel_center.x + xOffset, wheel_center.y + yOffset), radius, Vec3b(B, G, R), -1, cv::LINE_AA); // draw color disk
    cv::circle(wheel, cv::Point(wheel_center.x + xOffset, wheel_center.y + yOffset), radius, Vec3b(255,255,255), 2, cv::LINE_AA); // white border
}

void MainWindow::DrawOnWheel(const int &R, const int &G, const int &B, const int &radius) // draw one color circle on the wheel
{
    float H, S, L, C; // HSL values
    RGBtoHSL(float(R) / 255.0f, float(G) / 255.0f, float(B) / 255.0f, H, S, L, C); // convert RGB to HSL values

    float colorRadius = float(wheel_radius_center) * L; // color distance from center
    float angle = -(H + 0.25) * 2.0f * Pi; // angle convert degrees to Pi value + shift to have red on top
    float xOffset = cosf(angle) * colorRadius; // position from center of circle
    float yOffset = sinf(angle) * colorRadius;

    cv::circle(wheel, cv::Point(wheel_center.x + xOffset, wheel_center.y + yOffset), round(float(radius) * float(ui->verticalSlider_circle_size->value()) / 4), Vec3b(B, G, R), -1, cv::LINE_AA); // draw color disk
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
    DrawOnWheelBorder(255,0,0,40); // red
    DrawOnWheelBorder(0,255,0,40); // green
    DrawOnWheelBorder(0,0,255,40); // blue
    // Secondary
    DrawOnWheelBorder(255,255,0,30); // yellow
    DrawOnWheelBorder(255,0,255,30); // magenta
    DrawOnWheelBorder(0,255,255,30); // blue
    // Tertiary
    DrawOnWheelBorder(255,127,0,20); // orange
    DrawOnWheelBorder(255,0,127,20); // rose
    DrawOnWheelBorder(127,0,255,20); // violet
    DrawOnWheelBorder(0,127,255,20); // azure
    DrawOnWheelBorder(0,255,127,20); // aquamarine
    DrawOnWheelBorder(127,255,0,20); // chartreuse

    // Draw palette disks - size = percentage of use in quantized image
    for (int n = 0; n < nb_palettes;n++) // for each color in palette
        DrawOnWheel(palettes[n].R, palettes[n].G,palettes[n].B,
                    int(palettes[n].percentage * 100.0f)); // draw color disk

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

void MainWindow::SetCircleSize(int size) // called when circle size slider is moved
{
    ShowWheel();
}
