#include "utils.hpp"

bool utils::withinDistance(int startx, int starty, int startz, int x, int y, int z, int dist)
{
    return (x-startx)*(x-startx) + (y - starty)*(y-starty) + (z-startz)*(z-startz) <= dist*dist;
}

//flatten 3d coords to 1d array cords
// https://stackoverflow.com/questions/20266201/3d-array-1d-flat-indexing
int utils::coord3DTo1D (int x, int y, int z, int maxX, int maxY, int maxZ) { return x + maxX * (y + z * maxY); }

std::array<int, 3> utils::coord1DTo3D(int idx, int maxX, int maxY, int mazZ)
{
        int z = idx / (maxX * maxY);
        idx -= (z * maxX* maxY);
        int y = idx / maxX;
        int x = idx % maxX;
        return std::array<int, 3> {x, y, z};
}
