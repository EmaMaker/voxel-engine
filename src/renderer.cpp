#include "chunkmanager.hpp"
#include "chunkmesher.hpp"
#include "renderer.hpp"
#include "globals.hpp"

namespace renderer{
    Shader* theShader;

    Shader* getRenderShader() { return theShader; }

    void init(){
	theShader = new Shader{"shaders/shader.vs", "shaders/shader.fs"};
    }

    void render(){
	int total{0}, toGpu{0};
	glm::vec4 frustumPlanes[6];
	theCamera.getFrustumPlanes(frustumPlanes, true);

	glm::vec3 cameraPos = theCamera.getPos();
        int chunkX=static_cast<int>(cameraPos.x) / CHUNK_SIZE;
	int chunkY=static_cast<int>(cameraPos.y) / CHUNK_SIZE;
	int chunkZ=static_cast<int>(cameraPos.z) / CHUNK_SIZE;

	for(int i = 0; i < chunks_volume; i++) {
	    Chunk::Chunk* c = chunkmanager::getChunks().at(chunkmanager::calculateIndex(chunkmanager::getChunksIndices()[i][0] +
			chunkX, chunkmanager::getChunksIndices()[i][1] + chunkY, chunkmanager::getChunksIndices()[i][2] + chunkZ));
	    // Frustum Culling of chunk
	    total++;

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
		toGpu++;

		if(c->getState(Chunk::CHUNK_STATE_MESH_LOADED) && c->vIndex > 0)
		{
		    // glPolygonMode(GL_FRONT_AND_BACK, GL_LINE); // wireframe mode
		    theShader->use();
		    theShader->setMat4("model", model);
		    theShader->setMat4("view", theCamera.getView());
		    theShader->setMat4("projection", theCamera.getProjection());

		    glBindVertexArray(c->VAO);
		    glDrawElements(GL_TRIANGLES, c->vIndex , GL_UNSIGNED_INT, 0);
		    glBindVertexArray(0);
		}
	    }
	}

	//std::cout << "Chunks to mesh: " << to_mesh.size() << "\n";
	//std::cout << "Chunks to generate: " << to_generate.size() << "\n";
        //std::cout << "Total chunks to draw: " << total << ". Sent to GPU: " << toGpu << "\n";
    }

    void destroy(){
	delete theShader;
    }


};
