#include "chunkmanager.hpp"

#include <atomic>
#include <math.h>
#include <vector>
#include <thread>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <oneapi/tbb/concurrent_hash_map.h>

#include "block.hpp"
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

    int chunks_volume_real;
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
	chunks_volume_real = index;

	// Also init mesh data queue
	for(int i = 0; i < 10; i++)
	    chunkmesher::getMeshDataQueue().push(new chunkmesher::MeshData());

	should_run = true;
	std::thread update_thread (update);
	return update_thread;
    }

    oneapi::tbb::concurrent_queue<Chunk::Chunk*> chunks_todelete;
    int nUnloaded{0};
    void update(){
	while(should_run) {
	    int chunkX=static_cast<int>(theCamera.getAtomicPosX() / CHUNK_SIZE);
	    int chunkY=static_cast<int>(theCamera.getAtomicPosY() / CHUNK_SIZE);
	    int chunkZ=static_cast<int>(theCamera.getAtomicPosZ() / CHUNK_SIZE);

	    // Update other chunks
	    for(int i = 0; i < chunks_volume_real; i++) {
		const uint16_t x = chunks_indices[i][0] + chunkX;
		const uint16_t y = chunks_indices[i][1] + chunkY;
		const uint16_t z = chunks_indices[i][2] + chunkZ;
		const uint32_t index = calculateIndex(x, y, z);

		if(x > 1023 || y > 1023 || z > 1023) continue;

		ChunkTable::accessor a;
		if(!chunks.find(a, index)) chunks.emplace(a, std::make_pair(index, new Chunk::Chunk(glm::vec3(x,y,z))));

		if(! (a->second->getState(Chunk::CHUNK_STATE_GENERATED))) generateChunk(a->second);
		if(! (a->second->getState(Chunk::CHUNK_STATE_MESHED))) chunkmesher::mesh(a->second);

		renderer::getChunksToRender().insert(a->second);

		a.release();
	    }

	    Chunk::Chunk* n;
	    nUnloaded = 0;
	    while(chunks_todelete.try_pop(n)){
		int x = static_cast<uint16_t>(n->getPosition().x);
		int y = static_cast<uint16_t>(n->getPosition().y);
		int z = static_cast<uint16_t>(n->getPosition().z);
		if(x > 1023 || y > 1023 || z > 1023) continue;
		const uint32_t index = calculateIndex(x, y, z);

		delete n;
		chunks.erase(index);
		nUnloaded++;
	    }
	}
    }

    // uint32_t is fine, since i'm limiting the coordinate to only use up to ten bits (1024). There's actually two spare bits
    uint32_t calculateIndex(uint16_t i, uint16_t j, uint16_t k){
	 return i | (j << 10) | (k << 20); 
    }

    oneapi::tbb::concurrent_queue<Chunk::Chunk*>& getDeleteVector(){ return chunks_todelete; }
    std::array<std::array<int, 3>, chunks_volume>& getChunksIndices(){ return chunks_indices; }

    void stop() { should_run=false; }
    void destroy(){
	/*for(const auto& n : chunks){
	    delete n.second;
	}*/
    }

    void blockpick(bool place){
	// cast a ray from the camera in the direction pointed by the camera itself
	glm::vec3 pos = glm::vec3(theCamera.getAtomicPosX(), theCamera.getAtomicPosY(),
		theCamera.getAtomicPosZ());
	for(float t = 0.0; t <= 10.0; t += 0.5){
	    // traverse the ray a block at the time
	    pos = theCamera.getPos() + t * theCamera.getFront();

	    // get which chunk and block the ray is at
	    int px = ((int)(pos.x))/CHUNK_SIZE;
	    int py = ((int)(pos.y))/CHUNK_SIZE;
	    int pz = ((int)(pos.z))/CHUNK_SIZE;
	    int bx = pos.x - px*CHUNK_SIZE;
	    int by = pos.y - py*CHUNK_SIZE;
	    int bz = pos.z - pz*CHUNK_SIZE;

	    // exit early if the position is invalid or the chunk does not exist
	    if(px < 0 || py < 0 || pz < 0 || px >= 1024 || py >= 1024 || pz >= 1024) return;

	    ChunkTable::accessor a;
	    if(!chunks.find(a, calculateIndex(px, py, pz))) return;
	    Chunk::Chunk* c = a->second;
	    if(!c->getState(Chunk::CHUNK_STATE_GENERATED) || c->getState(Chunk::CHUNK_STATE_EMPTY)) continue;

	    Block b = c->getBlock(bx, by, bz);

	    a.release();
	    // if the block is non empty
	    if(b != Block::AIR){

		// if placing a new block
		if(place){
		    // Go half a block backwards on the ray, to check the block where the ray was
		    // coming from
		    // Doing this and not using normal adds the unexpected (and unwanted) ability to
		    // place blocks diagonally, without faces colliding with the block that has
		    // been clicked
		    pos -= theCamera.getFront()*0.5f;

		    int px1 = ((int)(pos.x))/CHUNK_SIZE;
		    int py1 = ((int)(pos.y))/CHUNK_SIZE;
		    int pz1 = ((int)(pos.z))/CHUNK_SIZE;
		    int bx1 = pos.x - px1*CHUNK_SIZE;
		    int by1 = pos.y - py1*CHUNK_SIZE;
		    int bz1 = pos.z - pz1*CHUNK_SIZE;

		    // exit early if the position is invalid or the chunk does not exist
		    if(px1 < 0 || py1 < 0 || pz1 < 0 || px1 >= 1024 || py1 >= 1024 || pz1 >= 1024) return;
		    ChunkTable::accessor a1;
		    if(!chunks.find(a1, calculateIndex(px1, py1, pz1))) return;
		    Chunk::Chunk* c1 = a1->second;
		    // place the new block (only stone for now)
		    c1->setBlock( Block::STONE, bx1, by1, bz1);

		    // mark the mesh of the chunk the be updated
		    c1->setState(Chunk::CHUNK_STATE_MESHED, false);
		}else{
		    // replace the current block with air to remove it
		    c->setBlock( Block::AIR, bx, by, bz);

		    c->setState(Chunk::CHUNK_STATE_MESHED, false);
		}
		break;
	    }
	}
    }

    Block getBlockAtPos(int x, int y, int z){
	if(x < 0 || y < 0 || z < 0) return Block::NULLBLK;

	int cx = static_cast<int>(x / CHUNK_SIZE);
	int cy = static_cast<int>(y / CHUNK_SIZE);
	int cz = static_cast<int>(z / CHUNK_SIZE);

	if(cx < 0 || cy < 0 || cz < 0 || cx > 1023 || cy > 1023 || cz > 1023) return Block::NULLBLK;

	//std::cout << "Block at " << x << ", " << y << ", " << z << " is in chunk " << cx << "," << cy << "," << cz << "\n";
	ChunkTable::accessor a;
	if(!chunks.find(a, calculateIndex(cx, cy, cz))) return Block::NULLBLK;
	else {
	    int bx = x % CHUNK_SIZE;
	    int by = y % CHUNK_SIZE;
	    int bz = z % CHUNK_SIZE;

	    Block b =  a->second->getBlock(bx, by, bz);
	    //std::cout << "Block is at " << bx << "," << by << "," << bz << "(" << (int)b << ")\n";
	    return b;
	}
    }
};
