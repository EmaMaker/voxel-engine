#ifndef RENDERER_H
#define RENDERER_H

#include <oneapi/tbb/concurrent_unordered_set.h>
#include <oneapi/tbb/concurrent_queue.h>

#include "chunk.hpp"
#include "chunkmesher.hpp"
#include "shader.hpp"

namespace renderer{
    typedef oneapi::tbb::concurrent_unordered_set<Chunk::Chunk*> RenderSet;

    void init(GLFWwindow* window);
    void render();
    void resize_framebuffer(int width, int height);
    void framebuffer_size_callback(GLFWwindow *window, int width, int height);
    void destroy();

    Shader* getRenderShader();
    RenderSet& getChunksToRender();
    oneapi::tbb::concurrent_queue<chunkmesher::MeshData*>& getMeshDataQueue();

};

#endif
