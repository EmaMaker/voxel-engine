#ifndef RENDERER_H
#define RENDERER_H

#include <oneapi/tbb/concurrent_unordered_set.h>

#include "chunk.hpp"
#include "shader.hpp"

namespace renderer{
    void init();
    void render();
    void destroy();
    Shader* getRenderShader();
    oneapi::tbb::concurrent_unordered_set<Chunk::Chunk*>& getChunksToRender();

};

#endif
