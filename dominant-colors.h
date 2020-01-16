/*#-------------------------------------------------
#
#     Dominant colors library with openCV
#
#    by AbsurdePhoton - www.absurdephoton.fr
#
#                v1.2 - 2020/01/11
#
#   - eigen vectors algorithm
#   - K-means algorithm
#
#-------------------------------------------------*/

#ifndef DOMINANT_H
#define DOMINANT_H

#include "opencv2/opencv.hpp"

///////////////////////////////////////////////
////                 Eigen
///////////////////////////////////////////////

typedef struct color_node { // for eigen algorithm
    cv::Mat     mean;
    cv::Mat     cov;
    int       class_id;

    color_node *left;
    color_node *right;
} color_node;

std::vector<cv::Vec3f> DominantColorsEigenCIELab(const cv::Mat &img, const int &nb_colors, cv::Mat &quantized); // Eigen algorithm

///////////////////////////////////////////////
////                K-means
///////////////////////////////////////////////

cv::Mat DominantColorsKMeansRGB(const cv::Mat &image, const int &cluster_number, cv::Mat1f &dominant_colors); // Dominant colors with K-means from RGB image
cv::Mat DominantColorsKMeansCIELAB(const cv::Mat &image, const int &cluster_number, cv::Mat1f &dominant_colors); // Dominant colors with K-means in CIELAB space from RGB image

///////////////////////////////////////////////
////              Mean-Shift
///////////////////////////////////////////////

class Point5D { // 5-Dimensional Point
    public:
        float x;			// Spatial value
        float y;			// Spatial value
        float l;			// Lab value
        float a;			// Lab value
        float b;			// Lab value
    public:
        Point5D();													// Constructor
        ~Point5D();													// Destructor
        void MSPoint5DAccum(const Point5D &);								// Accumulate points
        void MSPoint5DCopy(const Point5D &);								// Copy a point
        float MSPoint5DColorDistance(const Point5D &);						// Compute color space distance between two points
        float MSPoint5DSpatialDistance(const Point5D &);					// Compute spatial space distance between two points
        void MSPoint5DScale(const float);									// Scale point
        void MSPOint5DSet(const float &, const float &, const float &, const float &, const float &);		// Set point value
        //void Print();												// Print 5D point
};

class MeanShift {
    public:
        float hs;				// spatial radius
        float hr;				// color radius
        std::vector<cv::Mat> IMGChannels;
    public:
        MeanShift(const float &, const float &);									// Constructor for spatial bandwidth and color bandwidth
        void MeanShiftFilteringCIELab(cv::Mat &Img);										// Mean Shift Filtering
        void MeanShiftSegmentationCIELab(cv::Mat &Img);									// Mean Shift Segmentation
};

#endif // DOMINANT_H
