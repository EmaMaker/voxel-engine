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

    void update(float deltaTime);

    void updateChunk(uint32_t, uint16_t, uint16_t, uint16_t);
    void destroy();

    void mesh();
    void generate();

}

#endif
