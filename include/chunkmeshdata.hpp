#ifndef CHUNK_MESH_DATA_H
#define CHUNK_MESH_DATA_H

enum class ChunkMeshDataType{
    MESH_UPDATE
};

typedef struct ChunkMeshData{
    int32_t index;
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
