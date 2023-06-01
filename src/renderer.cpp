#include "renderer.hpp"

#include <oneapi/tbb/concurrent_vector.h>
#include <oneapi/tbb/concurrent_queue.h>

#include "chunkmanager.hpp"
#include "chunkmesher.hpp"
#include "globals.hpp"
#include "stb_image.h"

namespace renderer{
    RenderSet chunks_torender;
    oneapi::tbb::concurrent_vector<Chunk::Chunk*> render_todelete;
    oneapi::tbb::concurrent_queue<chunkmesher::MeshData*> MeshDataQueue;

    Shader* theShader;
    GLuint chunkTexture;

    Shader* getRenderShader() { return theShader; }
    RenderSet& getChunksToRender(){ return chunks_torender; }
    oneapi::tbb::concurrent_queue<chunkmesher::MeshData*>& getMeshDataQueue(){ return MeshDataQueue; }


    void init(){
	// Create Shader
	theShader = new Shader{"shaders/shader-texture.vs", "shaders/shader-texture.fs"};

	// Create 3d array texture
	constexpr int layerCount = 3;
	glGenTextures(1, &chunkTexture);
	glBindTexture(GL_TEXTURE_2D_ARRAY, chunkTexture);
	
	int width, height, nrChannels;
	unsigned char *texels = stbi_load("textures/cobblestone.png", &width, &height, &nrChannels, 0);
	glTexImage3D(GL_TEXTURE_2D_ARRAY, 0, GL_RGBA, width, height, layerCount, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
	glTexSubImage3D(GL_TEXTURE_2D_ARRAY, 0, 0, 0, 0, width, height, 1, GL_RGBA, GL_UNSIGNED_BYTE, texels);
	unsigned char *texels1 = stbi_load("textures/dirt.png", &width, &height, &nrChannels, 0);
	glTexSubImage3D(GL_TEXTURE_2D_ARRAY, 0, 0, 0, 1, width, height, 1, GL_RGBA, GL_UNSIGNED_BYTE, texels1);
	unsigned char *texels2 = stbi_load("textures/grass_top.png", &width, &height, &nrChannels, 0);
	glTexSubImage3D(GL_TEXTURE_2D_ARRAY, 0, 0, 0, 2, width, height, 1, GL_RGB, GL_UNSIGNED_BYTE, texels2);

	glTexParameteri(GL_TEXTURE_2D_ARRAY,GL_TEXTURE_MIN_FILTER,GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D_ARRAY,GL_TEXTURE_MAG_FILTER,GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D_ARRAY,GL_TEXTURE_WRAP_S,GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D_ARRAY,GL_TEXTURE_WRAP_T,GL_REPEAT);
    }

    void render(){
	int total{0}, toGpu{0};
	glm::vec4 frustumPlanes[6];
	theCamera.getFrustumPlanes(frustumPlanes, true);
	glm::vec3 cameraPos = theCamera.getPos();	
	glm::vec3 cameraChunkPos = cameraPos / static_cast<float>(CHUNK_SIZE);

	theShader->use();
	theShader->setVec3("viewPos", cameraPos);

	chunkmesher::MeshData* m;
	while(MeshDataQueue.try_pop(m)){
	    chunkmesher::sendtogpu(m);
	    chunkmesher::getMeshDataQueue().push(m);
	}

	for(auto& c : chunks_torender){
	    float dist = glm::distance(c->getPosition(), cameraChunkPos);
	    if(dist <= static_cast<float>(RENDER_DISTANCE)){
		if(!c->getState(Chunk::CHUNK_STATE_MESH_LOADED)) continue;

		// reset out-of-vision and unload flags
		c->setState(Chunk::CHUNK_STATE_OUTOFVISION, false);
		c->setState(Chunk::CHUNK_STATE_UNLOADED, false);

		// Perform frustum culling and eventually render
		glm::vec3 chunk = c->getPosition();
		glm::vec4 chunkW = glm::vec4(chunk.x*static_cast<float>(CHUNK_SIZE), chunk.y*static_cast<float>(CHUNK_SIZE), chunk.z*static_cast<float>(CHUNK_SIZE),1.0);
		glm::mat4 model = glm::translate(glm::mat4(1.0), ((float)CHUNK_SIZE) * chunk);

		// Check if all the corners of the chunk are outside any of the planes
		// TODO (?) implement frustum culling as per (Inigo Quilez)[https://iquilezles.org/articles/frustumcorrect/], and check each
		// plane against each corner of the chunk 
		bool out=false;
		int a{0};
		for(int p = 0; p < 6; p++){
		    a = 0;
		    for(int i = 0; i < 8; i++)  a += glm::dot(frustumPlanes[p], glm::vec4(chunkW.x + ((float)(i & 1))*CHUNK_SIZE, chunkW.y
				    + ((float)((i & 2) >> 1))*CHUNK_SIZE, chunkW.z + ((float)((i & 4) >> 2))*CHUNK_SIZE, 1.0)) < 0.0;

		    if(a==8){
			out=true;
			break;
		    }
		}

		if (!out)
		{
		    if(c->numTriangles > 0)
		    {
			// glPolygonMode(GL_FRONT_AND_BACK, GL_LINE); // wireframe mode
			theShader->setMat4("model", model);
			theShader->setMat4("view", theCamera.getView());
			theShader->setMat4("projection", theCamera.getProjection());

			glBindVertexArray(c->VAO);
			glDrawElements(GL_TRIANGLES, c->numTriangles , GL_UNSIGNED_INT, 0);
			glBindVertexArray(0);
		    }
		}
	    }else{
		// When the chunk is outside render distance

		if(c->getState(Chunk::CHUNK_STATE_OUTOFVISION)){
		    if(glfwGetTime() - c->unload_timer > UNLOAD_TIMEOUT){
			// If chunk was already out and enough time has passed
			// Mark the chunk to be unloaded
			// And mark is to be removed from the render set
			render_todelete.push_back(c);
		    }
		} else{
		    // Mark has out of vision and annotate when it started
		    c->setState(Chunk::CHUNK_STATE_OUTOFVISION, true);
		    c->setState(Chunk::CHUNK_STATE_UNLOADED, false);
		    c->unload_timer = glfwGetTime();
		}
		
	    }
	}

	for(auto& c : render_todelete){
	    // we can get away with unsafe erase as access to the container is only done by this
	    // thread
	    c->deleteBuffers();
	    chunks_torender.unsafe_erase(c);
	    chunkmanager::getDeleteVector().push(c);
	}
	render_todelete.clear();
    }

    void destroy(){
	delete theShader;
    }


};
