#include <iostream>

#include "chunk.hpp"
#include "block.hpp"
#include "utils.hpp"

namespace Chunk
{

    int coord3DTo1D(int x, int y, int z)
    {
        return utils::coord3DTo1D(x, y, z, CHUNK_SIZE, CHUNK_SIZE, CHUNK_SIZE);
    }

    Chunk::Chunk(glm::vec3 pos)
    {
        this->position = pos;
        // std::cout << "CHUNK" << std::endl;
    }

    Chunk ::~Chunk()
    {
    }

    Block Chunk::getBlock(int x, int y, int z)
    {
        return blocks[utils::coord3DTo1D(x, y, z, CHUNK_SIZE, CHUNK_SIZE, CHUNK_SIZE)];
    }

    void Chunk::setBlock(Block b, int x, int y, int z)
    {
        blocks[utils::coord3DTo1D(x, y, z, CHUNK_SIZE, CHUNK_SIZE, CHUNK_SIZE)] = b;
    }

    void Chunk::setState(uint8_t nstate, bool value)
    {
        if (value)
            this->state.set((size_t)nstate);
        else
            this->state.reset((size_t)nstate);
    }
}
