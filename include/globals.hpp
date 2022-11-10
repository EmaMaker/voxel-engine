#ifndef GLOBALS_H
#define GLOBALS_H

#include "camera.hpp"

#ifdef GLOBALS_DEFINER
#define extr
#else
#define extr extern
#endif

#define RENDER_DISTANCE 4

extr Camera theCamera;

extr uint32_t MORTON_XYZ_ENCODE[CHUNK_SIZE][CHUNK_SIZE][CHUNK_SIZE];
extr uint32_t MORTON_XYZ_DECODE[CHUNK_SIZE][3];
extr uint32_t HILBERT_XYZ_ENCODE[CHUNK_SIZE][CHUNK_SIZE][CHUNK_SIZE];
extr uint32_t HILBERT_XYZ_DECODE[CHUNK_SIZE][3];

#endif