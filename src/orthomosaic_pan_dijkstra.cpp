#include "solutions/pan_dijkstra.h"
#include "runner/runner.h"

int main(int argnum, char** args) {
    Runner::run(argnum, args, PanDijkstra(), "pan_dijkstra_");
    return 0;
}
