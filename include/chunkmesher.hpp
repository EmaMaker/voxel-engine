#ifndef CHUNKMESH_H
#define CHUNKMESH_H

#include <string>
#include <vector>

#include <glad/glad.h>
#include <glm/glm.hpp>
#include <GLFW/glfw3.h>
#include <oneapi/tbb/concurrent_queue.h>

#include "chunk.hpp"
#include "chunkmeshdata.hpp"
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

    ChunkMeshDataQueue& getMeshDataQueue();
    void init();
    void mesh(Chunk::Chunk* chunk);
}


#endif

