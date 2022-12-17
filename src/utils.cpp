#include "utils.hpp"

bool utils::withinDistance(int startx, int starty, int startz, int x, int y, int z, int dist){
    return (x-startx)*(x-startx) + (y - starty)*(y-starty) + (z-startz)*(z-startz) <= dist*dist;
}

    // https://stackoverflow.com/questions/20266201/3d-array-1d-flat-indexing
    //flatten 3d coords to 1d array cords
int utils::coord3DTo1D (int x, int y, int z, int maxX, int maxY, int maxZ){
        return y + maxY * (x + z * maxX);
    }

void utils::coord1DTo3D(int idx, int maxX, int maxY, int mazZ, int* x, int* y, int* z){
        *z = idx / (maxX * maxY);
        idx -= (*z * maxX* maxY);
        *x = idx / maxY;
        *y = idx % maxY;
    }
