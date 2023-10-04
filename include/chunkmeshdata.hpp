#ifndef CHUNK_MESH_DATA_H
#define CHUNK_MESH_DATA_H

#include <oneapi/tbb/concurrent_queue.h>
#include "chunk.hpp"

enum class ChunkMeshDataType{
    MESH_UPDATE
};

typedef struct ChunkMeshData{
    chunk_index_t index;
    glm::vec3 position;
    int num_vertices = 0;

    std::vector<GLfloat> vertices;
    std::vector<GLfloat> extents;
    std::vector<GLfloat> texinfo;
    
    ChunkMeshDataType message_type;

    void clear(){
	vertices.clear();
	texinfo.clear();
	extents.clear();
	index = 0;
	position = glm::vec3(0);
	num_vertices = 0;
    }

}ChunkMeshData;
typedef oneapi::tbb::concurrent_queue<ChunkMeshData*> ChunkMeshDataQueue;


#endif

