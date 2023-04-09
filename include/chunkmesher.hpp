#ifndef CHUNKMESH_H
#define CHUNKMESH_H

#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <string>
#include <vector>

#include "chunk.hpp"
#include "globals.hpp"
#include "shader.hpp"

namespace chunkmesher{
    void mesh(Chunk::Chunk* chunk);
    void sendtogpu(Chunk::Chunk* chunk);
    void draw(Chunk::Chunk* chunk, glm::mat4 model);

    void quad(Chunk::Chunk* chunk, glm::vec3 bottomLeft, glm::vec3 topLeft, glm::vec3 topRight,
	    glm::vec3 bottomRight, glm::vec3 normal, Block block, int dim, bool backFace);
}


#endif
