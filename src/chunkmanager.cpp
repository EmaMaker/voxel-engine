#include "chunkmanager.hpp"

#include <atomic>
#include <math.h>
#include <thread>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <oneapi/tbb/concurrent_hash_map.h>

#include "chunk.hpp"
#include "chunkgenerator.hpp"
#include "chunkmesher.hpp"
#include "globals.hpp"
#include "renderer.hpp"

namespace chunkmanager
{
    typedef oneapi::tbb::concurrent_hash_map<uint32_t, Chunk::Chunk*> ChunkTable;
    ChunkTable chunks;

    //std::unordered_map<std::uint32_t, Chunk::Chunk *> chunks;
    std::array<std::array<int, 3>, chunks_volume> chunks_indices;

    std::atomic_bool should_run;
    std::thread init(){
	int index{0};
	int rr{RENDER_DISTANCE * RENDER_DISTANCE};

        int xp{0}, x{0};
        bool b = true;

        // Iterate over all chunks, in concentric spheres starting fron the player and going outwards. Alternate left and right
        // Eq. of the sphere (x - a)² + (y - b)² + (z - c)² = r²
        while (xp <= RENDER_DISTANCE)
        {
	    // Alternate between left and right
            if (b) x = +xp;
            else x = -xp;

	    // Step 1. At current x, get the corresponding y values (2nd degree equation, up to 2
	    // possible results)
            int y1 = static_cast<int>(sqrt((rr) - x*x));

            for (int y = -y1 + 1 ; y <= y1; y++)
            {
                // Step 2. At both y's, get the corresponding z values
		int z1 = static_cast<int>(sqrt( rr - x*x - y*y));

                for (int z = -z1 + 1; z <= z1; z++){
		    chunks_indices[index][0] = x;
		    chunks_indices[index][1] = y;
		    chunks_indices[index][2] = z;
		    index++;
		}
            }

            if (!b)
            {
                xp++;
                b = true;
            }
            else  b = false;
	}

	should_run = true;
	std::thread update_thread (update);
	return update_thread;
    }

    std::vector<uint32_t> chunks_todelete;
    void update(){
	while(should_run) {
	    //cameraPos = theCamera.getPos();
	    int chunkX=static_cast<int>(theCamera.getAtomicPosX() / CHUNK_SIZE);
	    int chunkY=static_cast<int>(theCamera.getAtomicPosY() / CHUNK_SIZE);
	    int chunkZ=static_cast<int>(theCamera.getAtomicPosZ() / CHUNK_SIZE);

	    // Update other chunks
	    for(int i = 0; i < chunks_volume; i++) {
		const uint16_t x = chunks_indices[i][0] + chunkX;
		const uint16_t y = chunks_indices[i][1] + chunkY;
		const uint16_t z = chunks_indices[i][2] + chunkZ;
		const uint32_t index = calculateIndex(x, y, z);

		ChunkTable::accessor a;
		if(!chunks.find(a, index)) chunks.emplace(a, std::make_pair(index, new Chunk::Chunk(glm::vec3(x,y,z))));

		if(! (a->second->getState(Chunk::CHUNK_STATE_GENERATED))) generateChunk(a->second);
		if(! (a->second->getState(Chunk::CHUNK_STATE_MESHED))) chunkmesher::mesh(a->second);

		renderer::getChunksToRender().insert(a->second);

		a.release();
	    }
	}
    }

    // uint32_t is fine, since i'm limiting the coordinate to only use up to ten bits (1024). There's actually two spare bits
    uint32_t calculateIndex(uint16_t i, uint16_t j, uint16_t k){
	 return i | (j << 10) | (k << 20); 
    }

    std::array<std::array<int, 3>, chunks_volume>& getChunksIndices(){ return chunks_indices; }

    void stop() { should_run=false; }
    void destroy(){
	/*for(const auto& n : chunks){
	    delete n.second;
	}*/
    }

};
