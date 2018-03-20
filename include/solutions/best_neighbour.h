#pragma once

#include "basic/image.h"
#include <vector>


Seam getSeamBestNeighbour(Image& a, Image& b, std::vector<std::vector<double>>& dist);

std::vector<std::vector<double>> initialBestNeighbourDist(int height, int width);