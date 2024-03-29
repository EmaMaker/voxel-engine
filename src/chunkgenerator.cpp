#include <array>
#include <iostream>
#include <random> // for std::mt19937

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
#define TREE_STANDARD_HEIGHT 7
#define TREE_HEIGHT_VARIATION 2
#define WOOD_CELL_BORDER (LEAVES_RADIUS-1)
#define WOOD_MAX_OFFSET (WOOD_CELL_SIZE-WOOD_CELL_CENTER-WOOD_CELL_BORDER)

void generateNoise(Chunk::Chunk *chunk);
void generateNoise3D(Chunk::Chunk *chunk);
double evaluateNoise(OpenSimplexNoise::Noise noiseGen, double x, double y, double amplitude, double
	frequency, double persistence, double lacunarity, int octaves);
struct TreeCellInfo evaluateTreeCell(int wcx, int wcz);

std::random_device dev;
std::mt19937 mt(dev());
OpenSimplexNoise::Noise noiseGen1(mt());
OpenSimplexNoise::Noise noiseGen2(mt());
OpenSimplexNoise::Noise noiseGenWood(mt());

// Trees are generated by virtually dividing the world into cells. Each cell can contain exactly one
// tree, with some offset in the position. Having a border in the cell ensures that no trees are generated in
// adjacent blocks
// cover CHUNK_SIZE with WOOD_CELLS + 2 cells before and after the chunk
constexpr int TREE_LUT_SIZE = std::ceil(static_cast<float>(CHUNK_SIZE)/static_cast<float>(WOOD_CELL_SIZE)) + 2;
// Info on the tree cell to generate
struct TreeCellInfo{
    // Cell coordinates (in "tree cell space")
    int wcx, wcz;
    // trunk offset from 0,0 in the cell
    int trunk_x_offset, trunk_z_offset;
    // Global x,z position of the trunk
    int trunk_x, trunk_z;
    // Y of the center of the leaves sphere
    int leaves_y_pos;
};

// Lookup tables for generation
std::array<int, CHUNK_SIZE * CHUNK_SIZE> grassNoiseLUT;
std::array<int, CHUNK_SIZE * CHUNK_SIZE> dirtNoiseLUT;
std::array<TreeCellInfo, TREE_LUT_SIZE*TREE_LUT_SIZE> treeLUT;

void generateNoise(Chunk::Chunk *chunk)
{
    int cx = chunk->getPosition().x * CHUNK_SIZE;
    int cy = chunk->getPosition().y * CHUNK_SIZE;
    int cz = chunk->getPosition().z * CHUNK_SIZE;

    // Precalculate LUTs

    // Terrain LUTs
    // Noise value at a given (x,z), position represents:
    // Grass Noise LUT: Height of the terrain: when the grass is placed and the player will stand
    // Dirt Noise LUT: How many blocks of dirt to place before there is stone
    // Anything below (grass-level - dirt_height) will be stone
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
    // March along the space-filling curve, calculate information about the block at every position
    // A space-filling curve is continuous, so there is no particular order
    // Take advantage of the interval-map structure by only inserting contigous runs of blocks
    Block block_prev{Block::AIR}, block;
    int block_prev_start{0};
    for (int s = 0; s < CHUNK_VOLUME; s++)
    {
	int bx = HILBERT_XYZ_DECODE[s][0];
	int by = HILBERT_XYZ_DECODE[s][1];
	int bz = HILBERT_XYZ_DECODE[s][2];
        int x = bx + cx;
        int y = by + cy;
        int z = bz + cz;
        int lut_index = bx * CHUNK_SIZE + bz;
	
        int grassNoise = grassNoiseLUT[lut_index];
        int dirtNoise = dirtNoiseLUT[lut_index];
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
	int wcx = (int)(x / WOOD_CELL_SIZE) - tree_lut_x_offset; // wood cell x
	int wcz = (int)(z / WOOD_CELL_SIZE) - tree_lut_z_offset; // wood cell z

	// Retrieve info on the cell from LUT
	info = treeLUT[wcx * TREE_LUT_SIZE + wcz];

	// A tree is to be placed in this position if the coordinates are those of the tree of the current cell
	int wood_height = TREE_STANDARD_HEIGHT;
	bool wood = x == info.trunk_x && z == info.trunk_z && y > grassNoiseLUT[lut_index] && y <= info.leaves_y_pos;
	bool leaf{false};

	// Check placing of leaves
	if(wood) leaf = y > info.leaves_y_pos && y < info.leaves_y_pos+LEAVES_RADIUS;
	else{
	    if(!leaf) leaf = utils::withinDistance(x,y,z, info.trunk_x, info.leaves_y_pos, info.trunk_z, LEAVES_RADIUS);

	    // Eventually search neighboring cells
	    if(!leaf && wcx+1 < TREE_LUT_SIZE){
		info = treeLUT[(wcx+1) * TREE_LUT_SIZE + wcz];
		leaf = utils::withinDistance(x,y,z, info.trunk_x, info.leaves_y_pos, info.trunk_z, LEAVES_RADIUS);
	    }

	    if(!leaf && wcx-1 >= 0){
		info = treeLUT[(wcx-1) * TREE_LUT_SIZE + wcz];
		leaf = utils::withinDistance(x,y,z, info.trunk_x, info.leaves_y_pos, info.trunk_z, LEAVES_RADIUS);
	    }

	    if(!leaf && wcz-1 >= 0){
		info = treeLUT[wcx * TREE_LUT_SIZE + (wcz-1)];
		leaf = utils::withinDistance(x,y,z, info.trunk_x, info.leaves_y_pos, info.trunk_z, LEAVES_RADIUS);
	    }

	    if(!leaf && wcz+1 < TREE_LUT_SIZE){
		info = treeLUT[wcx * TREE_LUT_SIZE + (wcz+1)];
		leaf = utils::withinDistance(x,y,z, info.trunk_x, info.leaves_y_pos, info.trunk_z, LEAVES_RADIUS);
	    }
	}

	if(wood) block = Block::WOOD;
	if(leaf) block = Block::LEAVES;

	// Use the interval-map structure of the chunk to compress the world: insert "runs" of
	// equal blocks using indices in the hilbert curve
        if (block != block_prev)
        {
            chunk->setBlocks(block_prev_start, s, block_prev);
            block_prev_start = s;
        }
        block_prev = block;
    }
    // Insert the last run of blocks
    chunk->setBlocks(block_prev_start, CHUNK_VOLUME, block_prev);
    // Mark the chunk as generated, is needed to trigger the next steps
    chunk->setState(Chunk::CHUNK_STATE_GENERATED, true);
}

// Noise evaluation with Fractal Brownian Motion
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

// Tree cell Info
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
	result.trunk_x = wcx * WOOD_CELL_SIZE + wcx_off;
	result.trunk_z = wcz * WOOD_CELL_SIZE + wcz_off;

	result.trunk_x_offset = wcx_off;
	result.trunk_z_offset = wcz_off;

	result.leaves_y_pos = 1 + TREE_STANDARD_HEIGHT + GRASS_OFFSET + evaluateNoise(noiseGen1,
		result.trunk_x, result.trunk_z, NOISE_GRASS_MULT, 0.01, 0.35, 2.1, 5);

	return result;
}

void generateChunk(Chunk::Chunk *chunk)
{
    generateNoise(chunk);
}

/* EXPERIMENTAL STUFF */
void generateNoise3D(Chunk::Chunk *chunk) {
    Block block_prev{Block::AIR}, block;
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
