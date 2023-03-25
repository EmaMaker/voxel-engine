#ifndef CHUNKMANAGER_H
#define CHUNKMANAGER_H

// Second to be passed outside of render distance for a chunk to be destroyed
#define UNLOAD_TIMEOUT 10

#include <thread>

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
    void update(float deltaTime);
    void updateChunk(uint32_t, uint16_t, uint16_t, uint16_t);
}

#endif
