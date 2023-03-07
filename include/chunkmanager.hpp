#ifndef CHUNKMANAGER_H
#define CHUNKMANAGER_H

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
