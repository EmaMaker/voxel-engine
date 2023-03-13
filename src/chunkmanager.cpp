#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "chunk.hpp"
#include "chunkgenerator.hpp"
#include "chunkmanager.hpp"
#include "chunkmesher.hpp"
#include "globals.hpp"

#include <atomic>
#include <iostream>
#include <mutex>
#include <set>
#include <string>
#include <unordered_map>
#include <thread>

std::unordered_map<std::uint32_t, Chunk::Chunk *> chunks;

namespace chunkmanager
{
    std::mutex mutex_queue_generate;
    std::set<Chunk::Chunk *> to_generate;
    std::set<Chunk::Chunk *> to_generate_shared;

    std::mutex mutex_queue_mesh;
    std::set<Chunk::Chunk *> to_mesh;
    std::set<Chunk::Chunk *> to_mesh_shared;

    std::atomic_bool mesh_should_run;
    std::atomic_bool generate_should_run;

    void mesh()
    {
	while (mesh_should_run)
            if (mutex_queue_mesh.try_lock())
            {
                for (const auto &c : to_mesh)
                {
                    if (c->mutex_state.try_lock())
                    {
                        chunkmesher::mesh(c);
                        c->setState(Chunk::CHUNK_STATE_MESHED, true);
                        c->mutex_state.unlock();
                    }
                }
                to_mesh.clear();
                mutex_queue_mesh.unlock();
            }
    }

    void generate()
    {
        while (generate_should_run)
            if (mutex_queue_generate.try_lock())
            {
                for (const auto &c : to_generate)
                {
                    if (c->mutex_state.try_lock())
                    {
                        generateChunk(c);
                        c->setState(Chunk::CHUNK_STATE_GENERATED, true);
                        c->mutex_state.unlock();
                    }
                }
                to_generate.clear();
                mutex_queue_generate.unlock();
            }
    }

    std::thread initMeshThread()
    {
	mesh_should_run = true;
        std::thread mesh_thread(mesh);
        return mesh_thread;
    }
    std::thread initGenThread()
    {
	generate_should_run = true;
        std::thread gen_thread(generate);
        return gen_thread;
    }

    void stopGenThread(){
	generate_should_run = false;
    }

    void stopMeshThread(){
	mesh_should_run = false;
    }

    int total{0}, toGpu{0};
    int rr{RENDER_DISTANCE * RENDER_DISTANCE};
    uint8_t f = 0;
    glm::vec4 frustumPlanes[6];
    std::unordered_map<uint32_t, std::time_t> to_delete;
    std::set<uint32_t> to_delete_delete;

    void update(float deltaTime)
    {
	int nUnloaded{0};

        f = 0;
        f |= mutex_queue_generate.try_lock();
        f |= mutex_queue_mesh.try_lock() << 1;

        // Iterate over all chunks, in concentric spheres starting fron the player and going outwards
        // Eq. of the sphere (x - a)² + (y - b)² + (z - c)² = r²
        glm::vec3 cameraPos = theCamera.getPos();
	theCamera.getFrustumPlanes(frustumPlanes, true);
        int chunkX{(static_cast<int>(cameraPos.x)) / CHUNK_SIZE}, chunkY{(static_cast<int>(cameraPos.y)) / CHUNK_SIZE}, chunkZ{(static_cast<int>(cameraPos.z)) / CHUNK_SIZE};

	std::time_t currentTime = std::time(nullptr);
	// Check for far chunks that need to be cleaned up from memory
	for(const auto& n : chunks){
		Chunk::Chunk* c = n.second;
		int x{(int)(c->getPosition().x)};
		int y{(int)(c->getPosition().y)};
		int z{(int)(c->getPosition().z)};
		if( (chunkX-x)*(chunkX-x) + (chunkY-y)*(chunkY-y) + (chunkZ-z)*(chunkZ-z) >=
			(int)(RENDER_DISTANCE*1.5)*(int)(RENDER_DISTANCE*1.5))
		    if(to_delete.find(n.first) == to_delete.end())
			to_delete.insert(std::make_pair(n.first, currentTime));
	}
	for(const auto& n :to_delete){
	    if(  currentTime>=n.second + UNLOAD_TIMEOUT) {
		    delete chunks.at(n.first);
		    chunks.erase(n.first);
		    nUnloaded++;
		    to_delete_delete.insert(n.first);
	    }
	}
	for(uint32_t i : to_delete_delete) to_delete.erase(i);
	to_delete_delete.clear();
	if(nUnloaded) std::cout << "Unloaded " << nUnloaded << " chunks\n";


        // Possible change: coordinates everything at the origin, then translate later?
        // Step 1. Eq. of a circle. Fix the x coordinate, get the 2 possible y's
        int xp{0}, x{0};
        bool b = true;
        while (xp <= RENDER_DISTANCE)
        {
            if (b)
            {
                x = chunkX + xp;
            }
            else
            {
                x = chunkX - xp;
            }
            // for (int x = chunkX - RENDER_DISTANCE; x < chunkX + RENDER_DISTANCE; x++)
            // {

            // Possible optimization: use sqrt lookup
            int y1 = sqrt((rr) - (x - chunkX) * (x - chunkX)) + chunkY;
            int y2 = -sqrt((rr) - (x - chunkX) * (x - chunkX)) + chunkY;

            for (int y = y2 + 1; y <= y1; y++)
            {
                // Step 2. At both y's, get the corresponding z values
                int z1 = sqrt((rr) - (x - chunkX) * (x - chunkX) - (y - chunkY) * (y - chunkY)) + chunkZ;
                int z2 = -sqrt((rr) - (x - chunkX) * (x - chunkX) - (y - chunkY) * (y - chunkY)) + chunkZ;

                // std::cout << "RENDER DISTANCE " << RENDER_DISTANCE << " Current radius: " << r << " X: " << x << " Y Limits: " << y1 << " - " << y2 << "(" << y << ") Z Limits: " << z1 << " - " << z2 << std::endl;

                for (int z = z2; z <= z1; z++)
                {
                    uint16_t i{}, j{}, k{};

                    if (x < 0)
                        i = 0;
                    else if (x >= 1024)
                        i = 1023;
                    else
                        i = x;

                    if (y < 0)
                        j = 0;
                    else if (y >= 1024)
                        j = 1023;
                    else
                        j = y;

                    if (z < 0)
                        k = 0;
                    else if (z >= 1024)
                        k = 1023;
                    else
                        k = z;

                    uint32_t in = i | (j << 10) | (k << 20); // uint32_t is fine, since i'm limiting the coordinate to only use up to ten bits (1024). There's actually two spare bits
                    chunkmanager::updateChunk(in, i, j, k);
                }
            }

            if (!b)
            {
                xp++;
                b = true;
            }
            else
            {
                b = false;
            }
        }
	//std::cout << "Chunks to mesh: " << to_mesh.size() << "\n";
	//std::cout << "Chunks to generate: " << to_generate.size() << "\n";
        //std::cout << "Total chunks to draw: " << total << ". Sent to GPU: " << toGpu << "\n";
        //total = 0;
        //toGpu = 0;

        if ((f & 1))
            mutex_queue_generate.unlock();
        if ((f & 2))
            mutex_queue_mesh.unlock();
    }

    // Generation and meshing happen in two separate threads from the main one
    // Chunk states are used to decide which actions need to be done on the chunk and queues+mutexes to pass the chunks between the threads
    // Uploading data to GPU still needs to be done in the main thread, or another OpenGL context needs to be opened, which further complicates stuff
    void updateChunk(uint32_t index, uint16_t i, uint16_t j, uint16_t k)
    {
        if (chunks.find(index) == chunks.end())
        {
            Chunk::Chunk *c = new Chunk::Chunk(glm::vec3(i, j, k));
            chunks.insert(std::make_pair(index, c));
        }
        else
        {
            Chunk::Chunk *c = chunks.at(index);

            if (!(c->mutex_state.try_lock()))
                return;

            if (!c->getState(Chunk::CHUNK_STATE_GENERATED))
            {
                if (f & 1)
                    to_generate.insert(c);
            }
            else
            {
                if (!c->getState(Chunk::CHUNK_STATE_MESHED))
                {
                    if (f & 2)
                        to_mesh.insert(c);
                }
                else
                {
                    if (!c->getState(Chunk::CHUNK_STATE_MESH_LOADED))
                    {
                        if (c->vIndex > 0)
                        {

                            // bind the Vertex Array Object first, then bind and set vertex buffer(s), and then configure vertex attributes(s).
                            glBindVertexArray(c->VAO);

                            glBindBuffer(GL_ARRAY_BUFFER, c->VBO);
                            glBufferData(GL_ARRAY_BUFFER, c->vertices.size() * sizeof(GLfloat), &(c->vertices[0]), GL_STATIC_DRAW);

                            glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, c->EBO);
                            glBufferData(GL_ELEMENT_ARRAY_BUFFER, c->indices.size() * sizeof(GLuint), &(c->indices[0]), GL_STATIC_DRAW);

                            glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void *)0);
                            glEnableVertexAttribArray(0);

                            glBindBuffer(GL_ARRAY_BUFFER, c->colorBuffer);
                            glBufferData(GL_ARRAY_BUFFER, c->colors.size() * sizeof(GLfloat), &(c->colors[0]), GL_STATIC_DRAW);

                            glEnableVertexAttribArray(1);
                            glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void *)0);

                            // glDisableVertexAttribArray(0);
                            // glDisableVertexAttribArray(1);
                            glBindVertexArray(0);

                            c->vIndex = (GLuint)(c->indices.size());

                            c->vertices.clear();
                            c->indices.clear();
                            c->colors.clear();
                        }
                        c->setState(Chunk::CHUNK_STATE_MESH_LOADED, true);
                    }

		    // Frustum Culling of chunk
                    total++;

                    glm::vec3 chunk = c->getPosition();
		    glm::vec4 chunkW = glm::vec4(chunk.x*static_cast<float>(CHUNK_SIZE), chunk.y*static_cast<float>(CHUNK_SIZE), chunk.z*static_cast<float>(CHUNK_SIZE),1.0);
                    glm::mat4 model = glm::translate(glm::mat4(1.0), ((float)CHUNK_SIZE) * chunk);

		    bool out=false;
		    // First test, check if all the corners of the chunk are outside any of the
		    // planes
		    for(int p = 0; p < 6; p++){

			int a{0};
			for(int i = 0; i < 8; i++) {
			    a+=glm::dot(frustumPlanes[p], glm::vec4(chunkW.x + ((float)(i & 1))*CHUNK_SIZE, chunkW.y
					+ ((float)((i
						& 2) >> 1))*CHUNK_SIZE, chunkW.z + ((float)((i & 4) >>
						2))*CHUNK_SIZE, 1.0)) < 0.0;
			    }

			if(a==8){
			    out=true;
			    break;
			}
		    }

                    if (!out)
                    {
                        toGpu++;
                        chunkmesher::draw(c, model);
                    }
                }
            }
            c->mutex_state.unlock();
        }
    }

    void destroy()
    {
        for (auto &n : chunks)
            delete n.second;
    }
};
