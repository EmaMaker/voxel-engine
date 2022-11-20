#include <iostream>

#include "chunk.hpp"
#include "block.hpp"
#include "utils.hpp"
#include "intervalmap.hpp"
#include "globals.hpp"

namespace Chunk
{

    int coord3DTo1D(int x, int y, int z)
    {
        return utils::coord3DTo1D(x, y, z, CHUNK_SIZE, CHUNK_SIZE, CHUNK_SIZE);
    }

    Chunk::Chunk(glm::vec3 pos)
    {
        this->position = pos;
        this->blocks.insert(0, CHUNK_VOLUME, Block::AIR);
        // std::cout << "CHUNK" << std::endl;
    }

    Chunk ::~Chunk()
    {
    }

    Block Chunk::getBlock(int x, int y, int z)
    {
        return blocks.at(coord3DTo1D(x, y, z));
        // return blocks.at(HILBERT_XYZ_ENCODE[x][y][z]);
    }

    void Chunk::setBlock(Block b, int x, int y, int z)
    {
        // return blocks.insert(HILBERT_XYZ_ENCODE[x][y][z], HILBERT_XYZ_ENCODE[x][y][z]+1, b);
        int coord = coord3DTo1D(x, y, z);
        blocks.insert(coord <= 0 ? 0 : coord, coord+1 >= CHUNK_VOLUME ? CHUNK_VOLUME : coord+1, b);
    }

    void Chunk::setState(uint8_t nstate, bool value)
    {
        if (value)
            this->state.set((size_t)nstate);
        else
            this->state.reset((size_t)nstate);
    }
}
