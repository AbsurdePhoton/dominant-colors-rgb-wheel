/*#-------------------------------------------------
 *
 * OpenCV image tools library
 * Author: AbsurdePhoton
 *
 * v2.2 - 2020/02/06
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
 * Special image copy with zero as transparent value
 *
#-------------------------------------------------*/

#ifndef MATIMAGETOOLS_H
#define MATIMAGETOOLS

#include "opencv2/opencv.hpp"
#include <opencv2/ximgproc.hpp>
#include <QImage>

enum shift_direction{shift_up=1, shift_right, shift_down, shift_left}; // directions for shift function
enum gradientType {gradient_flat, gradient_linear, gradient_doubleLinear, gradient_radial}; // gradient types
enum curveType {curve_linear, curve_cosinus2, curve_sigmoid, curve_cosinus, curve_cos2sqrt,
                curve_power2, curve_cos2power2, curve_power3, curve_undulate, curve_undulate2, curve_undulate3}; // gray curve types
enum anaglyphTint {tint_color, tint_gray, tint_true, tint_half, tint_optimized, tint_dubois}; // red/cyan anaglyph tints

//// Conversions between QImage, QPixmap & Mat
///
cv::Mat QImage2Mat(const QImage &source); // convert QImage to Mat
cv::Mat QPixmap2Mat(const QPixmap &source); // convert QPixmap to Mat

QImage  Mat2QImage(const cv::Mat &source); // convert Mat to QImage
QPixmap Mat2QPixmap(const cv::Mat &source); // convert Mat to QPixmap
QPixmap Mat2QPixmapResized(const cv::Mat &source, const int &width, const int &height, const bool &smooth); // convert Mat to resized QPixmap
QImage  cvMatToQImage(const cv::Mat &source); // another implementation Mat type wise

//// Brightness, Contrast, Gamma, Equalize, Balance

cv::Mat BrightnessContrast(const cv::Mat &source, const double &alpha, const int &beta); // brightness and contrast
cv::Mat GammaCorrection(const cv::Mat &source, const double gamma); // gamma correction
cv::Mat EqualizeHistogram(const cv::Mat &source); // histogram equalization
cv::Mat SimplestColorBalance(const cv::Mat &source, const float &percent); // color balance with histograms

//// Dilation and Erosion

cv::Mat DilatePixels(const cv::Mat &source, const int &dilation_size); // dilate pixels
cv::Mat ErodePixels(const cv::Mat &source, const int &erosion_size); // erode pixels

//// Shift a frame in one direction

cv::Mat ShiftFrame(const cv::Mat &source, const int &nb_pixels, const shift_direction &direction); // shift frame in one direction

//// Clipping & Resizing

cv::Mat CopyFromImage(cv::Mat source, const cv::Rect &frame); // copy part of an image
cv::Mat ResizeImageAspectRatio(const cv::Mat &source, const cv::Size &frame); // Resize image keeping aspect ratio

//// Special copy
cv::Mat CopyNonZero(const cv::Mat &source1, const cv::Mat &source2); // merge two images with zero as transparent value - first over the second
cv::Mat CopyNonZeroAlpha(const cv::Mat &source, const cv::Mat &dest); // merge two images with alpha with zero as transparent value - first over the second

//// Alpha channel
cv::Mat AddAlphaToImage(const cv::Mat &source); // add alpha channel to image

//// Contours

cv::Mat DrawColoredContours(const cv::Mat &source, const double &sigma, const int &apertureSize, const int &thickness); // draw colored contours of an image

//// Noise filters utils

double PSNR(const cv::Mat &source1, const cv::Mat &source2); // noise difference between 2 images

//// Gray gradients

void GradientFillGray(const int &gradient_type, cv::Mat &img, const cv::Mat &msk,
                      const cv::Point &beginPoint, const cv::Point &endPoint,
                      const int &beginColor, const int &endColor,
                      const int &curve, cv::Rect area = cv::Rect(0, 0, 0, 0)); // fill a 1-channel image with the mask converted to gray gradients

//// Color tints

cv::Mat AnaglyphTint(const cv::Mat & source, const int &tint); // change tint of image to avoid disturbing colors in red-cyan anaglyph mode

//// Number of colors in image

int CountRGBUniqueValues(const cv::Mat &image); // count number of RGB colors in image

//// Conversion of images to other colors spaces

cv::Mat ImgRGBtoLab(const cv::Mat &source); // convert RGB image to CIELab CV_32FC3
cv::Mat ImgLabToRGB(const cv::Mat &source); // convert CV_32FC3 Lab image to RGB
void CreateCIELabPalettefromRGB(const int &Rvalue, const int &Gvalue, const int &Bvalue, const int &paletteSize, const int &sections,
                                const std::string filename, const bool &grid, const int &gap, const bool &invertCL); // create Lightness * Chroma palette image for one RGB color
void AnalyzeCIELabCurveImage(const int &sections, const std::string filename); // create CSV file of maximum Chroma values for each Lighness step from Lightness * Chroma palette image

#endif // MATIMAGETOOLS_H
