#ifndef GLOBALS_H
#define GLOBALS_H

#include "camera.hpp"
#include "chunk.hpp"

#ifdef GLOBALS_DEFINER
#define extr
#else
#define extr extern
#endif

#define RENDER_DISTANCE 16

extr Camera theCamera;
// the cube spans in both directions, to each axis has to be multiplied by 2. 2^3=8
constexpr int chunks_volume = 8*(RENDER_DISTANCE*RENDER_DISTANCE*RENDER_DISTANCE);
extr bool wireframe;

extr float sines[360];
extr float cosines[360];

extr uint32_t MORTON_XYZ_ENCODE[CHUNK_SIZE][CHUNK_SIZE][CHUNK_SIZE];
extr uint32_t MORTON_XYZ_DECODE[CHUNK_VOLUME][3];
extr uint32_t HILBERT_XYZ_ENCODE[CHUNK_SIZE][CHUNK_SIZE][CHUNK_SIZE];
extr uint32_t HILBERT_XYZ_DECODE[CHUNK_VOLUME][3];

#endif
