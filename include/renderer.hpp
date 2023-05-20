#ifndef RENDERER_H
#define RENDERER_H

#include <oneapi/tbb/concurrent_unordered_set.h>
#include <oneapi/tbb/concurrent_queue.h>

#include "chunk.hpp"
#include "chunkmesher.hpp"
#include "shader.hpp"

namespace renderer{
    typedef oneapi::tbb::concurrent_unordered_set<Chunk::Chunk*> RenderSet;

    void init();
    void render();
    void destroy();
    Shader* getRenderShader();
    RenderSet& getChunksToRender();
    oneapi::tbb::concurrent_queue<chunkmesher::MeshData*>& getMeshDataQueue();

};

#endif
