#ifndef RENDERER_H
#define RENDERER_H

#include <glm/glm.hpp>

#include "chunk.hpp"
#include "chunkmesher.hpp"
#include "chunkmeshdata.hpp"
#include "shader.hpp"

namespace renderer{
    typedef struct RenderInfo {
	int32_t index;
	int num_vertices;
	glm::vec3 position;
	bool buffers_allocated=false;

	GLuint VAO, VBO, extentsBuffer, texinfoBuffer;

	void allocateBuffers(){
	    // Allocate buffers
	    glGenVertexArrays(1, &VAO);
	    glGenBuffers(1, &VBO);
	    glGenBuffers(1, &extentsBuffer);
	    glGenBuffers(1, &texinfoBuffer);

	    buffers_allocated=true;
	}
    } RenderInfo;

    void init(GLFWwindow* window);
    void send_chunk_to_gpu(ChunkMeshData* mesh_data, RenderInfo* render_info);
    void render();
    void resize_framebuffer(int width, int height);
    void framebuffer_size_callback(GLFWwindow *window, int width, int height);
    void destroy();

    void saveScreenshot(bool forceFullHD=false);

    Shader* getRenderShader();
    ChunkMeshDataQueue& getMeshDataQueue();

};

#endif
