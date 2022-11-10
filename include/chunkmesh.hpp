#ifndef CHUNKMESH_H
#define CHUNKMESH_H

#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <vector>

#include "chunk.hpp"
#include "shader.hpp"

class ChunkMesh
{

public:
    ChunkMesh();
    ChunkMesh(Chunk::Chunk *c);
    void mesh();
    void draw();

    Chunk::Chunk *chunk;
    // static Shader theShader("shaders/shader.vs", "shaders/shader.fs");

private:
    void quad(glm::vec3 bottomLeft, glm::vec3 topLeft, glm::vec3 topRight, glm::vec3 bottomRight, Block block, bool backFace);

    // GLfloat vertices[(CHUNK_VOLUME)*8 * 3]{};
    // GLuint indices[(CHUNK_VOLUME)*8]{};
    // int vIndex{}, iIndex{};
    std::vector<GLfloat> vertices;
    std::vector<GLfloat> colors;
    std::vector<GLuint> indices;

    glm::mat4 model = glm::mat4(1.0f);

    GLuint VAO, VBO, EBO, colorBuffer, vIndex{0};
    Shader *theShader;
};

#endif