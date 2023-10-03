#include "chunkmanager.hpp"

#include <oneapi/tbb.h>

#include <atomic>
#include <math.h>
#include <queue>
#include <thread>

#include <glm/glm.hpp>
#include <glm/ext.hpp>
#include <glm/gtx/string_cast.hpp>
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
    // Concurrent queue for chunks to be deleted
    IntQueue chunks_todelete;
    // Chunk indices. Centered at (0,0,0), going in concentric sphere outwards
    std::array<std::array<int16_t, 3>, chunks_volume> chunks_indices;

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
	constexpr int rr{RENDER_DISTANCE * RENDER_DISTANCE};

        bool b = true;

	for(int16_t i = -RENDER_DISTANCE; i < RENDER_DISTANCE; i++)
	    for(int16_t j = -RENDER_DISTANCE; j < RENDER_DISTANCE; j++)
		for(int16_t k = -RENDER_DISTANCE; k < RENDER_DISTANCE; k++){

		    chunks_indices[index][0]=i;
		    chunks_indices[index][1]=j;
		    chunks_indices[index][2]=k;
		    index++;
		}

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
	    if(chunks_to_generate_queue.try_pop(entry)){
		generateChunk(entry.first);
		entry.first->setState(Chunk::CHUNK_STATE_IN_GENERATION_QUEUE, false);
	    }
	}
	//chunks_to_generate_queue.clear();
    }

    // Method for chunk meshing thread(s)
    void mesh(){
	while(should_run){
	    ChunkPQEntry entry;
	    if(chunks_to_mesh_queue.try_pop(entry)){
		Chunk::Chunk* chunk = entry.first;
		chunkmesher::mesh(chunk);
		chunk->setState(Chunk::CHUNK_STATE_IN_MESHING_QUEUE, false);
	    }
	}
	//chunks_to_mesh_queue.clear();
    }

    void update(){
	while(should_run) {
	    std::atomic_int nUnloaded{0}, nMarkUnload{0}, nExplored{0}, nMeshed{0}, nGenerated{0};
	    std::atomic_int chunkX=static_cast<int>(theCamera.getAtomicPosX() / CHUNK_SIZE);
	    std::atomic_int chunkY=static_cast<int>(theCamera.getAtomicPosY() / CHUNK_SIZE);
	    std::atomic_int chunkZ=static_cast<int>(theCamera.getAtomicPosZ() / CHUNK_SIZE);

	    // Eventually delete old chunks
	    int i;
	    ChunkTable::accessor a;
	    while(chunks_todelete.try_pop(i)){
		const int index = i;
		if(chunks.find(a, index)){
		    Chunk::Chunk* c = a->second;
		    if(chunks.erase(a)){
			nUnloaded++;
			renderer::getDeleteIndexQueue().push(index);
			delete c;
		    } else std::cout << "failed to delete " << index << std::endl;
		} else std::cout << "no such element found to delete\n";
	    }

	    // Eventually create new chunks near the player
	    for(int i = 0; i < chunks_volume; i++) {
		const int16_t x = chunks_indices[i][0] + chunkX;
		const int16_t y = chunks_indices[i][1] + chunkY;
		const int16_t z = chunks_indices[i][2] + chunkZ;

		if(x < 0 || y < 0 || z < 0 || x > 1023 || y > 1023 || z > 1023) continue;
		nExplored++;

		const int32_t index = Chunk::calculateIndex(x, y, z);
		ChunkTable::accessor a;
		if(!chunks.find(a, index)) chunks.emplace(a, std::make_pair(index, new
			    Chunk::Chunk(glm::vec3(x,y,z))));
	    }

	    // Now update all the chunks
	    oneapi::tbb::parallel_for(chunks.range(), [&](ChunkTable::range_type &r){
		for(ChunkTable::iterator a = r.begin(); a != r.end(); a++){
		    Chunk::Chunk* c = a->second;

		    int x = c->getPosition().x;
		    int y = c->getPosition().y;
		    int z = c->getPosition().z;
		    int distx = x - chunkX;
		    int disty = y - chunkY;
		    int distz = z - chunkZ;

		    int gen{0}, mesh{0}, unload{0};

		    if(
			    distx >= -RENDER_DISTANCE && distx <= RENDER_DISTANCE &&
			    disty >= -RENDER_DISTANCE && disty <= RENDER_DISTANCE &&
			    distz >= -RENDER_DISTANCE && distz <= RENDER_DISTANCE
		      ){

			// If within distance
			// Reset out-of-view flags
			c->setState(Chunk::CHUNK_STATE_OUTOFVISION, false);
			c->setState(Chunk::CHUNK_STATE_UNLOADED, false);

			// If not yet generated
			if(!c->getState(Chunk::CHUNK_STATE_GENERATED)){
			    if(c->isFree()){
				// Generate
				c->setState(Chunk::CHUNK_STATE_IN_GENERATION_QUEUE, true);
				chunks_to_generate_queue.push(std::make_pair(c, GENERATION_PRIORITY_NORMAL));
			    }
			}else{
			    gen++;
			    // If generated but not yet meshed
			    // TODO: not getting meshed
			    if(!c->getState(Chunk::CHUNK_STATE_MESHED)){
				if(c->isFree()){
				    // Mesh
				    c->setState(Chunk::CHUNK_STATE_IN_MESHING_QUEUE, true);
				    chunks_to_mesh_queue.push(std::make_pair(c, MESHING_PRIORITY_NORMAL));
				}
			    }else{
				mesh++;
				// If generated & meshed, render
			    }
			}

		    }else{
			// If not within distance
			if(c->getState(Chunk::CHUNK_STATE_OUTOFVISION)){
			    // If enough time has passed, set to be deleted
			    if(c->isFree() && glfwGetTime() - c->unload_timer >= UNLOAD_TIMEOUT){
				c->setState(Chunk::CHUNK_STATE_IN_DELETING_QUEUE, true);
				chunks_todelete.push(c->getIndex());
				unload++;
			    }
			}else{
			    // Mark as out of view, and start waiting time
			    c->setState(Chunk::CHUNK_STATE_OUTOFVISION, true);
			    c->setState(Chunk::CHUNK_STATE_UNLOADED, false);
			    c->unload_timer = glfwGetTime();
			}
		    }

		    nGenerated += gen;
		    nMeshed += mesh;
		    nMarkUnload += unload;
		}
	    });


	   std::cout << "time: " << glfwGetTime() << "\ntotal: " << chunks.size() << "\ngenerated: " << nGenerated << "\nmeshed: "
		<< nMeshed << "\nunloaded from prev loop: " << nUnloaded << "\nnew marked for unload: " << nMarkUnload << std::endl;
	    
	}
    }

    std::array<std::array<int16_t, 3>, chunks_volume>& getChunksIndices(){ return chunks_indices; }

    void stop() {
	should_run=false;

	std::cout << "waiting for secondary threads to finish\n";
	update_thread.join();
	std::cout << "update thread terminated\n";
	gen_thread.join();
	std::cout << "generation thread terminated\n";
	mesh_thread.join();
	std::cout << "meshing thread terminated\n";

    }

    void destroy(){
    }

	/*
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
	    if(!chunks.find(a, calculateIndex(px, py, pz))) continue;
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
		    if(bx == 0 && px - 1 >= 0 && chunks.find(a1, calculateIndex(px - 1, py, pz)))
		      chunkmesher::mesh(a1->second);
		    if(by == 0 && py - 1 >= 0 && chunks.find(b1, calculateIndex(px, py - 1, pz)))
		      chunkmesher::mesh(b1->second);
		    if(bz == 0 && pz - 1 >= 0 && chunks.find(c1, calculateIndex(px, py, pz - 1)))
		      chunkmesher::mesh(c1->second);
		    if(bx == CHUNK_SIZE - 1 && px +1 < 1024 && chunks.find(a2, calculateIndex(px +1, py, pz)))
		      chunkmesher::mesh(a2->second);
		    if(by == CHUNK_SIZE - 1 && py +1 < 1024 && chunks.find(b2, calculateIndex(px, py +1, pz)))
		      chunkmesher::mesh(b2->second);
		    if(bz == CHUNK_SIZE - 1 && pz +1 < 1024 && chunks.find(c2, calculateIndex(px, py, pz +1)))
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
	if(!chunks.find(a, calculateIndex(cx, cy, cz))) return Block::NULLBLK;
	else {
	    int bx = x % CHUNK_SIZE;
	    int by = y % CHUNK_SIZE;
	    int bz = z % CHUNK_SIZE;

	    Block b =  a->second->getBlock(bx, by, bz);
	    //std::cout << "Block is at " << bx << "," << by << "," << bz << "(" << (int)b << ")\n";
	    return b;
	}
    }*/
};
