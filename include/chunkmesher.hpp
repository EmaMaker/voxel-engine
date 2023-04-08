#ifndef CHUNKMESH_H
#define CHUNKMESH_H

#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

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
	    glm::vec3 bottomRight, glm::vec3 normal, Block block, bool backFace);
}


#endif
