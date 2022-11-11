#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "chunk.hpp"
#include "chunkgenerator.hpp"
#include "chunkmanager.hpp"
#include "chunkmesh.hpp"
#include "globals.hpp"

#include <iostream>
#include <unordered_map>
#include <string>

std::unordered_map<std::uint32_t, ChunkMesh *> chunks;

namespace chunkmanager
{
    void init()
    {
    }

    void update(float deltaTime)
    {
        // Iterate over all chunks, in concentric spheres starting fron the player and going outwards
        // Eq. of the sphere (x - a)² + (y - b)² + (z - c)² = r²
        glm::vec3 cameraPos = theCamera.getPos();

        int chunkX{(static_cast<int>(cameraPos.x)) / CHUNK_SIZE}, chunkY{(static_cast<int>(cameraPos.y)) / CHUNK_SIZE}, chunkZ{(static_cast<int>(cameraPos.z)) / CHUNK_SIZE};
        // std::cout << "Player at: " << chunkX << ", " << chunkY << ", " << chunkZ << "\n";

        for (int r = 0; r <= RENDER_DISTANCE; r++)
        {
            // Step 1. Eq. of a circle. Fix the x coordinate, get the 2 possible y's
            for (int x = chunkX - r; x <= chunkX + r; x++)
            {

                // Possible optimization: use sqrt lookup
                int y1 = sqrt((r * r) - (x - chunkX) * (x - chunkX)) + chunkY;
                int y2 = -sqrt((r * r) - (x - chunkX) * (x - chunkX)) + chunkY;

                for (int y = y2 + 1; y <= y1; y++)
                {
                    // Step 2. At both y's, get the corresponding z values
                    int z1 = sqrt((r * r) - (x - chunkX) * (x - chunkX) - (y - chunkY) * (y - chunkY)) + chunkZ;
                    int z2 = -sqrt((r * r) - (x - chunkX) * (x - chunkX) - (y - chunkY) * (y - chunkY)) + chunkZ;

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
            }
        }
    }

    void updateChunk(uint32_t index, uint16_t i, uint16_t j, uint16_t k)
    {
        // std::cout << "Checking" << i << ", " << j  << ", " << k <<std::endl;
        if (chunks.find(index) == chunks.end())
        {
            chunks.insert(std::make_pair(index, new ChunkMesh(new Chunk::Chunk(glm::vec3(i, j, k)))));
            generateChunk(chunks.at(index)->chunk);
            chunks.at(index)->mesh();
            // std::cout << "Creating new chunk" << i << ", " << j  << ", " << k <<std::endl;
        }
        else
            chunks.at(index)->draw();
    }

    void destroy()
    {
        for (auto &n : chunks) delete n.second;
    }
};
