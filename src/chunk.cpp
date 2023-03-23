#include <iostream>

#include "chunk.hpp"
#include "block.hpp"
#include "utils.hpp"
#include "intervalmap.hpp"
#include "globals.hpp"

#include <memory>
namespace Chunk
{

    int coord3DTo1D(int x, int y, int z)
    {
        return utils::coord3DTo1D(x, y, z, CHUNK_SIZE, CHUNK_SIZE, CHUNK_SIZE);
    }

    Chunk::Chunk(glm::vec3 pos)
    {
        this->position = pos;
        this->setState(CHUNK_STATE_EMPTY, true);

	glGenVertexArrays(1, &(this->VAO));
	glGenBuffers(1, &(this->colorBuffer));
	glGenBuffers(1, &(this->VBO));
	glGenBuffers(1, &(this->EBO));

    }

    Chunk ::~Chunk()
    {
        glDeleteBuffers(1, &(this->colorBuffer));
        glDeleteBuffers(1, &(this->VBO));
        glDeleteBuffers(1, &(this->EBO));
        glDeleteVertexArrays(1, &(this->VAO));

	vertices.clear();
	indices.clear();
	colors.clear();

	mutex_state.unlock();
    }

    Block Chunk::getBlock(int x, int y, int z)
    {
        return blocks.at(HILBERT_XYZ_ENCODE[x][y][z]);
    }

    void Chunk::setBlock(Block b, int x, int y, int z)
    {
        int coord = HILBERT_XYZ_ENCODE[x][y][z];
	this->setBlocks(coord, coord+1, b);
    }
    
    void Chunk::setBlocks(int start, int end, Block b){
        if(b != Block::AIR) this->setState(CHUNK_STATE_EMPTY, false);
        this->blocks.insert(start < 0 ? 0 : start, end >= CHUNK_VOLUME ? CHUNK_VOLUME : end, b);
    }

    void Chunk::setState(uint8_t nstate, bool value)
    {
        if (value)
            this->state.set((size_t)nstate);
        else
            this->state.reset((size_t)nstate);
    }
}
