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

#define CHUNK_MESH_DATA_QUANTITY 100
#define CHUNK_MESH_WORLD_LIMIT_BORDERS 0

namespace chunkmesher{

ChunkMeshDataQueue MeshDataQueue;

ChunkMeshDataQueue& getMeshDataQueue(){ return MeshDataQueue; }

void init()
{
    for(int i = 0; i < CHUNK_MESH_DATA_QUANTITY; i++)
	MeshDataQueue.push(new ChunkMeshData{});
}
    
void mesh(Chunk::Chunk* chunk)
{
    ChunkMeshData* mesh_data;
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
    mesh_data->clear();
    mesh_data->message_type = ChunkMeshDataType::MESH_UPDATE;
    mesh_data->index = chunk->getIndex();
    mesh_data->position = chunk->getPosition();

    // convert tree to array since it is easier to work with it
    int length{0};
    std::unique_ptr<Block[]> blocks;

    int k, l, u, v, w, h, n, j, i;
    int x[]{0, 0, 0};
    int q[]{0, 0, 0};
    int du[]{0, 0, 0};
    int dv[]{0, 0, 0};

    // Abort if chunk is empty
    if(chunk->getState(Chunk::CHUNK_STATE_EMPTY)) goto end;

    blocks = chunk->getBlocksArray(&length);
    if(length == 0) goto end;

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
#if CHUNK_MESH_WORLD_LIMIT_BORDERS == 1
			mask[n++] = b1 == b2 ? Block::NULLBLK
                                    : backFace ? b1 == Block::NULLBLK || b1 == Block::AIR ? b2 : Block::NULLBLK
                                    : b2 == Block::NULLBLK || b2 == Block::AIR ? b1 : Block::NULLBLK;
#else
			mask[n++] = b1 == b2 ? Block::NULLBLK
                                    : backFace ? b1 == Block::AIR ? b2 : Block::NULLBLK
                                    : b2 == Block::AIR ? b1 : Block::NULLBLK;
#endif
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

				// bottom left
				mesh_data->vertices.push_back(x[0]); //bottomLeft.x
				mesh_data->vertices.push_back(x[1]); //bottomLeft.y
				mesh_data->vertices.push_back(x[2]); //bottomLeft.z

				// extents, use normals for now
				mesh_data->extents.push_back(du[0] + dv[0]);
				mesh_data->extents.push_back(du[1] + dv[1]);
				mesh_data->extents.push_back(du[2] + dv[2]);

				mesh_data->texinfo.push_back(backFace ? 0.0 : 1.0);
				mesh_data->texinfo.push_back((int)(mask[n]) - 2);
				mesh_data->num_vertices++;
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

end:
    chunk->setState(Chunk::CHUNK_STATE_MESHED, true);
    renderer::getMeshDataQueue().push(mesh_data);
}
};
