#include <array>
#include <iostream>
#include <random> // for std::mt19937

#include "block.hpp"
#include "chunkgenerator.hpp"
#include "globals.hpp"
#include "OpenSimplexNoise.h"
#include "utils.hpp"

#define GRASS_OFFSET 100
#define NOISE_GRASS_MULT 50
#define NOISE_DIRT_MULT 3
#define NOISE_DIRT_MIN 3
#define NOISE_DIRT_X_MULT 0.001f
#define NOISE_DIRT_Z_MULT 0.001f
#define NOISE_GRASS_X_MULT 0.018f
#define NOISE_GRASS_Z_MULT 0.018f

void generatePyramid(Chunk::Chunk *chunk);
void generateNoise(Chunk::Chunk *chunk);
void generateNoise3D(Chunk::Chunk *chunk);

void generateChunk(Chunk::Chunk *chunk)
{
    generateNoise(chunk);
}

Block block;

std::random_device dev;
std::mt19937 mt(dev());
OpenSimplexNoise::Noise noiseGen1(mt());
OpenSimplexNoise::Noise noiseGen2(mt());

std::array<int, CHUNK_SIZE * CHUNK_SIZE> grassNoiseLUT;
std::array<int, CHUNK_SIZE * CHUNK_SIZE> dirtNoiseLUT;

double evaluateNoise(OpenSimplexNoise::Noise noiseGen, double x, double y, double amplitude, double
	frequency, double persistence, double lacunarity, int octaves){
    double sum = 0;

    for(int i = 0; i < octaves; i++){
	sum += amplitude * noiseGen.eval(x*frequency, y*frequency);
	amplitude *= persistence;
	frequency *= lacunarity;
    }

    return sum;
}

void generateNoise(Chunk::Chunk *chunk)
{
    for (int i = 0; i < grassNoiseLUT.size(); i++)
    {
        grassNoiseLUT[i] = -1;
        dirtNoiseLUT[i] = -1;
    }

    Block block_prev{Block::AIR};
    int block_prev_start{0};

    // A space filling curve is continuous, so there is no particular order
    for (int s = 0; s < CHUNK_VOLUME; s++)
    {

        int x = HILBERT_XYZ_DECODE[s][0] + CHUNK_SIZE * chunk->getPosition().x;
        int y = HILBERT_XYZ_DECODE[s][1] + CHUNK_SIZE * chunk->getPosition().y;
        int z = HILBERT_XYZ_DECODE[s][2] + CHUNK_SIZE * chunk->getPosition().z;
        int d2 = HILBERT_XYZ_DECODE[s][0] * CHUNK_SIZE + HILBERT_XYZ_DECODE[s][2];

        if (grassNoiseLUT[d2] == -1) 
	    grassNoiseLUT[d2] = GRASS_OFFSET + evaluateNoise(noiseGen1, x, z, NOISE_GRASS_MULT,
		    0.01, 0.35, 2.1, 5);
        if (dirtNoiseLUT[d2] == -1) 
	    dirtNoiseLUT[d2] = NOISE_DIRT_MIN + (int)((1 + noiseGen2.eval(x
			* NOISE_DIRT_X_MULT, z * NOISE_DIRT_Z_MULT)) * NOISE_DIRT_MULT);

	
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

        if (block != block_prev)
        {
            chunk->setBlocks(block_prev_start, s, block_prev);
            block_prev_start = s;
        }

        block_prev = block;
    }

    chunk->setBlocks(block_prev_start, CHUNK_VOLUME, block_prev);
    chunk->setState(Chunk::CHUNK_STATE_GENERATED, true);
}

void generateNoise3D(Chunk::Chunk *chunk) {
    Block block_prev{Block::AIR};
    int block_prev_start{0};

    // A space filling curve is continuous, so there is no particular order
    for (int s = 0; s < CHUNK_VOLUME; s++)
    {

        int x = HILBERT_XYZ_DECODE[s][0] + CHUNK_SIZE * chunk->getPosition().x;
        int y = HILBERT_XYZ_DECODE[s][1] + CHUNK_SIZE * chunk->getPosition().y;
        int z = HILBERT_XYZ_DECODE[s][2] + CHUNK_SIZE * chunk->getPosition().z;
        int d2 = HILBERT_XYZ_DECODE[s][0] * CHUNK_SIZE + HILBERT_XYZ_DECODE[s][2];

	double noise = noiseGen1.eval(x * 0.025, y*0.025, z * 0.025);

        if (noise < -0.1)
            block = Block::STONE;
        else if (noise >= -0.1 && noise < 0)
            block = Block::DIRT;
        else if (noise >= 0 && noise < 0.08)
            block = Block::GRASS;
        else
            block = Block::AIR;

        if (block != block_prev)
        {
            chunk->setBlocks(block_prev_start, s, block_prev);
            block_prev_start = s;
        }

        block_prev = block;
    }

    chunk->setBlocks(block_prev_start, CHUNK_VOLUME, block_prev);
}

void generatePyramid(Chunk::Chunk *chunk)
{
    for (int i = 0; i < CHUNK_SIZE; i++)
        for (int j = 0; j < CHUNK_SIZE; j++)
            for (int k = 0; k < CHUNK_SIZE; k++)
                chunk->setBlock(i >= j && i < CHUNK_SIZE - j && k >= j && k < CHUNK_SIZE - j ? (j & 1) == 0 ? Block::GRASS : Block::STONE : Block::AIR, i, j, k);
}
