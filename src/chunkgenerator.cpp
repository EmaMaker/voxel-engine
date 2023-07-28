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

// cover CHUNK_SIZE with WOOD_CELLS + 2 cells before and after the chunk
constexpr int TREE_LUT_SIZE = std::ceil(static_cast<float>(CHUNK_SIZE)/static_cast<float>(WOOD_CELL_SIZE)) + 2;
// Info on the tree cell to generate
struct TreeCellInfo{
    // Cell coordinates (in "tree cell space")
    int wcx, wcz;
    // trunk offset from 0,0 in the cell
    int wcx_offset, wcz_offset;
    // Global x,z position of the trunk
    int wx, wz;
    // Y of the center of the leaves sphere
    int leaves_y_pos;
};


// Lookup tables for generation
std::array<int, CHUNK_SIZE * CHUNK_SIZE> grassNoiseLUT;
std::array<int, CHUNK_SIZE * CHUNK_SIZE> dirtNoiseLUT;
std::array<TreeCellInfo, TREE_LUT_SIZE*TREE_LUT_SIZE> treeLUT;

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
struct TreeCellInfo evaluateTreeCell(int wcx, int wcz){
	int anglex = TREE_MASTER_SEED_X*wcx+TREE_MASTER_SEED_Z*wcz;
	int anglez = TREE_MASTER_SEED_Z*wcz+TREE_MASTER_SEED_X*wcx;

	// Start at the center of the cell, with a bit of random offset
	int wcx_off = WOOD_CELL_CENTER + WOOD_MAX_OFFSET * sines[anglex % 360];
	int wcz_off = WOOD_CELL_CENTER + WOOD_MAX_OFFSET * cosines[anglez % 360];

	struct TreeCellInfo result{};

	// Cell to world coordinates
	result.wx = wcx * WOOD_CELL_SIZE + wcx_off;
	result.wz = wcz * WOOD_CELL_SIZE + wcz_off;

	result.wcx_offset = wcx_off;
	result.wcz_offset = wcz_off;

	result.leaves_y_pos = 1 + TREE_STANDARD_HEIGHT + GRASS_OFFSET + evaluateNoise(noiseGen1,
		result.wx, result.wz, NOISE_GRASS_MULT, 0.01, 0.35, 2.1, 5);

	return result;
}


void generateNoise(Chunk::Chunk *chunk)
{
    Block block;

    int cx = chunk->getPosition().x * CHUNK_SIZE;
    int cy = chunk->getPosition().y * CHUNK_SIZE;
    int cz = chunk->getPosition().z * CHUNK_SIZE;

    // Precalculate LUTs
    // Terrain LUTs
    for (int i = 0; i < grassNoiseLUT.size(); i++)
    {
	int bx = i / CHUNK_SIZE;
	int bz = i % CHUNK_SIZE;

	grassNoiseLUT[i] = GRASS_OFFSET + evaluateNoise(noiseGen1, cx+bx, cz+bz, NOISE_GRASS_MULT, 0.01, 0.35, 2.1, 5);
	dirtNoiseLUT[i] = NOISE_DIRT_MIN + (int)((1 + noiseGen2.eval(cx+bx * NOISE_DIRT_X_MULT,
			cz+bz * NOISE_DIRT_Z_MULT)) * NOISE_DIRT_MULT);
    }

    // Tree LUT
    int tree_lut_x_offset = cx / WOOD_CELL_SIZE - 1;
    int tree_lut_z_offset = cz / WOOD_CELL_SIZE - 1;

    for(int i = 0; i < TREE_LUT_SIZE; i++)
	for(int k = 0; k < TREE_LUT_SIZE; k++){
	    int wcx = (tree_lut_x_offset + i);
	    int wcz = (tree_lut_z_offset + k);
	    treeLUT[i * TREE_LUT_SIZE + k] = evaluateTreeCell(wcx, wcz);
	}


    // Generation of terrain
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


	// Divide the world into cells, each with exactly one tree, so that no two trees will be adjacent of each other
	struct TreeCellInfo info;
	int wcx = (int)(x / WOOD_CELL_SIZE) - tree_lut_x_offset;
	int wcz = (int)(z / WOOD_CELL_SIZE) - tree_lut_z_offset;

	// Retrieve info on the cell from LUT
	info = treeLUT[wcx * TREE_LUT_SIZE + wcz];

	// A tree is to be placed in this position if the coordinates are those of the tree of the current cell
	int wood_height = TREE_STANDARD_HEIGHT;
	bool wood = x == info.wx && z == info.wz && y > grassNoiseLUT[d2] && y <= info.leaves_y_pos;
	bool leaf{false};

	// Check placing of leaves
	if(wood) leaf = y > info.leaves_y_pos && y < info.leaves_y_pos+LEAVES_RADIUS;
	else{
	    if(!leaf) leaf = utils::withinDistance(x,y,z, info.wx, info.leaves_y_pos, info.wz, LEAVES_RADIUS);

	    // Eventually search neighboring cells
	    if(!leaf && wcx+1 < TREE_LUT_SIZE){
		info = treeLUT[(wcx+1) * TREE_LUT_SIZE + wcz];
		leaf = utils::withinDistance(x,y,z, info.wx, info.leaves_y_pos, info.wz, LEAVES_RADIUS);
	    }

	    if(!leaf && wcx-1 >= 0){
		info = treeLUT[(wcx-1) * TREE_LUT_SIZE + wcz];
		leaf = utils::withinDistance(x,y,z, info.wx, info.leaves_y_pos, info.wz, LEAVES_RADIUS);
	    }

	    if(!leaf && wcz-1 >= 0){
		info = treeLUT[wcx * TREE_LUT_SIZE + (wcz-1)];
		leaf = utils::withinDistance(x,y,z, info.wx, info.leaves_y_pos, info.wz, LEAVES_RADIUS);
	    }

	    if(!leaf && wcz+1 < TREE_LUT_SIZE){
		info = treeLUT[wcx * TREE_LUT_SIZE + (wcz+1)];
		leaf = utils::withinDistance(x,y,z, info.wx, info.leaves_y_pos, info.wz, LEAVES_RADIUS);
	    }
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
