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

namespace Chunk
{

    constexpr uint16_t CHUNK_STATE_GENERATED = 1;
    constexpr uint16_t CHUNK_STATE_MESHED = 2;
    constexpr uint16_t CHUNK_STATE_MESH_LOADED = 4;
    constexpr uint16_t CHUNK_STATE_LOADED = 8;
    constexpr uint16_t CHUNK_STATE_OUTOFVISION = 16;
    constexpr uint16_t CHUNK_STATE_UNLOADED = 32;
    constexpr uint16_t CHUNK_STATE_EMPTY = 64;
    constexpr uint16_t CHUNK_STATE_IN_GENERATION_QUEUE = 128;
    constexpr uint16_t CHUNK_STATE_IN_MESHING_QUEUE = 256;
    constexpr uint16_t CHUNK_STATE_IN_RENDERING_QUEUE = 512;

    int coord3DTo1D(int x, int y, int z);

    class Chunk
    {

    public:
        Chunk(glm::vec3 pos = glm::vec3(0.0f)); // a default value for the argument satisfies the need for a default constructor when using the type in an unordered_map (i.e. in chunkmanager)

    public:
	void createBuffers();
	void deleteBuffers();

        glm::vec3 getPosition() { return this->position; }
        void setState(uint16_t nstate, bool value);
	uint16_t getTotalState() { return this->state; }
        bool getState(uint16_t n) { return (this->state & n) == n; }
	bool isFree(){ return !(getState(CHUNK_STATE_IN_GENERATION_QUEUE) ||
		getState(CHUNK_STATE_IN_MESHING_QUEUE)); }

        void setBlock(Block b, int x, int y, int z);
        void setBlocks(int start, int end, Block b);
        Block getBlock(int x, int y, int z);
        IntervalMap<Block>& getBlocks() { return (this->blocks); }
	std::unique_ptr<Block[]> getBlocksArray(int* len) { return (this->blocks.toArray(len)); }

    public:
        GLuint VAO{0}, VBO{0}, extentsBuffer{0}, texinfoBuffer{0}, numVertices{0};
	std::atomic<float> unload_timer{0};

    private:
        glm::vec3 position{};
        IntervalMap<Block> blocks{};
        
	std::atomic_uint16_t state{0};
    };
};

#endif
