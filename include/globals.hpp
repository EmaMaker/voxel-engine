#ifndef GLOBALS_H
#define GLOBALS_H

#include "camera.hpp"
#include "chunk.hpp"

#ifdef GLOBALS_DEFINER
#define extr
#else
#define extr extern
#endif

#define RENDER_DISTANCE 8

extr Camera theCamera;
//constexpr int chunks_volume = static_cast<int>(1.333333333333*M_PI*(RENDER_DISTANCE*RENDER_DISTANCE*RENDER_DISTANCE));
constexpr int chunks_volume = RENDER_DISTANCE*RENDER_DISTANCE*RENDER_DISTANCE*8;
extr bool wireframe;

extr float sines[360];
extr float cosines[360];

extr uint32_t MORTON_XYZ_ENCODE[CHUNK_SIZE][CHUNK_SIZE][CHUNK_SIZE];
extr uint32_t MORTON_XYZ_DECODE[CHUNK_VOLUME][3];
extr uint32_t HILBERT_XYZ_ENCODE[CHUNK_SIZE][CHUNK_SIZE][CHUNK_SIZE];
extr uint32_t HILBERT_XYZ_DECODE[CHUNK_VOLUME][3];

#endif
