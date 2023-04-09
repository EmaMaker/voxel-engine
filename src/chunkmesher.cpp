#include <array>
#include <memory>

#include "block.hpp"
#include "chunk.hpp"
#include "chunkmesher.hpp"
#include "globals.hpp"
#include "spacefilling.hpp"
#include "utils.hpp"

namespace chunkmesher{
    
void mesh(Chunk::Chunk* chunk)
{

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
    chunk->vertices.clear();
    chunk->indices.clear();
    chunk->colors.clear();
    chunk->vIndex = 0;

    // Abort if chunk is empty
    if(chunk->getState(Chunk::CHUNK_STATE_EMPTY)) return;

    // convert tree to array since it is easier to work with it
    int length{0};
    std::unique_ptr<Block[]> blocks = chunk->getBlocksArray(&length);
    if(length == 0) {
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
                        Block b1 = (x[dim] >= 0) ? blocks[HILBERT_XYZ_ENCODE[x[0]][x[1]][x[2]]] : Block::NULLBLK;
                        Block b2 = (x[dim] < CHUNK_SIZE - 1)
                                       ? blocks[HILBERT_XYZ_ENCODE[x[0] + q[0]][x[1] + q[1]][x[2] + q[2]]]
                                       : Block::NULLBLK;

                        // This is the original line taken from rob's code, readapted (replace voxelFace
                        // with b1 and b2).
                        // mask[n++] = ((voxelFace != Block::NULLBLK && voxelFace1 != Block::NULLBLK &&
                        // voxelFace.equals(voxelFace1))) ? Block::NULLBLK : backFace ? voxelFace1 : voxelFace;

                        // Additionally checking whether b1 and b2 are AIR or Block::NULLBLK allows face culling,
                        // thus not rendering faces that cannot be seen
                        // Removing the control for Block::NULLBLK disables chunk borders, which is
			// not always wanted and needs further checking
                        // This can be surely refactored in something that isn't such a big one-liner
                        mask[n++] = b1 != Block::NULLBLK && b2 != Block::NULLBLK && b1 == b2 ? Block::NULLBLK
                                    : backFace                                               ? b1 == Block::AIR || b1 == Block::NULLBLK ? b2 : Block::NULLBLK
                                    : b2 == Block::AIR || b2 == Block::NULLBLK               ? b1
                                                                                             : Block::NULLBLK;
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

                                quad(chunk, glm::vec3(x[0], x[1], x[2]),
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
}

void sendtogpu(Chunk::Chunk* chunk)
{
    if (chunk->vIndex > 0)
    {

	// bind the Vertex Array Object first, then bind and set vertex buffer(s), and then configure vertex attributes(s).
	glBindVertexArray(chunk->VAO);

	glBindBuffer(GL_ARRAY_BUFFER, chunk->VBO);
	glBufferData(GL_ARRAY_BUFFER, chunk->vertices.size() * sizeof(GLfloat), &(chunk->vertices[0]), GL_STATIC_DRAW);

	// position attribute
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void *)0);
	glEnableVertexAttribArray(0);

	// normal attribute
	glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void *)(3*
		    sizeof(float)));
	glEnableVertexAttribArray(1);

	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, chunk->EBO);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, chunk->indices.size() * sizeof(GLuint), &(chunk->indices[0]), GL_STATIC_DRAW);

	// texcoords attribute
	glBindBuffer(GL_ARRAY_BUFFER, chunk->colorBuffer);
	glBufferData(GL_ARRAY_BUFFER, chunk->colors.size() * sizeof(GLfloat), &(chunk->colors[0]), GL_STATIC_DRAW);

	glEnableVertexAttribArray(2);
	glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void *)0);

	glBindVertexArray(0);

	// save the number of indices of the mesh, it is needed later for drawing
	chunk->vIndex = (GLuint)(chunk->indices.size());

	// once data has been sent to the GPU, it can be cleared from system RAM
	chunk->vertices.clear();
	chunk->indices.clear();
	chunk->colors.clear();
    }
    
    // mark the chunk mesh has loaded on GPU
    chunk->setState(Chunk::CHUNK_STATE_MESH_LOADED, true);
}

void quad(Chunk::Chunk* chunk, glm::vec3 bottomLeft, glm::vec3 topLeft, glm::vec3 topRight,
	glm::vec3 bottomRight, glm::vec3 normal, Block block, int dim, bool backFace)
{

    chunk->vertices.push_back(bottomLeft.x);
    chunk->vertices.push_back(bottomLeft.y);
    chunk->vertices.push_back(bottomLeft.z);
    chunk->vertices.push_back(normal.x);
    chunk->vertices.push_back(normal.y);
    chunk->vertices.push_back(normal.z);

    chunk->vertices.push_back(bottomRight.x);
    chunk->vertices.push_back(bottomRight.y);
    chunk->vertices.push_back(bottomRight.z);
    chunk->vertices.push_back(normal.x);
    chunk->vertices.push_back(normal.y);
    chunk->vertices.push_back(normal.z);

    chunk->vertices.push_back(topLeft.x);
    chunk->vertices.push_back(topLeft.y);
    chunk->vertices.push_back(topLeft.z);
    chunk->vertices.push_back(normal.x);
    chunk->vertices.push_back(normal.y);
    chunk->vertices.push_back(normal.z);

    chunk->vertices.push_back(topRight.x);
    chunk->vertices.push_back(topRight.y);
    chunk->vertices.push_back(topRight.z);
    chunk->vertices.push_back(normal.x);
    chunk->vertices.push_back(normal.y);
    chunk->vertices.push_back(normal.z);

    // texcoords
    if(dim == 0){
	chunk->colors.push_back(0);
	chunk->colors.push_back(0);
	chunk->colors.push_back(((int)block) - 2);

	chunk->colors.push_back(abs(bottomRight.z - bottomLeft.z));
	chunk->colors.push_back(abs(bottomRight.y - bottomLeft.y));
	chunk->colors.push_back(((int)block) - 2);

	chunk->colors.push_back(abs(topLeft.z - bottomLeft.z));
	chunk->colors.push_back(abs(topLeft.y - bottomLeft.y));
	chunk->colors.push_back(((int)block) - 2);

	chunk->colors.push_back(abs(topRight.z - bottomLeft.z));
	chunk->colors.push_back(abs(topRight.y - bottomLeft.y));
	chunk->colors.push_back(((int)block) - 2);
    }else if(dim == 1){
	chunk->colors.push_back(0);
	chunk->colors.push_back(0);
	chunk->colors.push_back(((int)block) - 2);

	chunk->colors.push_back(abs(bottomRight.z - bottomLeft.z));
	chunk->colors.push_back(abs(bottomRight.x - bottomLeft.x));
	chunk->colors.push_back(((int)block) - 2);

	chunk->colors.push_back(abs(topLeft.z - bottomLeft.z));
	chunk->colors.push_back(abs(topLeft.x - bottomLeft.x));
	chunk->colors.push_back(((int)block) - 2);

	chunk->colors.push_back(abs(topRight.z - bottomLeft.z));
	chunk->colors.push_back(abs(topRight.x - bottomLeft.x));
	chunk->colors.push_back(((int)block) - 2);
    }else{
	chunk->colors.push_back(0);
	chunk->colors.push_back(0);
	chunk->colors.push_back(((int)block) - 2);

	chunk->colors.push_back(abs(bottomRight.x - bottomLeft.x));
	chunk->colors.push_back(abs(bottomRight.y - bottomLeft.y));
	chunk->colors.push_back(((int)block) - 2);

	chunk->colors.push_back(abs(topLeft.x - bottomLeft.x));
	chunk->colors.push_back(abs(topLeft.y - bottomLeft.y));
	chunk->colors.push_back(((int)block) - 2);

	chunk->colors.push_back(abs(topRight.x - bottomLeft.x));
	chunk->colors.push_back(abs(topRight.y - bottomLeft.y));
	chunk->colors.push_back(((int)block) - 2);
    }

    if (backFace)
    {   
        chunk->indices.push_back(chunk->vIndex + 2);
        chunk->indices.push_back(chunk->vIndex);
        chunk->indices.push_back(chunk->vIndex + 1);
        chunk->indices.push_back(chunk->vIndex + 1);
        chunk->indices.push_back(chunk->vIndex + 3);
        chunk->indices.push_back(chunk->vIndex + 2);
    }
    else
    {
        chunk->indices.push_back(chunk->vIndex + 2);
        chunk->indices.push_back(chunk->vIndex + 3);
        chunk->indices.push_back(chunk->vIndex + 1);
        chunk->indices.push_back(chunk->vIndex + 1);
        chunk->indices.push_back(chunk->vIndex);
        chunk->indices.push_back(chunk->vIndex + 2);
    }
    chunk->vIndex += 4;
}
};
