/*#-------------------------------------------------
 *
 * OpenCV image tools library
 * Author: AbsurdePhoton
 *
 * v2.3 - 2020/03/01
 *
 * Convert mat images to QPixmap or QImage and vice-versa
 * Brightness, Contrast, Gamma, Equalize, Color Balance
 * Erosion / dilation of pixels
 * Copy part of image
 * Resize image keeping aspect ration
 * Contours using Canny algorithm with auto min and max threshold
 * Noise reduction quality
 * Gray gradients
 * Red-cyan anaglyph tints
 * Count number of RGB colors in image
 * Special image copy with zero as transparent value
 *
#-------------------------------------------------*/

#include <QPixmap>

#include "opencv2/opencv.hpp"

#include "mat-image-tools.h"
#include "angles.h"
#include "color-spaces.h"

using namespace std;
using namespace cv;

///////////////////////////////////////////////////////////
//// Conversions from QImage and QPixmap to Mat
///////////////////////////////////////////////////////////

Mat QImage2Mat(const QImage &source) // Convert QImage to Mat, every number of channels supported
{
    switch (source.format()) { // which format ?

        case QImage::Format_ARGB32: // 8-bit, 4 channel
        case QImage::Format_ARGB32_Premultiplied: {
            Mat temp(source.height(), source.width(), CV_8UC4,
                        const_cast<uchar*>(source.bits()),
                        static_cast<size_t>(source.bytesPerLine()));
            return temp;
        }

        case QImage::Format_RGB32: { // 8-bit, 3 channel
            Mat temp(source.height(), source.width(), CV_8UC4,
                        const_cast<uchar*>(source.bits()),
                        static_cast<size_t>(source.bytesPerLine()));
            Mat tempNoAlpha;
            cvtColor(temp, tempNoAlpha, COLOR_BGRA2BGR);   // drop the all-white alpha channel
        return tempNoAlpha;
        }

        case QImage::Format_RGB888: { // 8-bit, 3 channel
            QImage swapped = source.rgbSwapped();
            return Mat(swapped.height(), swapped.width(), CV_8UC3,
                       const_cast<uchar*>(swapped.bits()),
                       static_cast<size_t>(swapped.bytesPerLine())).clone();
        }

        case QImage::Format_Indexed8: { // 8-bit, 1 channel
            Mat temp(source.height(), source.width(), CV_8UC1,
                        const_cast<uchar*>(source.bits()),
                        static_cast<size_t>(source.bytesPerLine()));
            return temp;
        }

        default:
            break;
    }

    return Mat(); // return empty Mat if type not found
}

Mat QPixmap2Mat(const QPixmap &source) // Convert Pixmap to Mat
{
    return QImage2Mat(source.toImage()); // simple !
}

///////////////////////////////////////////////////////////
//// Mat conversions to QImage and QPixmap
///////////////////////////////////////////////////////////

QImage Mat2QImage(const Mat &source) // convert BGR Mat to RGB QImage
{
     Mat temp;
     cvtColor(source, temp, COLOR_BGR2RGB); // convert Mat BGR to QImage RGB
     QImage dest((const uchar *) temp.data, temp.cols, temp.rows, temp.step, QImage::Format_RGB888); // conversion
     dest.bits(); // enforce deep copy of QImage::QImage (const uchar * data, int width, int height, Format format)

     return dest;
}

QPixmap Mat2QPixmap(const Mat &source) // convert Mat to QPixmap
{
    QImage i = Mat2QImage(source); // first step: convert to QImage
    QPixmap p = QPixmap::fromImage(i, Qt::AutoColor); // then convert QImage to QPixmap

    return p;
}

QPixmap Mat2QPixmapResized(const Mat &source, const int &width, const int &height, const bool &smooth) // convert Mat to resized QPixmap
{
    Qt::TransformationMode quality; // quality
    if (smooth) quality = Qt::SmoothTransformation;
        else quality = Qt::FastTransformation;

    QImage i = Mat2QImage(source); // first step: convert to QImage
    QPixmap p = QPixmap::fromImage(i, Qt::AutoColor); // then convert QImage to QPixmap

    return p.scaled(width, height, Qt::KeepAspectRatio, quality); // resize with high quality
}

QImage cvMatToQImage(const Mat &source) // another implementation BGR to RGB, every number of channels supported
{
    switch (source.type()) { // which type ?

        case CV_8UC4: { // 8-bit, 4 channel
            QImage image(   source.data,
                            source.cols, source.rows,
                            static_cast<int>(source.step),
                            QImage::Format_ARGB32);
            return image;
        }


        case CV_8UC3: { // 8-bit, 3 channel
            QImage image(   source.data,
                            source.cols, source.rows,
                            static_cast<int>(source.step),
                            QImage::Format_RGB888);
            return image.rgbSwapped();
        }

        case CV_8UC1: { // 8-bit, 1 channel
            QImage image(source.data, source.cols, source.rows,
                         static_cast<int>(source.step),
                         QImage::Format_Grayscale8);
        }
    }

    return QImage(); // return empty Mat if type not found
}

///////////////////////////////////////////////////////////
//// Brightness, Contrast, Gamma, Equalize, Balance
///////////////////////////////////////////////////////////

Mat BrightnessContrast(const Mat &source, const double &alpha, const int &beta) // set brightness and contrast
{
    Mat image = Mat::zeros(source.size(), source.type());

    // Do the operation new_image(i,j) = alpha*image(i,j) + beta
    for (int y = 0; y < source.rows; y++) // scan entire Mat
            for (int x = 0; x < source.cols; x++)
                for (int c = 0; c < 3; c++)
                    image.at<Vec3b>(y, x)[c] = saturate_cast<uchar>(alpha*(source.at<Vec3b>(y, x)[c]) + beta); // change individual values
    return image;
}

Mat GammaCorrection(const Mat &source, const double gamma)
{
    CV_Assert(gamma >= 0);

    Mat lookUpTable(1, 256, CV_8U);
    uchar* p = lookUpTable.ptr();
    for( int i = 0; i < 256; ++i)
        p[i] = saturate_cast<uchar>(pow(i / 255.0, gamma) * 255.0);

    Mat res = source.clone();
    LUT(source, lookUpTable, res);

    //Mat img_gamma_corrected;
    //hconcat(source, res, img_gamma_corrected);

    return res;
}

Mat EqualizeHistogram(const Mat &source) // histogram equalization
{
    Mat hist_equalized_image;
    cvtColor(source, hist_equalized_image, COLOR_BGR2YCrCb); // Convert the image from BGR to YCrCb color space

    // Split image into 3 channels: Y, Cr and Cb channels respectively and store it in a std::vector
    vector<Mat> vec_channels;
    split(hist_equalized_image, vec_channels);

    equalizeHist(vec_channels[0], vec_channels[0]); // Equalize the histogram of only the Y channel

    merge(vec_channels, hist_equalized_image); // Merge 3 channels in the vector to form the color image in YCrCB color space.

    cvtColor(hist_equalized_image, hist_equalized_image, COLOR_YCrCb2BGR); // Convert the histogram equalized image from YCrCb to BGR color space again

    return hist_equalized_image;
}

Mat SimplestColorBalance(const Mat &source, const float &percent) // color balance with histograms
{
    Mat dest;
    float half_percent = percent / 200.0f;
    vector<Mat> tmpsplit;

    split(source, tmpsplit); // split channels

    for (int i=0; i<3; i++) { // just the 3 first channels
        // find low and high precentile values (based on input percentile)
        Mat flat;
        tmpsplit[i].reshape(1,1).copyTo(flat);
        cv::sort(flat,flat,SORT_EVERY_ROW + SORT_ASCENDING);
        int lowval = flat.at<uchar>(cvFloor(((float)flat.cols) * half_percent));
        int highval = flat.at<uchar>(cvCeil(((float)flat.cols) * (1.0 - half_percent)));

        // saturate below the low percentile and above the high percentile
        tmpsplit[i].setTo(lowval,tmpsplit[i] < lowval);
        tmpsplit[i].setTo(highval,tmpsplit[i] > highval);

        // scale channel
        normalize(tmpsplit[i], tmpsplit[i], 0, 255, NORM_MINMAX);
    }

    merge(tmpsplit, dest); // result

    return dest;
}

///////////////////////////////////////////////////////////
//// Dilation and Erosion
///////////////////////////////////////////////////////////

Mat DilatePixels(const Mat &source, const int &dilation_size) // dilate pixels
{
    if (dilation_size <= 0)
        return source;

    // Create structuring element
    Mat element = getStructuringElement(MORPH_RECT, // MORPH_CROSS MORPH_RECT & MORPH_ELLIPSE possible
                                        Size(2 * dilation_size + 1, 2 * dilation_size + 1),
                                        Point(-1,-1));
    Mat dest;
    dilate(source, dest, element); // Apply dilation on the image

    return dest;
}

Mat ErodePixels(const Mat &source, const int &erosion_size) // erode pixels
{
    if (erosion_size <= 0)
        return source;

    // Create structuring element
    Mat element = getStructuringElement(  MORPH_RECT, // MORPH_CROSS MORPH_RECT & MORPH_ELLIPSE possible
                                              Size(2 * erosion_size + 1, 2 * erosion_size + 1),
                                              Point(-1,-1));
    Mat dest;
    erode(source, dest, element); // Apply erosion on the image

    return dest;
}

///////////////////////////////////////////////////////////
//// Shift a frame in one direction
///////////////////////////////////////////////////////////

Mat ShiftFrame(const Mat &source, const int &nb_pixels, const shift_direction &direction) // shift frame in one direction
{
    Mat dest = Mat::zeros(source.size(), source.type());

    switch (direction) { // just make a copy with Rect
        case(shift_up) :
            source(Rect(0, nb_pixels, source.cols, source.rows - nb_pixels)).copyTo(dest(Rect(0, 0, dest.cols, dest.rows - nb_pixels)));
            break;
        case(shift_right) :
            source(Rect(0, 0, source.cols - nb_pixels, source.rows)).copyTo(dest(Rect(nb_pixels, 0, source.cols - nb_pixels, source.rows)));
            break;
        case(shift_down) :
            source(Rect(0, 0, source.cols, source.rows - nb_pixels)).copyTo(dest(Rect(0, nb_pixels, source.cols, source.rows - nb_pixels)));
            break;
        case(shift_left) :
            source(Rect(nb_pixels, 0, source.cols - nb_pixels, source.rows)).copyTo(dest(Rect(0, 0, source.cols - nb_pixels, source.rows)));
            break;
    }

    return dest;
}

///////////////////////////////////////////////////////////
//// Clipping & Resizing
///////////////////////////////////////////////////////////

Mat CopyFromImage(Mat source, const Rect &frame) // copy part of an image
{
    if (source.channels() == 1)
        cvtColor(source, source, COLOR_GRAY2BGR);
    return source(frame); // just use the Rect area
}

Mat ResizeImageAspectRatio(const Mat &source, const Size &frame) // Resize image keeping aspect ratio
{
    double zoomX = double(frame.width) / source.cols; // try vertical and horizontal ratios
    double zoomY = double(frame.height) / source.rows;
    double zoom;
    if (zoomX < zoomY)
        zoom = zoomX; // the lowest fit the view
    else
        zoom = zoomY;

    Mat dest;
    resize(source, dest, Size(int(source.cols*zoom), int(source.rows*zoom)), 0, 0, INTER_AREA); // resize

    return dest;
}

///////////////////////////////////////////////////////////
//// Special copy
///////////////////////////////////////////////////////////

Mat CopyNonZero(const Mat &source1, const Mat &source2) // merge two images with zero as transparent value - first over the second
{
    if ((source1.size != source2.size) and (source1.type() != source2.type())) // images must be of same size and type
        return Mat();

    Mat result;
    source2.copyTo(result); // prepare result image

    Vec3b pixel;
    for (int x = 0; x < source1.cols; x++) // parse 1st image
        for (int y = 0; y < source1.rows; y++) {
            pixel = source1.at<Vec3b>(y, x); // current pixel
            if (pixel != Vec3b(0, 0, 0)) // is it black ?
                result.at<Vec3b>(y, x) = pixel; // copy non-zero values only
        }

    return result;
}

Mat CopyNonZeroAlpha(const Mat &source, const Mat &dest) // merge two images with alpha with zero as transparent value - first over the second
{
    if ((source.size != dest.size) and (source.type() != dest.type())) // images must be of same size and type
        return Mat();

    Mat result;
    dest.copyTo(result); // prepare result image

    Vec4b pixel;
    for (int x = 0; x < source.cols; x++) // parse 1st image
        for (int y = 0; y < source.rows; y++) {
            pixel = source.at<Vec4b>(y, x); // current pixel
            if (pixel != Vec4b(0, 0, 0, 0)) // is it black ?
                result.at<Vec4b>(y, x) = pixel; // copy non-zero values only
        }

    return result;
}

///////////////////////////////////////////////////////////
//// Alpha channel
///////////////////////////////////////////////////////////

Mat AddAlphaToImage(const Mat &source) // add alpha channel to image : transparency=black
{
  std::vector<cv::Mat> matChannels;
  cv::split(source, matChannels); // split image in separate channels

  // create alpha channel
  cv::Mat alpha;
  cv::cvtColor(source, alpha, COLOR_BGR2GRAY); // image to gray
  cv::Mat mask = 255 - (alpha == 0); // compute mask
  alpha.setTo(255, mask); // alpha to max where pixels are
  matChannels.push_back(alpha); // add alpha channel to channels stack

  Mat result;
  cv::merge(matChannels, result); // merge back channels

  return result;
}

///////////////////////////////////////////////////////////
//// Save PNG with or without transparency
///////////////////////////////////////////////////////////

void SavePNG(const std::string &filename, const Mat &source, const bool &transparency) // save PNG with or without transparency
{
    if (!transparency) {
        imwrite(filename, source); // save to PNG image without alpha
    }
    else {
        cv::Mat alpha;
        source.copyTo(alpha);
        alpha = AddAlphaToImage(alpha);
        imwrite(filename, alpha); // save to PNG image with alpha
    }
}

///////////////////////////////////////////////////////////
//// Contours
///////////////////////////////////////////////////////////

double MedianMat(Mat source) // median, used by DrawColoredContours
{
    source = source.reshape(0,1);// spread source to single row
    std::vector<double> vecFromMat;

    source.copyTo(vecFromMat); // copy source to vector vecFromMat
    std::nth_element(vecFromMat.begin(), vecFromMat.begin() + vecFromMat.size() / 2, vecFromMat.end());

    return vecFromMat[vecFromMat.size() / 2];
}

Mat DrawColoredContours(const Mat &source, const double &sigma, const int &aperture_size, const int &thickness) // Canny edge detection
{
    RNG rng(123); // controled randomization
    Mat canny_output, gray, blur;

    GaussianBlur(source, blur, Size(3,3), 0, 0); // better with a gaussian blur first
    cvtColor(blur, gray, COLOR_BGR2GRAY ); // convert to gray

    double median = MedianMat(gray); // get median value of matrix
    int lower = (int)std::max(0.0, (1.0-sigma) * median); // generate thresholds
    int upper = (int)std::min(255.0, (1.0+sigma) * median);

    vector<vector<Point>> contours; // create structure to compute
    vector<Vec4i> hierarchy;
    Canny(gray, canny_output, lower, upper, aperture_size, true ); // apply Canny
    findContours(canny_output, contours, hierarchy, RETR_TREE, CHAIN_APPROX_SIMPLE, Point(0, 0)); // find the edges
    Mat drawing = Mat::zeros(canny_output.size(), CV_8UC3);

    for (size_t i = 0; i< contours.size(); i++) { // scan the image
        Scalar color = Scalar(rng.uniform(0, 255), rng.uniform(0,255), rng.uniform(0,255)); // apply colors
        drawContours(drawing, contours, (int)i, color, thickness, 8, hierarchy, 0, Point()); // draw the lines
    }

    return drawing;
}

///////////////////////////////////////////////////////////
//// Noise filters utils
///////////////////////////////////////////////////////////

double PSNR(const Mat &source1, const Mat &source2) // noise difference between 2 images
{
    Mat s1;
    absdiff(source1, source2, s1);       // |source1 - source2|
    s1.convertTo(s1, CV_32F);  // cannot make a square on 8 bits
    s1 = s1.mul(s1);           // |source1 - source2|^2

    Scalar s = sum(s1);         // sum elements per channel

    double sse = s.val[0] + s.val[1] + s.val[2]; // sum channels

    if( sse <= 1e-10) // for small values return zero
        return 0;
    else // compute PSNR
    {
        double  mse =sse /(double)(source1.channels() * source1.total());
        double psnr = 10.0*log10((255*255)/mse);

        return psnr;
    }
}

///////////////////////////////////////////////////////////
//// Gray gradients
///////////////////////////////////////////////////////////

double GrayCurve(const int &color, const int &type, const int &begin, const int &range) // return a value transformed by a function
{
    // all functions must have continuous results in [0..1] range and have this shape : f(x) * range + begin

    double x = double(color-begin) / range; // x of f(x)

    if (range == 0) return color; // faster this way

    switch (type) {
        // good spread
        case curve_linear: return color; // linear -> the same color !
        // S-shaped
        case curve_cosinus2: return pow(cos(Pi / 2 - x * Pi / 2), 2) * range + begin; // cosinus² (better color gradient): f(x)=cos(pi/2-x*pi/2)²
        case curve_sigmoid: return 1.0 / (1 + exp(-5 * (2* (x) - 1))) * range + begin; // sigmoid (S-shaped): f(x)=1/(1 + e(-5*(2x -1))
        // fast beginning
        case curve_cosinus: return cos(Pi / 2 - x * Pi / 2) * range + begin; // cosinus (more of end color): f(x)=cos(pi/2-x*pi/2)
        case curve_cos2sqrt: return pow(cos(Pi / 2 - sqrt(x) * Pi / 2), 2) * range + begin; // cos²sqrt: f(x)=cos(pi/2−sqrt(x)*pi/2)²
        // fast ending
        case curve_power2: return pow(x, 2) * range + begin; // power2 (more of begin color): f(x)=x²
        case curve_cos2power2: return pow(cos(Pi / 2 - pow(x, 2) * Pi / 2), 2) * range + begin; // cos²power2: f(x)=cos(pi/2−x²∙pi/2)²
        case curve_power3: return pow(x, 3) * range + begin; // power3 (even more of begin color): f(x)=x³
        // undulate
        case curve_undulate: return cos(double(color - begin) / 4 * Pi) * range + begin; // undulate: f(x)=cos(x/4*pi)
        case curve_undulate2: return cos(pow(double(color - begin) * 2 * Pi / 2 + 0.5, 2)) * range + begin; // undulate²: f(x)=cos((x*2*pi)²) / 2 + 0.5
        case curve_undulate3: return (cos(Pi * Pi * pow(x+2.085,2)) / (pow(x + 2.085,3) + 8)+(x + 2.085) - 2.11) * range + begin; // undulate3: f(x) = cos(pi²∙(x+2.085)²) / ((x+2.085)³+10) + (x+2.085) − 2.11

        //case 5: return sqrt(double(color-begin) / range) * range + begin;
    }

    return color;
}

float EuclideanDistanceRadius(Point center, Point point, int radius){ // return distance between 2 points
    float distance = sqrt(std::pow(center.x - point.x, 2) + std::pow(center.y - point.y, 2));

    if (distance > radius) return radius; // no value beyond radius
        else return distance;
}

void GradientFillGray(const int &gradient_type, Mat &img, const Mat &msk, const Point &beginPoint,
                      const Point &endPoint, const int &beginColor, const int &endColor,
                      const int &curve, Rect area) // fill a 1-channel image with the mask converted to gray gradients
    // img must be 1-channel and msk 1-channel
    // img is input-output
    // area = rectangle containing the mask (speeds up the image scan) - no area given = entire image scanned
{
    if (area == Rect(0, 0, 0, 0)) // default area = 0
        area = Rect(0,0, img.cols, img.rows); // set it to image dimensions

    switch (gradient_type) {
        case (gradient_flat): { // flat = same color everywhere
            img.setTo(beginColor, msk); // fill the mask with this color
            return;
        }
        case (gradient_linear): { // grayscale is spread along the line
            int A = (endPoint.x - beginPoint.x); // horizontal difference
            int B = (endPoint.y - beginPoint.y); // vertical difference
            int C1 = A * beginPoint.x + B * beginPoint.y; // vectors
            int C2 = A * endPoint.x + B * endPoint.y;

            float CO; // will contain color values

            for (int row = area.y; row < area.y + area.height; row++) // scan mask
                for (int col = area.x; col < area.x + area.width; col++)
                    if (msk.at<uchar>(row, col) != 0) { // non-zero pixel in mask
                        int C = A * col + B * row; // "distance" for this pixel

                        if (C <= C1) CO = beginColor; // before begin point : begin color
                            else if (C >= C2) CO = endColor; // after end point : end color
                                else CO = round(GrayCurve(float(beginColor * (C2 - C) + endColor * (C - C1))/(C2 - C1),
                                                    curve, beginColor, endColor - beginColor)); // C0 = percentage between begin and end colors, "shaped" by gray curve
                        img.at<uchar>(row, col) = CO; // set pixel to image
                    }
            return; // done ! -> exit
        }
        case (gradient_doubleLinear): { // double linear = 2 times linear, just invert the vector the second time
            int A = (endPoint.x - beginPoint.x); // same comments as linear
            int B = (endPoint.y - beginPoint.y);
            int C1 = A * beginPoint.x + B * beginPoint.y;
            int C2 = A * endPoint.x + B * endPoint.y;

            float CO;

            for (int row = area.y; row < area.y + area.height; row++)
                for (int col = area.x; col < area.x + area.width; col++)
                    if (msk.at<uchar>(row, col) != 0) {
                        int C = A * col + B * row;
                        if (((C > C1) & (C < C2)) | (C >= C2) | (C == C1))  { // the only difference is we don't fill "before" the begin point
                            if (C == C1) CO = beginColor;
                                else if (C >= C2) CO = endColor;
                                    else CO = round(GrayCurve(float(beginColor * (C2 - C) + endColor * (C - C1))/(C2 - C1),
                                                        curve, beginColor, endColor - beginColor));
                            img.at<uchar>(row, col) = CO;
                        }
                    }

            Point newEndPoint; // invert vector
            newEndPoint.x = 2 * beginPoint.x - endPoint.x;
            newEndPoint.y = 2 * beginPoint.y - endPoint.y;

            A = (newEndPoint.x - beginPoint.x); // same as before, but with new inverted vector
            B = (newEndPoint.y - beginPoint.y);
            C1 = A * beginPoint.x + B * beginPoint.y;
            C2 = A * newEndPoint.x + B * newEndPoint.y;

            for (int row = area.y; row < area.y + area.height; row++)
                for (int col = area.x; col < area.x + area.width; col++)
                    if (msk.at<uchar>(row, col) != 0) {
                        int C = A * col + B * row;
                        if (((C > C1) & (C < C2)) | (C >= C2) | (C == C1)) { // once again don't fill "before" begin point
                            if (C == C1) CO = beginColor;
                                else if (C >= C2) CO = endColor;
                                    else CO = round(GrayCurve(float(beginColor * (C2 - C) + endColor * (C - C1))/(C2 - C1),
                                                        curve, beginColor, endColor - beginColor));
                            img.at<uchar>(row, col) = CO;
                        }
                    }
            return;
        }
        case (gradient_radial): { // radial = concentric circles
            float radius = std::sqrt(std::pow(beginPoint.x - endPoint.x, 2) + std::pow(beginPoint.y - endPoint.y, 2)); // maximum euclidian distance = vector length

            int CO; // will contain color values

            for (int row = area.y; row < area.y + area.height; row++) // scan entire mask
                for (int col = area.x; col < area.x + area.width; col++)
                    if (msk.at<uchar>(row, col) != 0) { // non-zero pixel in mask
                        CO = round(GrayCurve(beginColor + EuclideanDistanceRadius(beginPoint, Point(col, row), radius) / radius * (endColor - beginColor),
                                             curve, beginColor, endColor - beginColor)); // pixel in temp gradient mask = distance percentage, "shaped" by gray curve
                        img.at<uchar>(row, col) = CO;
                    }
            return;
        }
    }
}

//// Color tints

Mat AnaglyphTint(const Mat & source, const int &tint) // change tint of image to avoid disturbing colors in red-cyan anaglyph mode
{
    Mat dest = Mat::zeros(source.rows, source.cols, CV_8UC3);
    Mat splt[3]; //destination array
    split(source, splt); // split source to 3 channels (RGB)

    for (int row = 0; row < source.rows; row++) // scan entire image
        for (int col = 0; col < source.cols; col++) {
            int R, G, B;
            Vec3b color = source.at<Vec3b>(row, col); // get pixel color

            switch (tint) { // apply tint formula
                case tint_color:
                default: { // original colors
                    R = round( 1.000 * color[2] + 0.000 * color[1] + 0.000 * color[0]);
                    G = round( 0.000 * color[2] + 1.000 * color[1] + 0.000 * color[0]);
                    B = round( 0.000 * color[2] + 0.000 * color[1] + 1.000 * color[0]);
                    break;
                }
                case tint_gray: { // grayscale
                    R = round( 0.299 * color[2] + 0.587 * color[1] + 0.114 * color[0]);
                    G = round( 0.299 * color[2] + 0.587 * color[1] + 0.114 * color[0]);
                    B = round( 0.299 * color[2] + 0.587 * color[1] + 0.114 * color[0]);
                    break;
                }
                case tint_true: { // only red and cyan values
                    R = round( 0.299 * color[2] + 0.587 * color[1] + 0.114 * color[0]);
                    G = round( 0.000 * color[2] + 0.000 * color[1] + 0.000 * color[0]);
                    B = round( 0.299 * color[2] + 0.587 * color[1] + 0.114 * color[0]);
                    break;
                }
                case tint_half: { // half-colors
                    R = round( 0.299 * color[2] + 0.587 * color[1] + 0.114 * color[0]);
                    G = round( 0.000 * color[2] + 1.000 * color[1] + 0.000 * color[0]);
                    B = round( 0.000 * color[2] + 0.000 * color[1] + 1.000 * color[0]);
                    break;
                }
                case tint_optimized: { // optimized colors
                    R = round( 0.000 * color[2] + 0.700 * color[1] + 0.300 * color[0]);
                    G = round( 0.000 * color[2] + 1.000 * color[1] + 0.000 * color[0]);
                    B = round( 0.000 * color[2] + 0.000 * color[1] + 1.000 * color[0]);
                    break;
                }
                case tint_dubois: { // Dubois algorithm
                    R = round( 0.4045 * color[2] + 0.4346 * color[1] + 0.1609 * color[0]);
                    G = round( 0.3298 * color[2] + 0.6849 * color[1] - 0.0146 * color[0]);
                    B = round(-0.1162 * color[2] - 0.1902 * color[1] + 1.3099 * color[0]);
                    break;
                }
            }

            if (R < 0) R = 0; // check if values are out of range
            if (G < 0) G = 0;
            if (B < 0) B = 0;
            if (R > 255) R = 255;
            if (G > 255) G = 255;
            if (B > 255) B = 255;

            dest.at<Vec3b>(row, col) = Vec3b(B, G, R); // new pixel RGB values
        }

    return dest;
}

//// Number of colors in image

int CountRGBUniqueValues(const cv::Mat &image) // count number of RGB colors in image
{
    std::set<int> unique;

    for (Vec3b &p : cv::Mat_<Vec3b>(image)) { // iterate over pixels (assummes CV_8UC3 !)
        int n = (p[0] << 16) | (p[1] << 8) | (p[2]); // "hash" representation of the pixel
        unique.insert(n);
    }

    return unique.size();
}

//// Conversions of images to other colors spaces

cv::Mat ImgRGBtoLab(const cv::Mat &source) // convert RGB image to CIELab
{
    cv::Mat temp(source.rows, source.cols, CV_32FC3); // CIELab "image" values
    cv::Vec3b color;
    long double X, Y, Z, L, a, b;
    for (int x = 0; x < source.cols; x++) // parse image
        for  (int y = 0; y < source.rows; y++) {
            color = source.at<cv::Vec3b>(y, x); // current pixel
            RGBtoXYZ((long double)color[2] / 255.0, (long double)color[1] / 255.0, (long double)color[0] / 255.0, X, Y, Z); // convert RGB to XYZ
            XYZtoLAB(X, Y, Z, L, a, b); // convert XYZ to CIELab
            temp.at<cv::Vec3f>(y, x)[0] = float(L); // write CIELab values to temp image
            temp.at<cv::Vec3f>(y, x)[1] = float(a);
            temp.at<cv::Vec3f>(y, x)[2] = float(b);
        }

    return temp;
}

cv::Mat ImgLabToRGB(const cv::Mat &source) // convert Lab image to RGB
{
    long double R, G, B, X, Y, Z;
    cv::Mat output(source.rows, source.cols, CV_8UC3); // this will be the final RGB image
    cv::Vec3f Lab;
    for (int x = 0; x < output.cols; x++) // parse CIELab data
        for  (int y = 0; y < output.rows; y++) {
            Lab = source.at<cv::Vec3f>(y, x); // current CIELAb value
            LABtoXYZ(Lab[0], Lab[1], Lab[2], X, Y, Z); // convert it to XYZ
            XYZtoRGB(X, Y, Z, R, G, B); // then to RGB
            output.at<cv::Vec3b>(y, x)[2] = round(R * 255.0); // RGB value in final RGB image
            output.at<cv::Vec3b>(y, x)[1] = round(G * 255.0);
            output.at<cv::Vec3b>(y, x)[0] = round(B * 255.0);
        }

    return output;
}

void CreateCIELabPalettefromRGB(const int &Rvalue, const int &Gvalue, const int &Bvalue, const int &paletteSize, const int &sections,
                                const std::string filename, const bool &grid, const int &gap, const bool &invertCL) // create Lightness * Chroma palette image for one RGB color
{
    cv::Mat palette = cv::Mat::zeros(cv::Size(paletteSize + 100, paletteSize + 100), CV_8UC3); // create blank image
    long double unit = 1.0L / sections;
    long double L, a, b, X, Y, Z, R, G, B, C, H;
    RGBtoXYZ(Rvalue / 255.0, Gvalue / 255.0, Bvalue / 255.0, X, Y, Z);
    XYZtoLAB(X, Y, Z, L, a, b);
    LABtoLCHab(a, b, C, H);
    for (int l = 0; l <= sections; l++) { // for each L section
        for (int c = 0; c <= sections; c++) { // for each C section
            if (!invertCL) {
                C = unit * c;
                L = unit * l;
            }
            else {
                L = unit * c;
                C = unit * l;
            }

            LCHabToLAB(C, H, a, b);
            LABtoXYZ(L, a, b, X, Y, Z);
            XYZtoRGBNoClipping(X, Y, Z, R, G, B); // when RGB are out of gamut, return black (0,0,0)

            if (grid) { // draw grid ?
                if ((l % 10) == 0) {
                    cv::line(palette, cv::Point(round(l * unit * paletteSize), 0), cv::Point(round(l * unit * paletteSize), paletteSize), Vec3b(32, 32, 32), 1);
                }
                else
                    if ((l % 5) == 0) {
                        cv::line(palette, cv::Point(round(l * unit * paletteSize), 0), cv::Point(round(l * unit * paletteSize), paletteSize), Vec3b(8, 8, 8), 1);
                }
                if ((c % 10) == 0) {
                    cv::line(palette, cv::Point(0, round(c * unit * paletteSize)), cv::Point(paletteSize, round(c * unit * paletteSize)), Vec3b(32, 32, 32), 1);
                }
                else
                    if ((c % 5) == 0) {
                        cv::line(palette, cv::Point(0, round(c * unit * paletteSize)), cv::Point(paletteSize, round(c * unit * paletteSize)), Vec3b(8, 8, 8), 1);
                    }
            }
            cv::rectangle(palette, cv::Rect(round(l * unit * paletteSize), paletteSize - round(c * unit * paletteSize),
                                            round(unit * paletteSize) - gap, round(unit * paletteSize) - gap),
                                        cv::Vec3b(round(B * 255.0), round(G * 255.0), round(R * 255.0)), -1); // rectangle of current color
        }
    }
    cv::imwrite("LAB-palette-" + filename + ".png", palette); // write palette to file
}

void AnalyzeCIELabCurveImage(const int &sections, const std::string filename) // create CSV file of maximum Chroma values for each Lighness step from Lightness * Chroma palette image
{
    cv::Mat palette = imread(filename + ".png");
    int size = palette.cols / sections;

    std::ofstream saveCSV; // file to save
    saveCSV.open(filename + ".csv"); // save data file
    if (saveCSV) { // if successfully open
        saveCSV << "L-orig;C-orig;H;S;L;C;h\n"; //header

        for (int x = 0; x < sections; x++) { // parse image horizontaly
            int y = 0;
            while ((y < sections) and (palette.at<cv::Vec3b>(y * size + size / 2, x * size + size / 2) == cv::Vec3b(0, 0, 0))) // find first non-zero value
                y++;
            if (y == 100) // not found ?
                y = 99;

            cv::Vec3b color = palette.at<cv::Vec3b>(y * size + size / 2, x * size + size / 2); // get "maximum" pixel value

            long double H, S, L, C, h;
            HSLChfromRGB(color[2] / 255.0L, color[1] / 255.0L, color[0] / 255.0L, H, S, L, C, h); // convert RGB

            saveCSV << x << ";" << 99 - y << ";" << H * 360 << ";" << S * 100 << ";" << L * 100 << ";" << C * 100 << ";" << h * 360 << "\n"; // write result to file
        }

        saveCSV.close(); // close text file
    }
}
