/*#-------------------------------------------------
#
#     Dominant colors library with openCV
#
#    by AbsurdePhoton - www.absurdephoton.fr
#
#                v1.3 - 2020/02/06
#
#   - sectored means (my own) algorithm
#   - eigen vectors algorithm
#   - K-means algorithm
#
#-------------------------------------------------*/

#ifndef DOMINANT_H
#define DOMINANT_H

#include "opencv2/opencv.hpp"

///////////////////////////////////////////////
////         Sectored-Means algorithm
///////////////////////////////////////////////
// Original algorithm by AbsurdePhoton

struct struct_color_sectors { // struct to store color range values
    std::string name; // color name
    int hue; // main color hue (H from HSL)
    int begin, end; // range of hue
    int R, G, B; // equivalent in RGB with max saturation
    long double max; // max Chroma value for this hue
};
const int nb_color_sectors = 24;
static const struct_color_sectors color_sectors[nb_color_sectors] = { // color range values
    {"red",                    0,    353, 368,      255,   0,   0,       84},
    {"red-orange",            15,      8,  23,      255,  64,   0,       78},
    {"orange",                30,     23,  38,      255, 127,   0,       68},
    {"orange-yellow",         45,     38,  53,      255, 191,   0,       67},
    {"yellow",                60,     53,  68,      255, 255,   0,       78},
    {"yellow-chartreuse",     75,     68,  83,      191, 255,   0,       80},
    {"chartreuse",            90,     83,  98,      127, 255,   0,       87},
    {"green-chartreuse",     105,     98, 113,       64, 255,   0,       93},
    {"green",                120,    113, 128,        0, 255,   0,       95},
    {"green-spring",         135,    128, 143,        0, 255,  64,       88},
    {"spring",               150,    143, 158,        0, 255, 127,       72},
    {"cyan-spring",          165,    158, 173,        0, 255, 191,       54},
    {"cyan",                 180,    173, 188,        0, 255, 255,       41},
    {"cyan-dodger",          195,    188, 203,        0, 191, 255,       38},
    {"dodger",               210,    203, 218,        0, 127, 255,       49},
    {"blue-dodger",          225,    218, 233,        0,  64, 255,       89},
    {"blue",                 240,    233, 248,        0,   0, 255,      102},
    {"blue-indigo",          255,    248, 263,       64,   0, 255,      102},
    {"indigo",               270,    263, 278,      127,   0, 255,      100},
    {"magenta-indigo",       285,    278, 293,      191,   0, 255,       95},
    {"magenta",              300,    293, 308,      255,   0, 255,       92},
    {"pink-magenta",         315,    308, 323,      255,   0, 191,       76},
    {"pink",                 330,    323, 338,      255,   0, 127,       68},
    {"red-pink",             345,    338, 353,      255,   0,  64,       72}
};

struct struct_color_category { // struct to store a category
    int begin, end; // range
    int R, G, B; // RGB equivalent with max saturation
    std::string name; // name
};
const int nb_lightness_categories = 6;
static const struct_color_category lightness_categories[nb_lightness_categories] = { // Lightness categories
    {  0,  25,        0,   0,   0,      "black"},
    { 25,  45,       48,  48,  48,      "near-black"},
    { 45,  65,       92,  92,  92,      "dark"},
    { 65,  85,      144, 144, 144,      "medium"},
    { 85,  96,      192, 192, 192,      "light"},
    { 96, 999,      255, 255, 255,      "white"}
};

const int nb_chroma_categories = 5;
static const struct_color_category chroma_categories[nb_chroma_categories] = { // Chroma categories (from CIE LCHab
    {  0,  13,      123, 118, 115,      "gray"},
    { 13,  38,      150, 108, 101,      "very dull"},
    { 38,  63,      184,  90,  77,      "dull"},
    { 63,  88,      215,  58,  54,      "intense"},
    { 88, 999,      255,   0,   0,      "very intense"}
};


int WhichColorSector(const int &H); // get the color sector of a given Hue in HSL
int WhichLightnessCategory(const int &L); // get the Lightness category (L from CIELab)
int WhichChromaCategory(const int &C, const int &colorSector); // get the Chroma category (C from CIE LChab)

void SectoredMeansSegmentationLevels(const cv::Mat &image, const int &nb_chroma, cv::Mat &quantized); // image segmentation by color sector mean (H from HSL)
void SectoredMeansSegmentationCategories(const cv::Mat &image, cv::Mat &quantized); // image segmentation by color sector mean (H from HSL)

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

std::vector<cv::Vec3f> DominantColorsEigenCIELab(const cv::Mat &img, const int &nb_colors, cv::Mat &quantized); // Eigen algorithm with CIELab values in range [0..1]

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
