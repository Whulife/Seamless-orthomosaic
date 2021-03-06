#pragma once

#include "solutions/seam_solver.h"

class MinCut final : public SeamSolver {
    Seam getSeam(Image& a, Image& b) final;
};
