#include "chunkmanager.hpp"

#include <atomic>
#include <math.h>
#include <vector>
#include <thread>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include "block.hpp"
#include "chunk.hpp"
#include "chunkgenerator.hpp"
#include "chunkmesher.hpp"
#include "debugwindow.hpp"
#include "globals.hpp"
#include "renderer.hpp"

namespace chunkmanager
{
    void generate();
    void mesh();

    // Concurrent hash table of chunks
    ChunkTable chunks;
    // Chunk indices. Centered at (0,0,0), going in concentric sphere outwards
    std::array<std::array<chunk_intcoord_t, 3>, chunks_volume> chunks_indices;

    /* Multithreading */
    std::atomic_bool should_run;
    std::thread gen_thread, mesh_thread, update_thread;
    // Queue of chunks to be generated
    ChunkPriorityQueue chunks_to_generate_queue;
    // Queue of chunks to be meshed
    ChunkPriorityQueue chunks_to_mesh_queue;

    int block_to_place{2};

    // Init chunkmanager. Chunk indices and start threads
    void init(){
	int index{0};

	for(chunk_intcoord_t i = -RENDER_DISTANCE; i < RENDER_DISTANCE; i++)
	    for(chunk_intcoord_t j = -RENDER_DISTANCE; j < RENDER_DISTANCE; j++)
		for(chunk_intcoord_t k = -RENDER_DISTANCE; k < RENDER_DISTANCE; k++){

		    chunks_indices[index][0]=i;
		    chunks_indices[index][1]=j;
		    chunks_indices[index][2]=k;
		    index++;
		}

	// Also init mesh data queue
	for(int i = 0; i < 10; i++)
	    chunkmesher::getMeshDataQueue().push(new chunkmesher::MeshData());

	should_run = true;
	update_thread = std::thread(update);
	gen_thread = std::thread(generate);
	mesh_thread = std::thread(mesh);

	debug::window::set_parameter("block_type_return", &block_to_place);
    }

    // Method for world generation thread(s)
    void generate(){
	while(should_run){
	    ChunkPQEntry entry;
	    if(chunks_to_generate_queue.try_pop(entry)) generateChunk(entry.first);
	}
	chunks_to_generate_queue.clear();
    }

    // Method for chunk meshing thread(s)
    void mesh(){
	while(should_run){
	    ChunkPQEntry entry;
	    if(chunks_to_mesh_queue.try_pop(entry)){
		Chunk::Chunk* chunk = entry.first;
		if(chunk->getState(Chunk::CHUNK_STATE_GENERATED)){
		    chunkmesher::mesh(chunk);
		    renderer::getChunksToRender().insert(chunk);
		}
	    }
	}
	chunks_to_mesh_queue.clear();
    }

    oneapi::tbb::concurrent_queue<Chunk::Chunk*> chunks_todelete;
    int nUnloaded{0};
    void update(){
	while(should_run) {
	    int chunkX=static_cast<int>(theCamera.getAtomicPosX() / CHUNK_SIZE);
	    int chunkY=static_cast<int>(theCamera.getAtomicPosY() / CHUNK_SIZE);
	    int chunkZ=static_cast<int>(theCamera.getAtomicPosZ() / CHUNK_SIZE);

	    // Update other chunks
	    for(int i = 0; i < chunks_volume; i++) {
		const chunk_intcoord_t x = chunks_indices[i][0] + chunkX;
		const chunk_intcoord_t y = chunks_indices[i][1] + chunkY;
		const chunk_intcoord_t z = chunks_indices[i][2] + chunkZ;

		if(x < 0 || y < 0 || z < 0 || x > 1023 || y > 1023 || z > 1023) continue;
		const chunk_index_t index = Chunk::calculateIndex(x, y, z);

		ChunkTable::accessor a, a1, a2, b1, b2, c1, c2;
		if(!chunks.find(a, index)) chunks.emplace(a, std::make_pair(index, new Chunk::Chunk(glm::vec3(x,y,z))));

		if(! (a->second->getState(Chunk::CHUNK_STATE_GENERATED))) {
		    chunks_to_generate_queue.push(std::make_pair(a->second, GENERATION_PRIORITY_NORMAL));
		}else if(! (a->second->getState(Chunk::CHUNK_STATE_MESHED))){
		    if(
			    (x + 1 > 1023 || (chunks.find(a1, Chunk::calculateIndex(x+1, y, z)) &&
					      a1->second->getState(Chunk::CHUNK_STATE_GENERATED))) && 
			    (x - 1 < 0|| (chunks.find(a1, Chunk::calculateIndex(x-1, y, z)) &&
					      a1->second->getState(Chunk::CHUNK_STATE_GENERATED))) && 
			    (y + 1 > 1023 || (chunks.find(a1, Chunk::calculateIndex(x, y+1, z)) &&
					      a1->second->getState(Chunk::CHUNK_STATE_GENERATED))) && 
			    (y - 1 < 0|| (chunks.find(a1, Chunk::calculateIndex(x, y-1, z)) &&
					      a1->second->getState(Chunk::CHUNK_STATE_GENERATED))) && 
			    (z + 1 > 1023 || (chunks.find(a1, Chunk::calculateIndex(x, y, z+1)) &&
					      a1->second->getState(Chunk::CHUNK_STATE_GENERATED))) && 
			    (z - 1 < 0|| (chunks.find(a1, Chunk::calculateIndex(x, y, z-1)) &&
					      a1->second->getState(Chunk::CHUNK_STATE_GENERATED)))
		      )
		       chunks_to_mesh_queue.push(std::make_pair(a->second, MESHING_PRIORITY_NORMAL));
		}

		a.release();
	    }

	    debug::window::set_parameter("update_chunks_total", (int) (chunks.size()));
	    debug::window::set_parameter("update_chunks_bucket", (int) (chunks.max_size()));

	    Chunk::Chunk* n;
	    nUnloaded = 0;
	    while(chunks_todelete.try_pop(n)){
		chunk_intcoord_t x = static_cast<chunk_intcoord_t>(n->getPosition().x);
		chunk_intcoord_t y = static_cast<chunk_intcoord_t>(n->getPosition().y);
		chunk_intcoord_t z = static_cast<chunk_intcoord_t>(n->getPosition().z);
		if(x > 1023 || y > 1023 || z > 1023) continue;
		const uint32_t index = Chunk::calculateIndex(x, y, z);

		chunks.erase(index);
		//delete n;
		nUnloaded++;
	    }
	}
    }

    oneapi::tbb::concurrent_queue<Chunk::Chunk*>& getDeleteVector(){ return chunks_todelete; }
    std::array<std::array<chunk_intcoord_t, 3>, chunks_volume>& getChunksIndices(){ return chunks_indices; }

    void stop() {
	should_run=false;

	std::cout << "Waiting for secondary threads to shut down" << std::endl;
	update_thread.join();
	std::cout << "Update thread has terminated" << std::endl;
	gen_thread.join();
	std::cout << "Generation thread has terminated" << std::endl;
	mesh_thread.join();
	std::cout << "Meshing thread has terminated" << std::endl;
    }

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
	    if(px < 0 || py < 0 || pz < 0 || px >= 1024 || py >= 1024 || pz >= 1024) continue;

	    ChunkTable::accessor a;
	    if(!chunks.find(a, Chunk::calculateIndex(px, py, pz))) continue;
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
		    if(!chunks.find(a1, Chunk::calculateIndex(px1, py1, pz1))) return;
		    Chunk::Chunk* c1 = a1->second;
		    // place the new block (only stone for now)
		    c1->setBlock((Block)block_to_place, bx1, by1, bz1);

		    // mark the mesh of the chunk the be updated
		    chunks_to_mesh_queue.push(std::make_pair(c1, MESHING_PRIORITY_PLAYER_EDIT));
		    chunks_to_mesh_queue.push(std::make_pair(c, MESHING_PRIORITY_PLAYER_EDIT));

		    debug::window::set_parameter("block_last_action", place);
		    debug::window::set_parameter("block_last_action_block_type", (int)(Block::STONE));
		    debug::window::set_parameter("block_last_action_x", px1*CHUNK_SIZE + bx1);
		    debug::window::set_parameter("block_last_action_y", px1*CHUNK_SIZE + by1);
		    debug::window::set_parameter("block_last_action_z", px1*CHUNK_SIZE + bz1);
		}else{
		    // replace the current block with air to remove it
		    c->setBlock( Block::AIR, bx, by, bz);

		    chunks_to_mesh_queue.push(std::make_pair(c, MESHING_PRIORITY_PLAYER_EDIT));

		    // When necessary, also mesh nearby chunks
		    ChunkTable::accessor a1, a2, b1, b2, c1, c2;
		    if(bx == 0 && px - 1 >= 0 && chunks.find(a1, Chunk::calculateIndex(px - 1, py, pz)))
		      chunkmesher::mesh(a1->second);
		    if(by == 0 && py - 1 >= 0 && chunks.find(b1, Chunk::calculateIndex(px, py - 1, pz)))
		      chunkmesher::mesh(b1->second);
		    if(bz == 0 && pz - 1 >= 0 && chunks.find(c1, Chunk::calculateIndex(px, py, pz - 1)))
		      chunkmesher::mesh(c1->second);
		    if(bx == CHUNK_SIZE - 1 && px +1 < 1024 && chunks.find(a2, Chunk::calculateIndex(px +1, py, pz)))
		      chunkmesher::mesh(a2->second);
		    if(by == CHUNK_SIZE - 1 && py +1 < 1024 && chunks.find(b2, Chunk::calculateIndex(px, py +1, pz)))
		      chunkmesher::mesh(b2->second);
		    if(bz == CHUNK_SIZE - 1 && pz +1 < 1024 && chunks.find(c2, Chunk::calculateIndex(px, py, pz +1)))
		      chunkmesher::mesh(c2->second);

		    debug::window::set_parameter("block_last_action", place);
		    debug::window::set_parameter("block_last_action_block_type", (int) (Block::AIR));
		    debug::window::set_parameter("block_last_action_x", px*CHUNK_SIZE + bx);
		    debug::window::set_parameter("block_last_action_y", py*CHUNK_SIZE + by);
		    debug::window::set_parameter("block_last_action_z", pz*CHUNK_SIZE + bz);

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
	if(!chunks.find(a, Chunk::calculateIndex(cx, cy, cz))) return Block::NULLBLK;
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

