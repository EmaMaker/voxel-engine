#ifndef CHUNKMESH_H
#define CHUNKMESH_H

#include <string>
#include <vector>

#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <oneapi/tbb/concurrent_queue.h>

#include "chunk.hpp"
#include "globals.hpp"
#include "shader.hpp"

namespace chunkmesher{
    struct MeshData{
	Chunk::Chunk* chunk; 
	GLuint numVertices{0};

	std::vector<GLfloat> vertices;
	std::vector<GLfloat> extents;
	std::vector<GLfloat> texinfo;
    };
    oneapi::tbb::concurrent_queue<MeshData*>& getMeshDataQueue();

    void mesh(Chunk::Chunk* chunk);
    void sendtogpu(MeshData* mesh_data);
    void quad(MeshData* mesh_data, glm::vec3 bottomLeft, glm::vec3 topLeft, glm::vec3 topRight,
	    glm::vec3 bottomRight, glm::vec3 normal, Block block, int dim, bool backFace);
}


#endif
