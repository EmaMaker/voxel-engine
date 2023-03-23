#ifndef UTILS_H
#define UTILS_H

#include <array>

namespace utils
{
    bool withinDistance(int startx, int starty, int startz, int x, int y, int z, int dist);
    int coord3DTo1D(int x, int y, int z, int maxX, int maxY, int maxZ);
    std::array<int, 3> coord1DTo3D(int idx, int maxX, int maxY, int mazZ);

}
#endif
