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

    chunk_index_t calculateIndex(glm::vec3 pos){
	return calculateIndex(static_cast<chunk_intcoord_t>(pos.x), static_cast<chunk_intcoord_t>(pos.y),
		static_cast<chunk_intcoord_t>(pos.z));
    }

    chunk_index_t calculateIndex(chunk_intcoord_t i, chunk_intcoord_t j, chunk_intcoord_t k){
	 return i | (j << 10) | (k << 20); 
    }

    Chunk::Chunk(glm::vec3 pos)
    {
        this->position = pos;
        this->setState(CHUNK_STATE_EMPTY, true);
	this->setBlocks(0, CHUNK_MAX_INDEX, Block::AIR);
	this->index = calculateIndex(pos);
    }

    Chunk ::~Chunk()
    {
    }

    void Chunk::createBuffers(){
	glGenVertexArrays(1, &(this->VAO));
	glGenBuffers(1, &(this->VBO));
	glGenBuffers(1, &(this->extentsBuffer));
	glGenBuffers(1, &(this->texinfoBuffer));

    }

    void Chunk::deleteBuffers(){
	glDeleteBuffers(1, &(this->VBO));
	glDeleteBuffers(1, &(this->extentsBuffer));
	glDeleteBuffers(1, &(this->texinfoBuffer));
        glDeleteVertexArrays(1, &(this->VAO));
    }

    Block Chunk::getBlock(int x, int y, int z)
    {
	if(x < 0 || y < 0 || z < 0 || x > CHUNK_SIZE -1 || y > CHUNK_SIZE -1 || z > CHUNK_SIZE-1 ||
		!getState(CHUNK_STATE_GENERATED)) return Block::AIR;
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

    void Chunk::setState(chunk_state_t nstate, bool value)
    {
        if (value)
	    this->state.fetch_or(nstate);
        else
	    this->state.fetch_and(~nstate);
    }
}
