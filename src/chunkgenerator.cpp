#include <array>
#include <iostream>
#include <random> // for std::mt19937

#include "block.hpp"
#include "chunkgenerator.hpp"
#include "OpenSimplexNoise.h"
#include "utils.hpp"

#define GRASS_OFFSET 40
#define NOISE_GRASS_MULT 20
#define NOISE_DIRT_MULT 3
#define NOISE_DIRT_MIN 2
#define NOISE_DIRT_X_MULT 0.001f
#define NOISE_DIRT_Z_MULT 0.001f
#define NOISE_GRASS_X_MULT 0.035f
#define NOISE_GRASS_Z_MULT 0.035f

void generatePyramid(Chunk::Chunk *chunk);
void generateNoise(Chunk::Chunk *chunk);

void generateChunk(Chunk::Chunk *chunk)
{
    generateNoise(chunk);
}

Block block;

std::mt19937 mt;
OpenSimplexNoise::Noise noiseGen1(1234);
OpenSimplexNoise::Noise noiseGen2(12345);

std::array<int, CHUNK_SIZE * CHUNK_SIZE> grassNoiseLUT;
std::array<int, CHUNK_SIZE * CHUNK_SIZE> dirtNoiseLUT;
void generateNoise(Chunk::Chunk *chunk)
{
    for (int i = 0; i < grassNoiseLUT.size(); i++)
    {
        grassNoiseLUT[i] = -1;
        dirtNoiseLUT[i] = -1;
    }

    Block block_prev{Block::AIR};
    int block_prev_start{0};

    // The order of iteration MUST RESPECT the order in which the array is transformed from 3d to 1d
    for (int k = 0; k < CHUNK_SIZE; k++)
    {
            for (int i = 0; i < CHUNK_SIZE; i++)
        {
        for (int j = 0; j < CHUNK_SIZE; j++)
            {
                int x = i + CHUNK_SIZE * chunk->getPosition().x;
                int y = j + CHUNK_SIZE * chunk->getPosition().y;
                int z = k + CHUNK_SIZE * chunk->getPosition().z;
                int d2 = k * CHUNK_SIZE + i;

                if (grassNoiseLUT[d2] == -1)
                    grassNoiseLUT[d2] = GRASS_OFFSET + (int)((0.5 + noiseGen1.eval(x * NOISE_GRASS_X_MULT, z * NOISE_GRASS_Z_MULT) * NOISE_GRASS_MULT));
                if (dirtNoiseLUT[d2] == -1)
                    dirtNoiseLUT[d2] = NOISE_DIRT_MIN + (int)((0.5 + noiseGen2.eval(x * NOISE_DIRT_X_MULT, z * NOISE_DIRT_Z_MULT) * NOISE_DIRT_MULT));

                int grassNoise = grassNoiseLUT[d2];
                int dirtNoise = dirtNoiseLUT[d2];
                int stoneLevel = grassNoise - dirtNoise;

                if (y < stoneLevel)
                    block = Block::STONE;
                else if (y >= stoneLevel && y < grassNoise)
                    block = Block::DIRT;
                else if (y == grassNoise)
                    block = Block::GRASS;
                else
                    block = Block::AIR;

                int index = Chunk::coord3DTo1D(i, j, k);
                int min = index > block_prev_start ? block_prev_start : index, max = index > block_prev_start ? index : block_prev_start;
                if (block != block_prev)
                {
                    chunk->setBlocks(block_prev_start, index, block_prev);
                    block_prev_start = index;
                }

                block_prev = block;
            }
        }
    }

    chunk->setBlocks(block_prev_start, CHUNK_VOLUME, block_prev);
}

void generatePyramid(Chunk::Chunk *chunk)
{
    for (int i = 0; i < CHUNK_SIZE; i++)
        for (int j = 0; j < CHUNK_SIZE; j++)
            for (int k = 0; k < CHUNK_SIZE; k++)
                //             blocks[utils::coord3DTo1D(i, j, k, CHUNK_SIZE, CHUNK_SIZE, CHUNK_SIZE)] = j == 0 ? Block::STONE : Block::AIR;
                chunk->setBlock(i >= j && i < CHUNK_SIZE - j && k >= j && k < CHUNK_SIZE - j ? (j & 1) == 0 ? Block::GRASS : Block::STONE : Block::AIR, i, j, k);
}