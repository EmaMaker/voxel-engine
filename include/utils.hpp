#ifndef UTILS_H
#define UTILS_H

#include <array>

namespace utils
{
    bool withinDistance(int startx, int starty, int startz, int x, int y, int z, int dist);
    // https://stackoverflow.com/questions/20266201/3d-array-1d-flat-indexing
    // flatten 3d coords to 1d array cords
    int coord3DTo1D(int x, int y, int z, int maxX, int maxY, int maxZ);
    std::array<int, 3> coord1DTo3D(int idx, int maxX, int maxY, int mazZ);

}
#endif