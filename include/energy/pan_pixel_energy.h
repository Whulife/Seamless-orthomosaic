#pragma once

#include "energy/pixel_energy.h"

class PanPixelEnergy : PixelEnergy<double> {
 public:
    double calcEnergy(int x, int y) final;
    PanPixelEnergy(Image &a, Image &b);

 private:
    Image &a;
    Image &b;
};
