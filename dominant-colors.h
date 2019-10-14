/*#-------------------------------------------------
#
#     Dominant colors library with openCV
#
#    by AbsurdePhoton - www.absurdephoton.fr
#
#                v0 - 2019/10/10
#
#   - eigen vectors algorithm
#   - K-means algorithm
#
#-------------------------------------------------*/

#ifndef DOMINANT_H
#define DOMINANT_H

#include "opencv2/opencv.hpp"

typedef struct color_node { // tree node
    cv::Mat mean; // mean of this node
    cv::Mat cov; // covariance
    uchar class_id; // class ID

    color_node  *left; // branches of tree
    color_node  *right;
} color_node;

std::vector<cv::Vec3b> DominantColorsEigen(const cv::Mat &img, const int &nb_colors, cv::Mat *quantized);

cv::Mat DominantColorsKMeans(const cv::Mat &image, const int &cluster_number, cv::Mat1f *dominant_colors);

#endif // DOMINANT_H
