#include "energy/pan_pixel_energy.h"

//#define EDGE_DETECTION

static const int frameSize = 5;
static const int delta = 1600;

inline int getI(Image &a, int x, int y) {
    Pixel* p = a.getPixel(x, y);
    return (p[0] + (int)p[1] + p[2]);
}

inline double grayscale(Image &a, int x, int y) {
    Pixel* p = a.getPixel(x, y);
    return p[0] * 0.114 + p[1] * 0.587 + p[2] * 0.299;
}

inline double difference(Image &a, Image &b, int x, int y) {
    //return fabs(grayscale(a, x, y) - grayscale(b, x, y)) * 3.0; // 3 for normalization to [0,765]
    return abs(getI(a, x, y) - getI(b, x, y));
}

PanPixelEnergy::PanPixelEnergy(Image &a, Image &b, bool segmentation) : a(a), b(b), segmented(segmentation) {
    int intersectionTop = top = std::max(a.top, b.top);
    int bottom = std::min(a.top + a.height, b.top + b.height); // bottom not included
    int height = bottom - intersectionTop;
    int intersectionLeft = left = std::max(a.left, b.left);
    int right = std::min(a.left + a.width, b.left + b.width); // right not included
    width = right - intersectionLeft;

    std::vector<char> isPR;
    std::vector<int> labels;

    if (segmentation) {
        cv::Mat im = cv::Mat::zeros(cv::Size(width, height), CV_8UC3);

        int sigmaS = 6;
        int sigmaR = 4;
        int minArea = 1000;
        SpeedUpLevel speedupLevel = AUTO_SPEEDUP;

        #pragma omp parallel for
        for (int y = intersectionTop; y < bottom; y++)
            for (int x = intersectionLeft; x < right; x++)
                if (a.inside(x, y) && b.inside(x, y)) {
                    Pixel *to = im.data +
                                (im.type() == CV_8UC4 ? 4 : 3) * ((y - intersectionTop) * width + x - intersectionLeft);
                    Pixel *from = a.getPixel(x, y);
                    if (im.type() == CV_8UC4)
                        to[3] = 255;

                    for (int i = 0; i < 3; i++)
                        to[i] = from[i];
                }

        SegmentedRegions regions = meanShiftSegmentation(im.data, width, height, 3,
                                                         sigmaS, sigmaR, minArea, speedupLevel, false);

        std::vector<int> minX, maxX, minY, maxY;

        int num = regions.getNumRegions();

        minX.resize(num, 1e9);
        maxX.resize(num, -1e9);
        minY.resize(num, 1e9);
        maxY.resize(num, -1e9);

        for (size_t i = 0; i < num; i++) {
            std::vector<PixelPosition> border = regions.getRegionBorder(i);
            for (auto &x : border) {
                for (int j = 0; j < 3; j++)
                    (im.data + 3 * (x.row * width + x.column))[j] = 255;
                minX[i] = std::min(minX[i], x.column);
                maxX[i] = std::max(maxX[i], x.column);
                minY[i] = std::min(minY[i], x.row);
                maxY[i] = std::max(maxY[i], x.row);
            }
        }

        isPR.resize(num, false);
        labels = std::move(regions.getPixelLabels());

        for (int i = 0; i < num; i++)
            if (maxX[i] - minX[i] > delta || maxY[i] - minY[i] > delta)
                isPR[i] = true;


        #ifdef VISUALIZATION

        #pragma omp parallel for
        for (int y = 0; y < height; y++)
            for (int x = 0; x < width; x++)
                if (a.inside(x + intersectionLeft, y + intersectionTop) && isPR[labels[y * width + x]])
                    for (int j = 0; j < 3; j++)
                        (im.data + 3 * (y * width + x))[j] = magic[j];

        cv::imwrite(std::to_string(rand()) + "pan_dijkstra_segmentation_result.jpg", im);*/

        #endif
    }

    #ifdef EDGE_DETECTION



    int gx[3][3] = {{-1, 0, 1}, {-2, 0, 2}, {-1, 0, 1}};
    int gy[3][3] = {{1, 2, 1}, {0, 0, 0}, {-1, -2, -1}};

    std::vector<std::vector<int>> gradient;
    gradient.resize(height, std::vector<int>());
    #pragma omp parallel for
    for (int i = 0; i < gradient.size(); i++)
        gradient[i].resize(width, 0);

    for (int y = top; y < bottom; y++)
        for (int x = left; x < right; x++)
            if (a.inside(x, y) && b.inside(x, y)) {
                int gradxa = 0;
                int gradya = 0;
                int gradxb = 0;
                int gradyb = 0;
                bool bad = false;
                for (int i = 0; i < 3; i++)
                    for (int j = 0; j < 3; j++)
                        if (a.inside(x + j - 1, y + i - 1) && b.inside(x + j - 1, y + i - 1)) {
                            gradxa += gx[i][j] * getI(a, x + j - 1, y + i - 1);
                            gradya += gy[i][j] * getI(a, x + j - 1, y + i - 1);
                            gradxb += gx[i][j] * getI(b, x + j - 1, y + i - 1);
                            gradyb += gy[i][j] * getI(b, x + j - 1, y + i - 1);
                        } else {
                            bad = true;
                        }
                if (!bad)
                    gradient[y  - intersectionTop][x - intersectionLeft] = abs(gradxa) + abs(gradxb) + abs(gradya) + abs(gradyb);
            }

    cv::Mat im = cv::Mat::zeros(cv::Size(width, height), CV_8UC3);

    const int level = 600;

    for (int y = top; y < bottom; y++)
        for (int x = left; x < right; x++)
            if (a.inside(x, y) && b.inside(x, y)) {
                if (gradient[y - intersectionTop][x - intersectionLeft] >= level) {
                    Pixel *to = im.data +
                                (im.type() == CV_8UC4 ? 4 : 3) * ((y - intersectionTop) * width + x - intersectionLeft);
                    for (int i = 0; i < 3; i++)
                        to[i] = 255;
                }
            }

    cv::imwrite("edges_result.jpg", im);

    #endif // EDGE_DETECTION

    energy.resize(height, std::vector<double>());
    #pragma omp parallel for
    for (int i = 0; i < energy.size(); i++)
        energy[i].resize(width, 0);

    #pragma omp parallel for
    for (int y = top; y < bottom; y++)
        for (int x = left; x < right; x++)
            if (a.inside(x, y) && b.inside(x, y)) {
                #ifdef EDGE_DETECTION
                bool edge = false;
                #endif
                for (int dy = -frameSize; dy <= frameSize; dy++)
                    for (int dx = -frameSize; dx <= frameSize; dx++) {
                        int xx = x + dx;
                        int yy = y + dy;

                        if (a.inside(xx, yy) && b.inside(xx, yy)) {
                            double cost = difference(a, b, xx, yy);
                            if (segmentation)
                                cost *= (isPR[labels[(yy - top) * width + xx - left]] ? 0.4 : 1);

                            #ifdef EDGE_DETECTION
                            if (gradient[yy - intersectionTop][xx - intersectionLeft] >= level)
                                edge = true;
                            #endif

                            energy[y - top][x - left] = std::max(energy[y - top][x - left], cost);
                        }
                    }
                #ifdef EDGE_DETECTION
                if (edge)
                    energy[y - top][x - left] *= 0.7;
                #endif
            }

};

static inline int mhtDist(Pixel *a, Pixel *b) {
    return abs(a[0] - b[0]) + abs(a[1] - b[1]) + abs(a[2] - b[2]);
}

double PanPixelEnergy::calcEnergy(int x, int y) {
    return energy[y - top][x - left];
}