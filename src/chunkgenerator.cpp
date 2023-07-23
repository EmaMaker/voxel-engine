#include <array>
#include <iostream>
#include <random> // for std::mt19937
#include <experimental/random>

#include "block.hpp"
#include "chunkgenerator.hpp"
#include "globals.hpp"
#include "OpenSimplexNoise.h"
#include "utils.hpp"

#define GRASS_OFFSET 40
#define NOISE_GRASS_MULT 30
#define NOISE_DIRT_MULT 3
#define NOISE_DIRT_MIN 3
#define NOISE_DIRT_X_MULT 0.001f
#define NOISE_DIRT_Z_MULT 0.001f
#define NOISE_GRASS_X_MULT 0.018f
#define NOISE_GRASS_Z_MULT 0.018f
#define NOISE_TREE_X_MULT 0.01f
#define NOISE_TREE_Z_MULT 0.01f

#define LEAVES_RADIUS 3
#define WOOD_CELL_SIZE 13
#define WOOD_CELL_CENTER 7
#define WOOD_CELL_BORDER (LEAVES_RADIUS-1)
#define WOOD_MAX_OFFSET (WOOD_CELL_SIZE-WOOD_CELL_CENTER-WOOD_CELL_BORDER)
#define TREE_STANDARD_HEIGHT 7
#define TREE_HEIGHT_VARIATION 2

void generatePyramid(Chunk::Chunk *chunk);
void generateNoise(Chunk::Chunk *chunk);
void generateNoise3D(Chunk::Chunk *chunk);

std::random_device dev;
std::mt19937 mt(dev());
OpenSimplexNoise::Noise noiseGen1(mt());
OpenSimplexNoise::Noise noiseGen2(mt());
OpenSimplexNoise::Noise noiseGenWood(mt());

std::array<int, CHUNK_SIZE * CHUNK_SIZE> grassNoiseLUT;
std::array<int, CHUNK_SIZE * CHUNK_SIZE> dirtNoiseLUT;

void generateChunk(Chunk::Chunk *chunk)
{
    generateNoise(chunk);
}

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

const int TREE_MASTER_SEED_X = mt();
const int TREE_MASTER_SEED_Z = mt();

void evaluateTreeCell(int cx, int cz, int wcx, int wcz, int* wcx_offset, int*
	wcz_offset, int* wx, int* wz, int* bwx, int* bwz, int* leaves_noise){
	
	static int old_cx = -1, old_cz = -1, old_leaves_noise = -1;

	int anglex = TREE_MASTER_SEED_X*wcx+TREE_MASTER_SEED_Z*wcz;
	int anglez = TREE_MASTER_SEED_Z*wcz+TREE_MASTER_SEED_X*wcx;

	// Start at the center of the cell, with a bit of random offset
	int wcx_off = WOOD_CELL_CENTER + WOOD_MAX_OFFSET * sines[anglex % 360];
	int wcz_off = WOOD_CELL_CENTER + WOOD_MAX_OFFSET * cosines[anglez % 360];

	//std::cout << "cell: (" << wcx << "," << wcz << "): offset: (" << (int)wcx_off << "," << (int)wcz_off << ")\n";

	// Cell to world coordinates
	*wx = wcx * WOOD_CELL_SIZE + wcx_off;
	*wz = wcz * WOOD_CELL_SIZE + wcz_off;

	*wcx_offset = wcx_off;
	*wcz_offset = wcz_off;

	*bwx = *wx - cx;
	*bwz = *wz - cz;

	if(old_leaves_noise == -1 || old_cx != cx || old_cz != cx || *bwx < 0 || *bwz < 0 || *bwx >= CHUNK_SIZE || *bwz >= CHUNK_SIZE)
	     *leaves_noise = TREE_STANDARD_HEIGHT + GRASS_OFFSET + evaluateNoise(noiseGen1, *wx, *wz, NOISE_GRASS_MULT, 0.01, 0.35, 2.1, 5);
	else *leaves_noise = TREE_STANDARD_HEIGHT + grassNoiseLUT[*bwx * CHUNK_SIZE + *bwz];

	old_leaves_noise = *leaves_noise;
	old_cx = cx;
	old_cz = cz;
}


Block block;

void generateNoise(Chunk::Chunk *chunk)
{
    int cx = chunk->getPosition().x * CHUNK_SIZE;
    int cy = chunk->getPosition().y * CHUNK_SIZE;
    int cz = chunk->getPosition().z * CHUNK_SIZE;

    // Precalculate LUTs
    for (int i = 0; i < grassNoiseLUT.size(); i++)
    {
	int bx = i / CHUNK_SIZE;
	int bz = i % CHUNK_SIZE;

	grassNoiseLUT[i] = GRASS_OFFSET + evaluateNoise(noiseGen1, cx+bx, cz+bz, NOISE_GRASS_MULT, 0.01, 0.35, 2.1, 5);
	dirtNoiseLUT[i] = NOISE_DIRT_MIN + (int)((1 + noiseGen2.eval(cx+bx * NOISE_DIRT_X_MULT,
			cz+bz * NOISE_DIRT_Z_MULT)) * NOISE_DIRT_MULT);
    }

    Block block_prev{Block::AIR};
    int block_prev_start{0};
    
    // A space filling curve is continuous, so there is no particular order
    for (int s = 0; s < CHUNK_VOLUME; s++)
    {

	int bx = HILBERT_XYZ_DECODE[s][0];
	int by = HILBERT_XYZ_DECODE[s][1];
	int bz = HILBERT_XYZ_DECODE[s][2];
        int x = bx + cx;
        int y = by + cy;
        int z = bz + cz;
        int d2 = bx * CHUNK_SIZE + bz;
	
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


	// Divide the world into cells, so that no two trees will be adjacent of each other
	int wcx = x / WOOD_CELL_SIZE, wcz = z / WOOD_CELL_SIZE;
	int wcx_offset, wcz_offset, wx, wz, bwx, bwz, leavesNoise;

	evaluateTreeCell(cx, cz, wcx, wcz, &wcx_offset, &wcz_offset, &wx, &wz, &bwx, &bwz,
		&leavesNoise);

	// A tree is to be places if the coordinates are those of the tree of the current cell
	int wood_height = TREE_STANDARD_HEIGHT;// + noiseGenWood.eval(wcx * NOISE_TREE_X_MULT, wcz * NOISE_TREE_Z_MULT) * TREE_HEIGHT_VARIATION;
	bool wood = x == wx && z == wz && y > grassNoiseLUT[d2] && y <= leavesNoise;
	bool leaf = false;

	leaf = wood && y > leavesNoise && y < leavesNoise+LEAVES_RADIUS;
	if(!leaf) leaf = utils::withinDistance(x,y,z, wx, leavesNoise, wz, LEAVES_RADIUS);

	if(!leaf){
	    evaluateTreeCell(cx, cz, wcx+1, wcz, &wcx_offset, &wcz_offset, &wx, &wz, &bwx, &bwz,
		    &leavesNoise);
	    leaf = utils::withinDistance(x,y,z, wx, leavesNoise, wz, LEAVES_RADIUS);
	}

	if(!leaf){
	    evaluateTreeCell(cx, cz, wcx, wcz+1, &wcx_offset, &wcz_offset, &wx, &wz, &bwx, &bwz,
		    &leavesNoise);
	    leaf = utils::withinDistance(x,y,z, wx, leavesNoise, wz, LEAVES_RADIUS);
	}

	if(!leaf){
	    evaluateTreeCell(cx, cz, wcx-1, wcz, &wcx_offset, &wcz_offset, &wx, &wz, &bwx, &bwz,
		    &leavesNoise);
	 leaf = utils::withinDistance(x,y,z, wx, leavesNoise, wz, LEAVES_RADIUS);
	}

	if(!leaf){
	    evaluateTreeCell(cx, cz, wcx, wcz-1, &wcx_offset, &wcz_offset, &wx, &wz, &bwx, &bwz,
		    &leavesNoise);
	    leaf = utils::withinDistance(x,y,z, wx, leavesNoise, wz, LEAVES_RADIUS);
	}

	if(wood) block = Block::WOOD;
	if(leaf) block = Block::LEAVES;

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
