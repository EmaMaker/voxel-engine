#include "chunkmesher.hpp"

#include <array>
#include <memory>

#include "block.hpp"
#include "chunk.hpp"
#include "chunkmanager.hpp"
#include "globals.hpp"
#include "renderer.hpp"
#include "spacefilling.hpp"
#include "utils.hpp"

namespace chunkmesher{

oneapi::tbb::concurrent_queue<MeshData*> MeshDataQueue;

oneapi::tbb::concurrent_queue<MeshData*>& getMeshDataQueue(){ return MeshDataQueue; }
    
void mesh(Chunk::Chunk* chunk)
{
    MeshData* mesh_data;
    if(!MeshDataQueue.try_pop(mesh_data)) return;

    /*
     * Taking inspiration from 0fps and the jme3 porting at
     * https://github.com/roboleary/GreedyMesh/blob/master/src/mygame/Main.java
     *
     * By carefully re-reading the code and the blog post, I've come to the
     * realization that I wrote something very similar yet a lot messier (and
     * uglier) back in
     * https://github.com/EmaMaker/voxel-engine-jme3/blob/master/src/voxelengine
     * /world/Chunk.java
     *
     * Reading roboleary's impl. I've learned how to optimize having to loop
     * across different planes everytime I change dimension without having to
     * write 3 separate 3-nested-for-loops
     */

    // Cleanup previous data
    mesh_data->numVertices = 0;
    mesh_data->chunk = chunk;
    mesh_data->vertices.clear();
    mesh_data->indices.clear();
    mesh_data->colors.clear();

    // Abort if chunk is empty
    if(chunk->getState(Chunk::CHUNK_STATE_EMPTY)){
	chunk->setState(Chunk::CHUNK_STATE_MESHED, true);
	renderer::getMeshDataQueue().push(mesh_data);
	return;
    }

    // convert tree to array since it is easier to work with it
    int length{0};
    std::unique_ptr<Block[]> blocks = chunk->getBlocksArray(&length);
    if(length == 0) {
	chunk->setState(Chunk::CHUNK_STATE_MESHED, true);
	renderer::getMeshDataQueue().push(mesh_data);
	return;
    }
    
    int k, l, u, v, w, h, n, j, i;
    int x[]{0, 0, 0};
    int q[]{0, 0, 0};
    int du[]{0, 0, 0};
    int dv[]{0, 0, 0};

    std::array<Block, CHUNK_SIZE * CHUNK_SIZE> mask;
    for (bool backFace = true, b = false; b != backFace; backFace = backFace && b, b = !b)
    {
        // iterate over 3 dimensions
        for (int dim = 0; dim < 3; dim++)
        {
            // offsets of other 2 axes
            u = (dim + 1) % 3;
            v = (dim + 2) % 3;

            x[0] = 0;
            x[1] = 0;
            x[2] = 0;

            q[0] = 0;
            q[1] = 0;
            q[2] = 0;
            q[dim] = 1; // easily mark which dimension we are comparing
                        // voxels
                        // on

            for (x[dim] = -1; x[dim] < CHUNK_SIZE;)
            {
                n = 0;

                for (x[v] = 0; x[v] < CHUNK_SIZE; x[v]++)
                {
                    for (x[u] = 0; x[u] < CHUNK_SIZE; x[u]++)
                    {
			Block b1, b2;
			if(x[dim] >= 0) b1 = blocks[HILBERT_XYZ_ENCODE[x[0]][x[1]][x[2]]];
			else{
			    int cx = chunk->getPosition().x*CHUNK_SIZE;
			    int cy = chunk->getPosition().y*CHUNK_SIZE;
			    int cz = chunk->getPosition().z*CHUNK_SIZE;

			    int bx = cx+x[0];
			    int by = cy+x[1];
			    int bz = cz+x[2];

			    b1 = chunkmanager::getBlockAtPos(bx, by, bz);
			}

			if(x[dim] < CHUNK_SIZE - 1) b2 = blocks[HILBERT_XYZ_ENCODE[x[0] + q[0]][x[1]
			    + q[1]][x[2] + q[2]]];
			else{
			    int cx = chunk->getPosition().x*CHUNK_SIZE;
			    int cy = chunk->getPosition().y*CHUNK_SIZE;
			    int cz = chunk->getPosition().z*CHUNK_SIZE;

			    int bx = cx+x[0] + q[0];
			    int by = cy+x[1] + q[1];
			    int bz = cz+x[2] + q[2];

			    b2 = chunkmanager::getBlockAtPos(bx, by, bz);
			}

			// Compute the mask
			// Checking if b1==b2 is needed to generate a single quad
			// The else case provides face culling for adjacent solid faces
			// Checking for NULLBLK avoids creating empty faces if nearby chunk was not
			// yet generated
			mask[n++] = b1 == b2 ? Block::NULLBLK
                                    : backFace ? b1 == Block::NULLBLK || b1 == Block::AIR ? b2 : Block::NULLBLK
                                    : b2 == Block::NULLBLK || b2 == Block::AIR ? b1 : Block::NULLBLK;
                    }
                }

                x[dim]++;
                n = 0;
                // Actually generate the mesh from the mask. This is the same thing I used in my old crappy voxel engine
                for (j = 0; j < CHUNK_SIZE; j++)
                {
                    for (i = 0; i < CHUNK_SIZE;)
                    {

                        if (mask[n] != Block::NULLBLK)
                        {
                            // First compute the width
                            for (w = 1; i + w < CHUNK_SIZE && mask[n + w] != Block::NULLBLK && mask[n] == mask[n + w]; w++)
                            {
                            }

                            bool done = false;
                            for (h = 1; j + h < CHUNK_SIZE; h++)
                            {
                                for (k = 0; k < w; k++)
                                {
                                    if (mask[n + k + h * CHUNK_SIZE] == Block::NULLBLK || mask[n + k + h * CHUNK_SIZE] != mask[n])
                                    {
                                        done = true;
                                        break;
                                    }
                                }
                                if (done)
                                    break;
                            }

                            if (mask[n] != Block::AIR)
                            {
                                x[u] = i;
                                x[v] = j;

                                du[0] = 0;
                                du[1] = 0;
                                du[2] = 0;
                                du[u] = w;

                                dv[0] = 0;
                                dv[1] = 0;
                                dv[2] = 0;
                                dv[v] = h;

                                quad(mesh_data, glm::vec3(x[0], x[1], x[2]),
                                     glm::vec3(x[0] + du[0], x[1] + du[1], x[2] + du[2]),
                                     glm::vec3(x[0] + du[0] + dv[0], x[1] + du[1] + dv[1],
                                               x[2] + du[2] + dv[2]),
                                     glm::vec3(x[0] + dv[0], x[1] + dv[1], x[2] + dv[2]),
				     glm::vec3(backFace ? q[0] : -q[0], backFace ? q[1] : -q[1], backFace ? q[2] : -q[2] ),
                                     mask[n], dim, backFace);
                            }

                            for (l = 0; l < h; ++l)
                            {
                                for (k = 0; k < w; ++k)
                                {
                                    mask[n + k + l * CHUNK_SIZE] = Block::NULLBLK;
                                }
                            }

                            /*
                             * And then finally increment the counters and
                             * continue
                             */
                            i += w;
                            n += w;
                        }
                        else
                        {
                            i++;
                            n++;
                        }
                    }
                }
            }
        }
    }

    chunk->setState(Chunk::CHUNK_STATE_MESHED, true);
    renderer::getMeshDataQueue().push(mesh_data);
    return;
}

void sendtogpu(MeshData* mesh_data)
{
    if (mesh_data->numVertices > 0)
    {
	if(mesh_data->chunk->VAO == 0) mesh_data->chunk->createBuffers();

	// bind the Vertex Array Object first, then bind and set vertex buffer(s), and then configure vertex attributes(s).
	glBindVertexArray(mesh_data->chunk->VAO);

	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mesh_data->chunk->EBO);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, mesh_data->indices.size() * sizeof(GLuint), &(mesh_data->indices[0]), GL_STATIC_DRAW);

	// position attribute
	glBindBuffer(GL_ARRAY_BUFFER, mesh_data->chunk->VBO);
	glBufferData(GL_ARRAY_BUFFER, mesh_data->vertices.size() * sizeof(GLfloat), &(mesh_data->vertices[0]), GL_STATIC_DRAW);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void *)0);
	glEnableVertexAttribArray(0);

	// normal attribute
	glBindBuffer(GL_ARRAY_BUFFER, mesh_data->chunk->normalsBuffer);
	glBufferData(GL_ARRAY_BUFFER, mesh_data->normals.size() * sizeof(GLfloat), &(mesh_data->normals[0]), GL_STATIC_DRAW);
	glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void *)(0));
	glEnableVertexAttribArray(1);

	// texcoords attribute
	glBindBuffer(GL_ARRAY_BUFFER, mesh_data->chunk->colorBuffer);
	glBufferData(GL_ARRAY_BUFFER, mesh_data->colors.size() * sizeof(GLfloat), &(mesh_data->colors[0]), GL_STATIC_DRAW);
	glEnableVertexAttribArray(2);
	glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void *)0);

	glBindVertexArray(0);

	// save the number of indices of the mesh, it is needed later for drawing
	mesh_data->chunk->numTriangles = (GLuint)(mesh_data->indices.size());

	// once data has been sent to the GPU, it can be cleared from system RAM
	mesh_data->vertices.clear();
	mesh_data->normals.clear();
	mesh_data->indices.clear();
	mesh_data->colors.clear();
    }
    
    // mark the chunk mesh has loaded on GPU
    mesh_data->chunk->setState(Chunk::CHUNK_STATE_MESH_LOADED, true);
}

void quad(MeshData* mesh_data, glm::vec3 bottomLeft, glm::vec3 topLeft, glm::vec3 topRight,
	glm::vec3 bottomRight, glm::vec3 normal, Block block, int dim, bool backFace)
{
    mesh_data->vertices.push_back(bottomLeft.x);
    mesh_data->vertices.push_back(bottomLeft.y);
    mesh_data->vertices.push_back(bottomLeft.z);

    mesh_data->vertices.push_back(bottomRight.x);
    mesh_data->vertices.push_back(bottomRight.y);
    mesh_data->vertices.push_back(bottomRight.z);

    mesh_data->vertices.push_back(topLeft.x);
    mesh_data->vertices.push_back(topLeft.y);
    mesh_data->vertices.push_back(topLeft.z);

    mesh_data->vertices.push_back(topRight.x);
    mesh_data->vertices.push_back(topRight.y);
    mesh_data->vertices.push_back(topRight.z);

    for(int i = 0; i < 4; i++){
	mesh_data->normals.push_back(normal.x);
	mesh_data->normals.push_back(normal.y);
	mesh_data->normals.push_back(normal.z);
    }

    // texcoords
    if(dim == 0){
	mesh_data->colors.push_back(0);
	mesh_data->colors.push_back(0);
	mesh_data->colors.push_back(((int)block) - 2);

	mesh_data->colors.push_back(abs(bottomRight.z - bottomLeft.z));
	mesh_data->colors.push_back(abs(bottomRight.y - bottomLeft.y));
	mesh_data->colors.push_back(((int)block) - 2);

	mesh_data->colors.push_back(abs(topLeft.z - bottomLeft.z));
	mesh_data->colors.push_back(abs(topLeft.y - bottomLeft.y));
	mesh_data->colors.push_back(((int)block) - 2);

	mesh_data->colors.push_back(abs(topRight.z - bottomLeft.z));
	mesh_data->colors.push_back(abs(topRight.y - bottomLeft.y));
	mesh_data->colors.push_back(((int)block) - 2);
    }else if(dim == 1){
	mesh_data->colors.push_back(0);
	mesh_data->colors.push_back(0);
	mesh_data->colors.push_back(((int)block) - 2);

	mesh_data->colors.push_back(abs(bottomRight.z - bottomLeft.z));
	mesh_data->colors.push_back(abs(bottomRight.x - bottomLeft.x));
	mesh_data->colors.push_back(((int)block) - 2);

	mesh_data->colors.push_back(abs(topLeft.z - bottomLeft.z));
	mesh_data->colors.push_back(abs(topLeft.x - bottomLeft.x));
	mesh_data->colors.push_back(((int)block) - 2);

	mesh_data->colors.push_back(abs(topRight.z - bottomLeft.z));
	mesh_data->colors.push_back(abs(topRight.x - bottomLeft.x));
	mesh_data->colors.push_back(((int)block) - 2);
    }else{
	mesh_data->colors.push_back(0);
	mesh_data->colors.push_back(0);
	mesh_data->colors.push_back(((int)block) - 2);

	mesh_data->colors.push_back(abs(bottomRight.x - bottomLeft.x));
	mesh_data->colors.push_back(abs(bottomRight.y - bottomLeft.y));
	mesh_data->colors.push_back(((int)block) - 2);

	mesh_data->colors.push_back(abs(topLeft.x - bottomLeft.x));
	mesh_data->colors.push_back(abs(topLeft.y - bottomLeft.y));
	mesh_data->colors.push_back(((int)block) - 2);

	mesh_data->colors.push_back(abs(topRight.x - bottomLeft.x));
	mesh_data->colors.push_back(abs(topRight.y - bottomLeft.y));
	mesh_data->colors.push_back(((int)block) - 2);
    }

    if (backFace)
    {   
        mesh_data->indices.push_back(mesh_data->numVertices + 2);
        mesh_data->indices.push_back(mesh_data->numVertices);
        mesh_data->indices.push_back(mesh_data->numVertices + 1);
        mesh_data->indices.push_back(mesh_data->numVertices + 1);
        mesh_data->indices.push_back(mesh_data->numVertices + 3);
        mesh_data->indices.push_back(mesh_data->numVertices + 2);
    }
    else
    {
        mesh_data->indices.push_back(mesh_data->numVertices + 2);
        mesh_data->indices.push_back(mesh_data->numVertices + 3);
        mesh_data->indices.push_back(mesh_data->numVertices + 1);
        mesh_data->indices.push_back(mesh_data->numVertices + 1);
        mesh_data->indices.push_back(mesh_data->numVertices);
        mesh_data->indices.push_back(mesh_data->numVertices + 2);
    }
    mesh_data->numVertices += 4;
}
};
