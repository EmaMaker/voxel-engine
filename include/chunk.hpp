#ifndef CHUNK_H
#define CHUNK_H

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <atomic>
#include <array>
#include <bitset>
#include <mutex>
#include <vector>

#include "block.hpp"
#include "spacefilling.hpp"
#include "intervalmap.hpp"
#include "shader.hpp"

#define CHUNK_SIZE 32
#define CHUNK_VOLUME (CHUNK_SIZE * CHUNK_SIZE * CHUNK_SIZE)
#define CHUNK_MAX_INDEX (CHUNK_VOLUME - 1)

// int32_t is fine, since i'm limiting the coordinate to only use up to ten bits (1023). There's actually two spare bits
typedef int32_t chunk_index_t;
typedef int16_t chunk_intcoord_t;
typedef uint16_t chunk_state_t;

namespace Chunk
{

    chunk_index_t calculateIndex(chunk_intcoord_t i, chunk_intcoord_t j, chunk_intcoord_t k);
    chunk_index_t calculateIndex(glm::vec3 pos);

    constexpr chunk_state_t CHUNK_STATE_GENERATED = 1;
    constexpr chunk_state_t CHUNK_STATE_MESHED = 2;
    constexpr chunk_state_t CHUNK_STATE_MESH_LOADED = 4;
    constexpr chunk_state_t CHUNK_STATE_LOADED = 8;
    constexpr chunk_state_t CHUNK_STATE_OUTOFVISION = 16;
    constexpr chunk_state_t CHUNK_STATE_UNLOADED = 32;
    constexpr chunk_state_t CHUNK_STATE_EMPTY = 64;
    constexpr chunk_state_t CHUNK_STATE_IN_GENERATION_QUEUE = 128;
    constexpr chunk_state_t CHUNK_STATE_IN_MESHING_QUEUE = 256;
    constexpr chunk_state_t CHUNK_STATE_IN_DELETING_QUEUE = 512;

    int coord3DTo1D(int x, int y, int z);

    class Chunk
    {

    public:
        Chunk(glm::vec3 pos = glm::vec3(0.0f)); // a default value for the argument satisfies the need for a default constructor when using the type in an unordered_map (i.e. in chunkmanager)
        ~Chunk();

    public:
        glm::vec3 getPosition() { return this->position; }
        void setState(chunk_state_t nstate, bool value);
        bool getState(chunk_state_t n) { return (this->state & n) == n; }
	bool isFree(){ return !(
		this->getState(CHUNK_STATE_IN_GENERATION_QUEUE) ||
		this->getState(CHUNK_STATE_IN_MESHING_QUEUE) ||
		this->getState(CHUNK_STATE_IN_DELETING_QUEUE)
		); }
        chunk_state_t getTotalState() { return this->state; }

        void setBlock(Block b, int x, int y, int z);
        void setBlocks(int start, int end, Block b);
        Block getBlock(int x, int y, int z);
        IntervalMap<Block>& getBlocks() { return (this->blocks); }
	std::unique_ptr<Block[]> getBlocksArray(int* len) { return (this->blocks.toArray(len)); }

    public:
	std::atomic<float> unload_timer{0};
	chunk_index_t getIndex(){ return this->index; }

    private:
        glm::vec3 position{};
        IntervalMap<Block> blocks{};
        
	std::atomic<chunk_state_t> state{0};
	chunk_index_t index;
    };
};

#endif
