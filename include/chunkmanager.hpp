#ifndef CHUNKMANAGER_H
#define CHUNKMANAGER_H

// Second to be passed outside of render distance for a chunk to be destroyed
#define UNLOAD_TIMEOUT 10

#include <unordered_map>
#include <thread>
#include "chunk.hpp"
#include "globals.hpp"

namespace chunkmanager
{
    std::thread initGenThread();
    std::thread initMeshThread();
    void stopGenThread();
    void stopMeshThread();

    void mesh();
    void generate();

    void init();
    void blockpick(bool place);
    uint32_t calculateIndex(uint16_t i, uint16_t j, uint16_t k);
    
    void destroy();
    std::unordered_map<std::uint32_t, Chunk::Chunk*>& getChunks();
    std::array<std::array<int, 3>, chunks_volume>& getChunksIndices();
    void update(float deltaTime);
    void updateChunk(uint32_t, uint16_t, uint16_t, uint16_t);
}

#endif
