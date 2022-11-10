#ifndef CHUNKMANAGER_H
#define CHUNKMANAGER_H

namespace chunkmanager
{
    void init();
    void update(float deltaTime);
    void updateChunk(uint32_t, uint16_t, uint16_t, uint16_t);
}

#endif