#ifndef RENDERER_H
#define RENDERER_H

#include "shader.hpp"

namespace renderer{
    void init();
    void render();
    void destroy();
    Shader* getRenderShader();

};

#endif
