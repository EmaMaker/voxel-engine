#ifndef CHUNK_H
#define CHUNK_H

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <array>
#include <bitset>

#include "block.hpp"
#include "spacefilling.hpp"
#include "intervalmap.hpp"

#define CHUNK_SIZE 16
#define CHUNK_VOLUME (CHUNK_SIZE * CHUNK_SIZE * CHUNK_SIZE)
#define CHUNK_MAX_INDEX (CHUNK_VOLUME - 1)

namespace Chunk
{

    constexpr uint8_t CHUNK_STATE_GENERATED = 1;
    constexpr uint8_t CHUNK_STATE_MESHED = 2;
    constexpr uint8_t CHUNK_STATE_LOADED = 3;
    constexpr uint8_t CHUNK_STATE_EMPTY = 4;

    int coord3DTo1D(int x, int y, int z);

    class Chunk
    {

    public:
        Chunk(glm::vec3 pos = glm::vec3(0.0f)); // a default value for the argument satisfies the need for a default constructor when using the type in an unordered_map (i.e. in chunkmanager)
        ~Chunk();

    public:
        glm::vec3 getPosition() { return this->position; }
        std::bitset<8> getTotalState() { return this->state; }
        bool getState(uint8_t n) { return this->state.test(n); }
        void setState(uint8_t nstate, bool value);

        void setBlock(Block b, int x, int y, int z);
        void setBlocks(int start, int end, Block b);
        Block getBlock(int x, int y, int z);
        IntervalMap<Block>& getBlocks() { return (this->blocks); }
        Block* getBlocksArray(int* len) { return (this->blocks.toArray(len)); }

    private:
        glm::vec3 position{};
        IntervalMap<Block> blocks{};

        std::bitset<8> state{0};
    };
};

#endif