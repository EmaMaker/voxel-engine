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
    ChunkMesh(Chunk::Chunk *c);
    ~ChunkMesh();
    
    void mesh();
    void draw();

    Chunk::Chunk *chunk{nullptr};
    // static Shader theShader("shaders/shader.vs", "shaders/shader.fs");
    glm::mat4 model = glm::mat4(1.0f);

private:
    void quad(glm::vec3 bottomLeft, glm::vec3 topLeft, glm::vec3 topRight, glm::vec3 bottomRight, Block block, bool backFace);

    std::vector<GLfloat> vertices;
    std::vector<GLfloat> colors;
    std::vector<GLuint> indices;


    GLuint VAO{0}, VBO{0}, EBO{0}, colorBuffer{0}, vIndex{0};
    Shader *theShader{nullptr};
};

#endif