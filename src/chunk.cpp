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

    int32_t calculateIndex(glm::vec3 pos){
	return calculateIndex(static_cast<int16_t>(pos.x), static_cast<int16_t>(pos.y),
		static_cast<int16_t>(pos.z));
    }
    int32_t calculateIndex(int16_t x, int16_t y, int16_t z){
	return x | (y << 10) | (z << 20); 
    }

    Chunk::Chunk(glm::vec3 pos)
    {
        this->position = pos;
        this->setState(CHUNK_STATE_EMPTY, true);
	this->setBlocks(0, CHUNK_MAX_INDEX, Block::AIR);
	this->index = calculateIndex(pos);
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

    void Chunk::setState(uint16_t nstate, bool value)
    {
        if (value)
	    this->state.fetch_or(nstate);
        else
	    this->state.fetch_and(~nstate);
    }
}
