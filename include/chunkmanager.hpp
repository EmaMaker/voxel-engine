#ifndef CHUNKMANAGER_H
#define CHUNKMANAGER_H

// Seconds to be passed outside of render distance for a chunk to be destroyed
#define UNLOAD_TIMEOUT 10

#include <thread>

#include <oneapi/tbb/concurrent_queue.h>

#include "chunk.hpp"
#include "globals.hpp"

namespace chunkmanager
{
    std::thread init();
    void blockpick(bool place);
    uint32_t calculateIndex(uint16_t i, uint16_t j, uint16_t k);
    
    void stop();
    void destroy();
    oneapi::tbb::concurrent_queue<Chunk::Chunk*>& getDeleteVector();
    std::array<std::array<int, 3>, chunks_volume>& getChunksIndices();
    Block getBlockAtPos(int x, int y, int z);
    void update();
}

#endif
