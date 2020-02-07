/*#-------------------------------------------------
#
#     Dominant colors library with openCV
#               in 3D color spaces
#
#    by AbsurdePhoton - www.absurdephoton.fr
#
#                v1.3 - 2020/02/06
#
#   - eigen vectors algorithm
#   - K-means algorithm
#
#-------------------------------------------------*/

#include <opencv2/opencv.hpp>

#include "dominant-colors.h"
#include "color-spaces.h"
#include "mat-image-tools.h"

///////////////////////////////////////////////
////         Sectored-Means algorithm
///////////////////////////////////////////////
// Original algorithm by AbsurdePhoton

int WhichColorSector(const int &H) // get the color sector of a given Hue in HSL (H in degrees)
{
    int h = H % 360; // Hue in [0..359]
    if (h < 8) // reds come across the circle origin
        h += 360.0; // get a continuous range

    int hPrime; // value to return
    for (hPrime = 0; hPrime < nb_color_sectors; hPrime++) // parse Hue ranges
        if ((h >= color_sectors[hPrime].begin) and (h < color_sectors[hPrime].end)) // is this the right range ?
            break; // yes !

    if (hPrime == nb_color_sectors) // didn't find a range ?
        hPrime = -1; // dummy value

    return hPrime; // return color sector
}

int WhichLightnessCategory(const int &L) // get the Lightness category (L from CIELab)
{
    int l;
    for (l = 0; l < nb_lightness_categories; l++) // parse Lightness ranges
        if ((L >= lightness_categories[l].begin) and (L < lightness_categories[l].end)) // is this the right range ?
            break; // yes !

    if (l == nb_lightness_categories) // didn't find a range ?
        l = -1; // dummy value

    return l; // return Lightness catgory
}

int WhichChromaCategory(const int &C, const int &colorSector) // get the Chroma category (C from CIE LChab)
{
    long double max = color_sectors[colorSector].max; // get highest Chroma for color sector of value to compute
    int Crectified = round((long double)C / max * 100.0L); // Chroma in [0..100]

    int c;
    for (c = 0; c < nb_chroma_categories; c++) // parse Chroma ranges
        if ((Crectified >= chroma_categories[c].begin) and (Crectified < chroma_categories[c].end)) // is this the right range ?
            break; // yes !

    if (c == nb_chroma_categories) // didn't find a range ?
        c = -1; // dummy value

    return c; // return Chroma category
}

void SectoredMeansSegmentationLevels(const cv::Mat &image, const int &nb_levels, cv::Mat &quantized) // image segmentation by color sector mean (H from HSL)
{
    quantized = cv::Mat::zeros(image.rows, image.cols, CV_8UC3); // init quantized image = black
    cv::Mat mask_sector[nb_color_sectors][nb_levels][nb_levels]; // mask for each sector
    for (int s = 0; s < nb_color_sectors; s++) // init these masks to black
        for (int l = 0; l < nb_levels; l++)
            for (int c = 0; c < nb_levels; c++)
                mask_sector[s][l][c] = cv::Mat::zeros(image.rows, image.cols, CV_8UC3); // zero mask of same size than image

    cv::Vec3b RGB;
    long double H, S, L, C, h, r, g, b;
    for (int x = 0; x < image.cols; x++) // parse image
        for (int y = 0; y < image.rows; y++) {
            RGB = image.at<cv::Vec3b>(y, x); // get current pixel
            HSLChfromRGB((long double)RGB[2] / 255.0, (long double)RGB[1] / 255.0, (long double)RGB[0] / 255.0, H, S, L, C, h); // get "HSLC" from RGB

            int l = int(L * nb_levels); // get Lightness range of current pixel
            if (l >= nb_levels - 1) // stay in range
                l = nb_levels - 1;
            int c = int(C * nb_levels); // get Chroma range of current pixel
            if (c >= nb_levels - 1) // stay in range
                c = nb_levels - 1;

            H *= 360.0; // Hue in degrees
            int s = WhichColorSector(H); // get color sector of current pixel
            GammaCorrectionToSRGB(RGB[2] / 255.0, RGB[1] / 255.0, RGB[0] / 255.0, r, g, b); // RGB value in linear space => to compute mean
            mask_sector[s][l][c].at<cv::Vec3b>(y, x) = cv::Vec3b(round(b * 255.0), round(g * 255.0), round(r * 255.0)); // copy pixel in the right sector mask
        }

    int nb_pal = 0;
    for (int s = 0; s < nb_color_sectors; s++) { // for each sector
        for (int l = 0; l < nb_levels; l++)
            for (int c = 0; c < nb_levels; c++) {
                cv::Mat gray; // gray mask
                cv::cvtColor(mask_sector[s][l][c], gray, cv::COLOR_BGR2GRAY); // countNonZero works only on a 1-dimension Mat
                if (cv::countNonZero(gray) > 0) { // does the sector mask contain any values ?
                    cv::Scalar mean = cv::mean(mask_sector[s][l][c], gray); // compute mean of entire sector
                    GammaCorrectionFromSRGB(mean[2] / 255.0, mean[1] / 255.0, mean[0] / 255.0, r, g, b); // get rgb back from sRGB mean value
                    quantized.setTo(cv::Vec3b(round(b * 255.0), round(g * 255.0), round(r * 255.0)), gray); // plot this mean color to quantized image from mask
                    nb_pal++; // one more color in palette
            }
        }
    }
}

void SectoredMeansSegmentationCategories(const cv::Mat &image, cv::Mat &quantized) // image segmentation by color sector mean (H from HSL)
{
    quantized = cv::Mat::zeros(image.rows, image.cols, CV_8UC3); // init quantized image = black
    cv::Mat mask_sector[nb_color_sectors][nb_lightness_categories][nb_chroma_categories]; // mask for each sector
    for (int s = 0; s < nb_color_sectors; s++) // init these masks to black
        for (int l = 0; l < nb_lightness_categories; l++)
            for (int c = 0; c < nb_chroma_categories; c++)
                mask_sector[s][l][c] = cv::Mat::zeros(image.rows, image.cols, CV_8UC3); // zero mask of same size than image

    cv::Vec3b RGB;
    long double H, S, L, C, h, r, g, b;
    for (int x = 0; x < image.cols; x++) // parse image
        for (int y = 0; y < image.rows; y++) {
            RGB = image.at<cv::Vec3b>(y, x); // get current pixel
            HSLChfromRGB((long double)RGB[2] / 255.0, (long double)RGB[1] / 255.0, (long double)RGB[0] / 255.0, H, S, L, C, h); // get "HSLC" from RGB

            int s = WhichColorSector(round(H * 360.0)); // get sector and C and L categories for current pixel
            int l = WhichLightnessCategory(round(L * 100.0));
            int c = WhichChromaCategory(round(C * 100.0), s);
            GammaCorrectionToSRGB(RGB[2] / 255.0, RGB[1] / 255.0, RGB[0] / 255.0, r, g, b); // RGB value in linear space => to compute mean
            mask_sector[s][l][c].at<cv::Vec3b>(y, x) = cv::Vec3b(round(b * 255.0), round(g * 255.0), round(r * 255.0)); // copy pixel in the right sector mask
        }

    int nb_pal = 0;
    for (int s = 0; s < nb_color_sectors; s++) { // for each sector
        for (int l = 0; l < nb_lightness_categories; l++)
            for (int c = 0; c < nb_chroma_categories; c++) {
                cv::Mat gray; // gray mask
                cv::cvtColor(mask_sector[s][l][c], gray, cv::COLOR_BGR2GRAY); // countNonZero works only on a 1-dimension Mat
                if (cv::countNonZero(gray) > 0) { // does the sector mask contain any values ?
                    cv::Scalar mean = cv::mean(mask_sector[s][l][c], gray); // compute mean of entire sector
                    GammaCorrectionFromSRGB(mean[2] / 255.0, mean[1] / 255.0, mean[0] / 255.0, r, g, b); // get rgb back from sRGB mean value
                    quantized.setTo(cv::Vec3b(round(b * 255.0), round(g * 255.0), round(r * 255.0)), gray); // plot this mean color to quantized image from mask
                    nb_pal++; // one more color in palette
            }
        }
    }
}

////////////////////////////////////////////////////////////
////                Eigen vectors algorithm
////////////////////////////////////////////////////////////

// code adapted from Utkarsh Sinha, no more 256 colors limit by using int for "class id"
// source : http://aishack.in/tutorials/dominant-color/
// works for any color space, because values are in range [0..1]
// only implemented CIELab though

std::vector<color_node*> GetLeaves(color_node *root)
{
    std::vector<color_node*> ret;
    std::queue<color_node*> queue;
    queue.push(root);

    while (queue.size() > 0) {
        color_node *current = queue.front();
        queue.pop();

        if (current->left && current->right) {
            queue.push(current->left);
            queue.push(current->right);
            continue;
        }

        ret.push_back(current);
    }

    return ret;
}

std::vector<cv::Vec3f> GetDominantColors(color_node *root)
{
    std::vector<color_node*> leaves = GetLeaves(root);
    std::vector<cv::Vec3f> ret;

    for (unsigned int i=0; i < leaves.size(); i++) {
        cv::Mat mean = leaves[i]->mean;
        ret.push_back(cv::Vec3f(mean.at<double>(0),
                                mean.at<double>(1),
                                mean.at<double>(2)));
    }

    return ret;
}

int GetNextClassId(color_node *root) {
    int maxid = 0;
    std::queue<color_node*> queue;
    queue.push(root);

    while (queue.size() > 0) {
        color_node* current = queue.front();
        queue.pop();

        if (current->class_id > maxid)
            maxid = current->class_id;

        if (current->left != NULL)
            queue.push(current->left);

        if (current->right)
            queue.push(current->right);
    }

    return maxid + 1;
}

void GetClassMeanCov(cv::Mat img, cv::Mat classes, color_node *node)
{
    const int width = img.cols;
    const int height = img.rows;
    const int class_id = node->class_id;

    cv::Mat mean = cv::Mat(3, 1, CV_64FC1, cv::Scalar(0));
    cv::Mat cov = cv::Mat(3, 3, CV_64FC1, cv::Scalar(0));

    // start out with the average color
    double pix_count = 0;
    for (int y = 0; y < height; y++) {
        cv::Vec3f* ptr = img.ptr<cv::Vec3f>(y);
        char16_t* ptrClass = classes.ptr<char16_t>(y);
        for (int x=0; x < width; x++) {
            if (ptrClass[x] != class_id)
                continue;

            cv::Vec3f color = ptr[x];
            cv::Mat scaled = cv::Mat(3, 1, CV_64FC1, cv::Scalar(0));
            scaled.at<double>(0) = color[0];
            scaled.at<double>(1) = color[1];
            scaled.at<double>(2) = color[2];

            mean += scaled;
            cov = cov + (scaled * scaled.t());

            pix_count++;
        }
    }

    cov = cov - (mean * mean.t()) / pix_count;
    mean = mean / pix_count;

    // node mean and covariance
    node->mean = mean.clone();
    node->cov = cov.clone();

    return;
}

void PartitionClass(cv::Mat img, cv::Mat classes, char16_t nextid, color_node *node) {
    const int width = img.cols;
    const int height = img.rows;
    const int class_id = node->class_id;

    const int new_id_left = nextid;
    const int new_id_right = nextid + 1;

    cv::Mat mean = node->mean;
    cv::Mat cov = node->cov;
    cv::Mat eigen_values, eigen_vectors;
    cv::eigen(cov, eigen_values, eigen_vectors);

    cv::Mat eig = eigen_vectors.row(0);
    cv::Mat comparison_value = eig * mean;

    node->left = new color_node();
    node->right = new color_node();

    node->left->class_id = new_id_left;
    node->right->class_id = new_id_right;

    // start out with average color
    for (int y = 0; y < height; y++) {
        cv::Vec3f* ptr = img.ptr<cv::Vec3f>(y);
        char16_t* ptr_class = classes.ptr<char16_t>(y);
        for (int x = 0; x < width; x++) {
            if (ptr_class[x] != class_id)
                continue;

            cv::Vec3f color = ptr[x];
            cv::Mat scaled = cv::Mat(3, 1,
                                  CV_64FC1,
                                  cv::Scalar(0));

            scaled.at<double>(0) = color[0];
            scaled.at<double>(1) = color[1];
            scaled.at<double>(2) = color[2];

            cv::Mat this_value = eig * scaled;

            if (this_value.at<double>(0, 0) <= comparison_value.at<double>(0, 0)) {
                ptr_class[x] = new_id_left;
            } else {
                ptr_class[x] = new_id_right;
            }
        }
    }
    return;
}

cv::Mat GetQuantizedImage(cv::Mat classes, color_node *root) {
    std::vector<color_node*> leaves = GetLeaves(root);

    const int height = classes.rows;
    const int width = classes.cols;
    cv::Mat ret(height, width, CV_32FC3, cv::Scalar(0));

    for (int y = 0; y < height; y++) {
        char16_t *ptr_class = classes.ptr<char16_t>(y);
        cv::Vec3f *ptr = ret.ptr<cv::Vec3f>(y);
        for (int x = 0; x < width; x++) {
            char16_t pixel_class = ptr_class[x];
            for (unsigned int i = 0; i < leaves.size(); i++) {
                if (leaves[i]->class_id == pixel_class) {
                    ptr[x] = cv::Vec3f(leaves[i]->mean.at<double>(0),
                                       leaves[i]->mean.at<double>(1),
                                       leaves[i]->mean.at<double>(2));
                }
            }
        }
    }

    return ret;
}

color_node* GetMaxEigenValueNode(color_node *current) {
    double max_eigen = -1;
    cv::Mat eigen_values, eigen_vectors;

    std::queue<color_node*> queue;
    queue.push(current);

    color_node *ret = current;
    if (!current->left && !current->right)
        return current;

    while (queue.size() > 0) {
        color_node *node = queue.front();
        queue.pop();

        if (node->left && node->right) {
            queue.push(node->left);
            queue.push(node->right);
            continue;
        }

        cv::eigen(node->cov, eigen_values, eigen_vectors);
        double val = eigen_values.at<double>(0);
        if (val > max_eigen) {
            max_eigen = val;
            ret = node;
        }
    }

    return ret;
}

std::vector<cv::Vec3f> DominantColorsEigenCIELab(const cv::Mat &img, const int &nb_colors, cv::Mat &quantized) // Eigen algorithm
{
    // CIELab values are in range [0..1]

    const int width = img.cols;
    const int height = img.rows;

    cv::Mat classes = cv::Mat(height, width, CV_16UC1, cv::Scalar(1));
    color_node *root = new color_node();

    root->class_id = 1;
    root->left = NULL;
    root->right = NULL;

    color_node *next = root;
    GetClassMeanCov(img, classes, root);

    for (int i = 0; i < nb_colors - 1; i++) {
        next = GetMaxEigenValueNode(root);
        PartitionClass(img, classes, GetNextClassId(root), next);
        GetClassMeanCov(img, classes, next->left);
        GetClassMeanCov(img, classes, next->right);
    }

    std::vector<cv::Vec3f> colors = GetDominantColors(root);
    quantized = GetQuantizedImage(classes, root); // the quantized image has values in range [0..1]
    return colors;
}

////////////////////////////////////////////////////////////
////                K_means algorithm
////////////////////////////////////////////////////////////

cv::Mat DominantColorsKMeansRGB(const cv::Mat &source, const int &nb_clusters, cv::Mat1f &dominant_colors) // Dominant colors with K-means from RGB image
{
    const unsigned int data_size = source.rows * source.cols; // size of source
    cv::Mat data = source.reshape(1, data_size); // reshape the source to a single line
    data.convertTo(data, CV_32F); // floats needed by K-means

    std::vector<int> indices; // color clusters
    cv::Mat1f colors; // colors output
    cv::kmeans(data, nb_clusters, indices, cv::TermCriteria(cv::TermCriteria::EPS+cv::TermCriteria::COUNT, 100, 1.0),
               100, cv::KMEANS_PP_CENTERS, colors); // ending criterias : 100 iterations and epsilon=1.0

    for (unsigned int i = 0 ; i < data_size ; i++ ) { // replace colors in image data
        data.at<float>(i, 0) = colors(indices[i], 0);
        data.at<float>(i, 1) = colors(indices[i], 1);
        data.at<float>(i, 2) = colors(indices[i], 2);
    }

    cv::Mat output_image = data.reshape(3, source.rows); // RGB channels needed for output
    output_image.convertTo(output_image, CV_8UC3); // BGR image

    dominant_colors = colors; // save colors clusters

    return output_image; // return quantized image
}

cv::Mat DominantColorsKMeansCIELAB(const cv::Mat &source, const int &nb_clusters, cv::Mat1f &dominant_colors) // Dominant colors with K-means in CIELAB space from RGB image
{
    cv::Mat temp = ImgRGBtoLab(source);

    const unsigned int data_size = source.rows * source.cols; // size of source
    cv::Mat1f data = temp.reshape(1, data_size); // reshape CIELab data to a single line

    std::vector<int> indices; // color clusters
    cv::Mat1f colors; // colors output
    cv::kmeans(data, nb_clusters, indices, cv::TermCriteria(cv::TermCriteria::EPS+cv::TermCriteria::COUNT, 100, 1.0),
               100, cv::KMEANS_PP_CENTERS, colors); // k-means on CIELab data, ending criterias : 100 iterations and epsilon=1.0

    for (unsigned int i = 0 ; i < data_size ; i++ ) { // replace colors in CIELab data
        data.at<float>(i, 0) = colors(indices[i], 0);
        data.at<float>(i, 1) = colors(indices[i], 1);
        data.at<float>(i, 2) = colors(indices[i], 2);
    }

    cv::Mat output_image = data.reshape(3, source.rows); // 3 channels needed for output
    cv::Mat output_temp = ImgLabToRGB(output_image);

    dominant_colors = colors; // save colors clusters in CIELab color space (all values in range [0..1])
    return output_temp; // return quantized image
}

////////////////////////////////////////////////////////////
////                  Mean-Shift algorithm
////////////////////////////////////////////////////////////

// adpated from Bingyang Liu to directly work in CIELab color space
// source : https://github.com/bbbbyang/Mean-Shift-Segmentation

// Definitions
#define MS_MAX_NUM_CONVERGENCE_STEPS	5										// up to 10 steps are for convergence
#define MS_MEAN_SHIFT_TOL_COLOR			0.3										// minimum mean color shift change
#define MS_MEAN_SHIFT_TOL_SPATIAL		0.3										// minimum mean spatial shift change
const int dxdy[][2] = {{-1,-1},{-1,0},{-1,1},{0,-1},{0,1},{1,-1},{1,0},{1,1}};	// region growing

Point5D::Point5D() // Constructor
{
    x = -1;
    y = -1;
}

Point5D::~Point5D() // Destructor
{
}

void Point5D::MSPoint5DAccum(const Point5D &Pt) // Accumulate points
{
    x += Pt.x;
    y += Pt.y;
    l += Pt.l;
    a += Pt.a;
    b += Pt.b;
}

void Point5D::MSPoint5DCopy(const Point5D &Pt) // Copy a point
{
    x = Pt.x;
    y = Pt.y;
    l = Pt.l;
    a = Pt.a;
    b = Pt.b;
}

float Point5D::MSPoint5DColorDistance(const Point5D &Pt) // Color space distance between two points
{
    return sqrtf(powf(l * 100.0 - Pt.l * 100.0, 2) + powf(a * 127.0 - Pt.a * 127.0, 2) + powf(b * 127.0 - Pt.b * 127.0, 2)); // CIE76 color difference - not very good but fast
    //return distanceCIEDE2000LAB(Pt.l, Pt.a, Pt.b, l, a, b); // takes too much time
}

float Point5D::MSPoint5DSpatialDistance(const Point5D &Pt) // Spatial space distance between two points
{
    return sqrtf(powf(x - Pt.x, 2) + powf(y - Pt.y, 2)); // euclidian distance
}

void Point5D::MSPoint5DScale(const float scale) // Scale point
{
    x *= scale;
    y *= scale;
    l *= scale;
    a *= scale;
    b *= scale;
}

void Point5D::MSPOint5DSet(const float &px, const float &py, const float &pl, const float &pa, const float &pb) // Set point value
{
    x = px;
    y = py;
    l = pl;
    a = pa;
    b = pb;
}

MeanShift::MeanShift(const float &s, const float &r) // Constructor for spatial bandwidth and color bandwidth
{
    hs = s;
    hr = r;
}

void MeanShift::MeanShiftFilteringCIELab(cv::Mat &Img) // Mean Shift Filtering
{
    int ROWS = Img.rows;			// Get row number
    int COLS = Img.cols;			// Get column number
    split(Img, IMGChannels);		// Split Lab color

    Point5D PtCur;					// Current point
    Point5D PtPrev;					// Previous point
    Point5D PtSum;					// Sum vector of the shift vector
    Point5D Pt;
    int Left;						// Left boundary
    int Right;						// Right boundary
    int Top;						// Top boundary
    int Bottom;						// Bottom boundary
    int NumPts;						// number of points in a hypersphere
    int step;

    for(int i = 0; i < ROWS; i++) {
        for(int j = 0; j < COLS; j++) {
            Left = (j - hs) > 0 ? (j - hs) : 0;						// Get Left boundary of the filter
            Right = (j + hs) < COLS ? (j + hs) : COLS;				// Get Right boundary of the filter
            Top = (i - hs) > 0 ? (i - hs) : 0;						// Get Top boundary of the filter
            Bottom = (i + hs) < ROWS ? (i + hs) : ROWS;				// Get Bottom boundary of the filter
            PtCur.MSPOint5DSet(i, j, (float)IMGChannels[0].at<float>(i, j), (float)IMGChannels[1].at<float>(i, j), (float)IMGChannels[2].at<float>(i, j)); // Set current point
            step = 0;				// count the times
            do {
                PtPrev.MSPoint5DCopy(PtCur);						// Set the original point and previous one
                PtSum.MSPOint5DSet(0, 0, 0, 0, 0);					// Initial Sum vector
                NumPts = 0;											// Count number of points that satisfy the bandwidths
                for(int hx = Top; hx < Bottom; hx++) {
                    for(int hy = Left; hy < Right; hy++) {
                        Pt.MSPOint5DSet(hx, hy, (float)IMGChannels[0].at<float>(hx, hy), (float)IMGChannels[1].at<float>(hx, hy), (float)IMGChannels[2].at<float>(hx, hy)); // Set point in the spatial bandwidth
                        if (Pt.MSPoint5DColorDistance(PtCur) < hr) { // Check it satisfied color bandwidth or not
                            PtSum.MSPoint5DAccum(Pt);				// Accumulate the point to Sum vector
                            NumPts++;								// Count
                        }
                    }
                }
                PtSum.MSPoint5DScale(1.0 / NumPts);					// Scale Sum vector to average vector
                PtCur.MSPoint5DCopy(PtSum);							// Get new origin point
                step++;												// One time end
            } while((PtCur.MSPoint5DColorDistance(PtPrev) > MS_MEAN_SHIFT_TOL_COLOR) && (PtCur.MSPoint5DSpatialDistance(PtPrev) > MS_MEAN_SHIFT_TOL_SPATIAL)
                        && (step < MS_MAX_NUM_CONVERGENCE_STEPS)); // filter iteration to end

            Img.at<cv::Vec3f>(i, j) = cv::Vec3f(PtCur.l, PtCur.a, PtCur.b); // Copy result to image
        }
    }
}

void MeanShift::MeanShiftSegmentationCIELab(cv::Mat &Img) // Mean Shift Segmentation
{
    int ROWS = Img.rows;			// Get row number
    int COLS = Img.cols;			// Get column number

    Point5D PtCur;                  // Current point
    Point5D Pt;

    int label = -1;					// Label number
    float *Mode = new float [ROWS * COLS * 3];					// Store the Lab color of each region
    int *MemberModeCount = new int [ROWS * COLS];				// Store the number of each region
    memset(MemberModeCount, 0, ROWS * COLS * sizeof(int));		// Initialize the MemberModeCount
    split(Img, IMGChannels); // split image

    // Label for each point
    int **Labels = new int *[ROWS];
    for(int i = 0; i < ROWS; i++)
        Labels[i] = new int [COLS];

    // Initialization
    for(int i = 0; i < ROWS; i++) {
        for(int j = 0; j < COLS; j++) {
            Labels[i][j] = -1;
        }
    }

    for(int i = 0; i < ROWS; i++) {
        for(int j = 0; j < COLS; j ++) {
            if (Labels[i][j] < 0) { // If the point is not being labeled
                Labels[i][j] = ++label;		// Give it a new label number
                PtCur.MSPOint5DSet(i, j, (float)IMGChannels[0].at<float>(i, j), (float)IMGChannels[1].at<float>(i, j), (float)IMGChannels[2].at<float>(i, j)); // Get the point

                // Store each value of Lab
                Mode[label * 3 + 0] = PtCur.l;
                Mode[label * 3 + 1] = PtCur.a;
                Mode[label * 3 + 2] = PtCur.b;

                // Region Growing 8 Neighbours
                std::vector<Point5D> NeighbourPoints;
                NeighbourPoints.push_back(PtCur);
                while(!NeighbourPoints.empty()) {
                    Pt = NeighbourPoints.back();
                    NeighbourPoints.pop_back();

                    // Get 8 neighbours
                    for(int k = 0; k < 8; k++) {
                        int hx = Pt.x + dxdy[k][0];
                        int hy = Pt.y + dxdy[k][1];
                        if ((hx >= 0) && (hy >= 0) && (hx < ROWS) && (hy < COLS) && (Labels[hx][hy] < 0)) {
                            Point5D P;
                            P.MSPOint5DSet(hx, hy, (float)IMGChannels[0].at<float>(hx, hy), (float)IMGChannels[1].at<float>(hx, hy), (float)IMGChannels[2].at<float>(hx, hy));

                            // Check the color
                            if (PtCur.MSPoint5DColorDistance(P) < hr) { // Satisfied the color bandwidth
                                Labels[hx][hy] = label;				// Give the same label
                                NeighbourPoints.push_back(P);		// Push it into stack
                                MemberModeCount[label]++;			// This region number plus one
                                // Sum all color in same region
                                Mode[label * 3 + 0] += P.l;
                                Mode[label * 3 + 1] += P.a;
                                Mode[label * 3 + 2] += P.b;
                            }
                        }
                    }
                }
                MemberModeCount[label]++;							// Count the point itself
                Mode[label * 3 + 0] /= MemberModeCount[label];		// Get average color
                Mode[label * 3 + 1] /= MemberModeCount[label];
                Mode[label * 3 + 2] /= MemberModeCount[label];
            }
        }
    }

    // Get result image from Mode array
    for(int i = 0; i < ROWS; i++) {
        for(int j = 0; j < COLS; j++) {
            label = Labels[i][j];
            float l = Mode[label * 3 + 0];
            float a = Mode[label * 3 + 1];
            float b = Mode[label * 3 + 2];
            Point5D Pixel;
            Pixel.MSPOint5DSet(i, j, l, a, b);
            Img.at<cv::Vec3f>(i, j) = cv::Vec3f(Pixel.l, Pixel.a, Pixel.b);
        }
    }

// Clean Memory
    delete[] Mode;
    delete[] MemberModeCount;

    for(int i = 0; i < ROWS; i++)
        delete[] Labels[i];
    delete[] Labels;
}
