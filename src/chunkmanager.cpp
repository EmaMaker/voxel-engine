#include "chunkmanager.hpp"

#include <atomic>
#include <math.h>
#include <vector>
#include <thread>

#include <glm/glm.hpp>
#include <glm/gtx/string_cast.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <oneapi/tbb/parallel_for.h>

#include "block.hpp"
#include "chunk.hpp"
#include "chunkgenerator.hpp"
#include "chunkmesher.hpp"
#include "debugwindow.hpp"
#include "globals.hpp"
#include "renderer.hpp"
#include "utils.hpp"

namespace chunkmanager
{
    void blockpick(WorldUpdateMsg& msg); // There's no need of passing by value again (check
					 // controls.cpp)
    void generate();
    void mesh();
    void send_to_chunk_meshing_thread(Chunk::Chunk* c, int priority);

    /* Chunk holding data structures */
    // Concurrent hash table of chunks
    ChunkTable chunks;
    // Chunk indices. Centered at (0,0,0), going in concentric sphere outwards
    std::array<std::array<chunk_intcoord_t, 3>, chunks_volume> chunks_indices;

    /* World Update messaging data structure */
    WorldUpdateMsgQueue WorldUpdateQueue;

    /* Multithreading */
    std::atomic_bool should_run;
    std::thread gen_thread, mesh_thread, update_thread;

    // Queue of chunks to be generated
    ChunkPriorityQueue chunks_to_generate_queue;
    // Queue of chunks to be meshed
    ChunkPriorityQueue chunks_to_mesh_queue;

    WorldUpdateMsgQueue& getWorldUpdateQueue(){ return WorldUpdateQueue; }
    
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

	should_run = true;
	update_thread = std::thread(update);
	gen_thread = std::thread(generate);
	mesh_thread = std::thread(mesh);
    }

    // Method for world generation thread(s)
    void generate(){
	while(should_run){
	    ChunkPQEntry entry;
	    if(chunks_to_generate_queue.try_pop(entry)){
		Chunk::Chunk* chunk = entry.first;
		generateChunk(chunk);
		chunk->setState(Chunk::CHUNK_STATE_IN_GENERATION_QUEUE, false);
	    }
	}
	chunks_to_generate_queue.clear();
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
	chunks_to_mesh_queue.clear();
    }

    void send_to_chunk_meshing_thread(Chunk::Chunk* c, int priority){
	c->setState(Chunk::CHUNK_STATE_IN_MESHING_QUEUE, true);
	chunks_to_mesh_queue.push(std::make_pair(c, MESHING_PRIORITY_NORMAL));
    }

    oneapi::tbb::concurrent_queue<chunk_index_t> chunks_todelete;
    void update(){
	while(should_run) {
	    /* Setup variables for the whole loop */
	    // Atomic is needed by parallel_for
	    std::atomic_int nUnloaded{0}, nMarkUnload{0}, nExplored{0}, nMeshed{0}, nGenerated{0};
	    std::atomic_int chunkX=static_cast<int>(theCamera.getAtomicPosX() / CHUNK_SIZE);
	    std::atomic_int chunkY=static_cast<int>(theCamera.getAtomicPosY() / CHUNK_SIZE);
	    std::atomic_int chunkZ=static_cast<int>(theCamera.getAtomicPosZ() / CHUNK_SIZE);

	    /* Process update messages before anything happens */
	    WorldUpdateMsg msg;
	    while(WorldUpdateQueue.try_pop(msg)){
		switch(msg.msg_type){
		    case WorldUpdateMsgType::BLOCKPICK_BREAK:
		    case WorldUpdateMsgType::BLOCKPICK_PLACE:
			blockpick(msg);
			break;
		}
	    }


	    /* Delete old chunks */
	    // In my head it makes sense to first delete old chunks, then create new ones
	    // I think it's easier for memory allocator to re-use the memory that was freed just
	    // before, but this isn't backed be any evidence and I might be wrong. Anyway this way
	    // works fine so I'm gonna keep it.
	    chunk_index_t i;
	    ChunkTable::accessor a;
	    while(chunks_todelete.try_pop(i)){
		const chunk_index_t index = i;
		if(chunks.find(a, index)){
		    Chunk::Chunk* c = a->second;
		    // Use the accessor to erase the element
		    // Using the key doesn't work
		    if(chunks.erase(a)){
			nUnloaded++;
			renderer::getDeleteIndexQueue().push(index);
			delete c;
		    } else {
			c->setState(Chunk::CHUNK_STATE_IN_DELETING_QUEUE, false);
			std::cout << "failed to delete " << index << std::endl;
		    }
		} else std::cout << "no such element found to delete\n";
	    }

	    /* Create new chunks around the player */
	    for(int i = 0; i < chunks_volume; i++) {
		const chunk_intcoord_t x = chunks_indices[i][0] + chunkX;
		const chunk_intcoord_t y = chunks_indices[i][1] + chunkY;
		const chunk_intcoord_t z = chunks_indices[i][2] + chunkZ;

		if(x < 0 || y < 0 || z < 0 || x > 1023 || y > 1023 || z > 1023) continue;
		nExplored++;

		const chunk_index_t index = Chunk::calculateIndex(x, y, z);
		ChunkTable::accessor a;
		if(!chunks.find(a, index)) chunks.emplace(a, std::make_pair(index, new
			    Chunk::Chunk(glm::vec3(x,y,z))));
	    }

	    /* Update all the chunks */
	    oneapi::tbb::parallel_for(chunks.range(), [&](ChunkTable::range_type &r){
		for(ChunkTable::iterator a = r.begin(); a != r.end(); a++){
		    Chunk::Chunk* c = a->second;

		    int x = c->getPosition().x;
		    int y = c->getPosition().y;
		    int z = c->getPosition().z;
		    int distx = x - chunkX;
		    int disty = y - chunkY;
		    int distz = z - chunkZ;

		    // Local variables avoid continously having to call atomic variables
		    int gen{0}, mesh{0}, unload{0};

		    if(
			    distx >= -RENDER_DISTANCE && distx < RENDER_DISTANCE &&
			    disty >= -RENDER_DISTANCE && disty < RENDER_DISTANCE &&
			    distz >= -RENDER_DISTANCE && distz < RENDER_DISTANCE
		      ){

			// If within distance
			// Reset out-of-view flags
			c->setState(Chunk::CHUNK_STATE_OUTOFVISION, false);
			c->setState(Chunk::CHUNK_STATE_UNLOADED, false);

			// If not yet generated
			if(!c->getState(Chunk::CHUNK_STATE_GENERATED)){
			    if(c->isFree()){
				// Generate

				// Mark as present in the queue before sending to avoid strange
				// a chunk being marked as in the queue after it was already
				// processed
				c->setState(Chunk::CHUNK_STATE_IN_GENERATION_QUEUE, true);
				chunks_to_generate_queue.push(std::make_pair(c, GENERATION_PRIORITY_NORMAL));
			    }
			}else{
			    gen++;
			    // If generated but not yet meshed
			    if(!c->getState(Chunk::CHUNK_STATE_MESHED)){
				ChunkTable::accessor a1;

				// Checking if nearby chunks have been generated allows for seamless
				// borders between chunks
				if(c->isFree() &&
				    (distx+1 >= RENDER_DISTANCE || x + 1 > 1023 || (chunks.find(a1, Chunk::calculateIndex(x+1, y, z)) &&
					a1->second->getState(Chunk::CHUNK_STATE_GENERATED))) &&
				    (distx-1 < -RENDER_DISTANCE || x - 1 < 0 || (chunks.find(a1, Chunk::calculateIndex(x-1, y, z)) &&
					a1->second->getState(Chunk::CHUNK_STATE_GENERATED))) &&
				    (disty+1 >= RENDER_DISTANCE || y + 1 > 1023 || (chunks.find(a1, Chunk::calculateIndex(x, y+1, z)) &&
					a1->second->getState(Chunk::CHUNK_STATE_GENERATED))) &&
				    (disty-1 < -RENDER_DISTANCE || y - 1 < 0|| (chunks.find(a1, Chunk::calculateIndex(x, y-1, z)) &&
					a1->second->getState(Chunk::CHUNK_STATE_GENERATED))) &&
				    (distz+1 >= RENDER_DISTANCE || z + 1 > 1023 || (chunks.find(a1, Chunk::calculateIndex(x, y, z+1)) &&
					a1->second->getState(Chunk::CHUNK_STATE_GENERATED))) &&
				    (distz-1 < -RENDER_DISTANCE || z - 1 < 0|| (chunks.find(a1, Chunk::calculateIndex(x, y, z-1)) &&
					a1->second->getState(Chunk::CHUNK_STATE_GENERATED)))
				  )
				{
				    // Mesh

				    // Mark as present in the queue before sending to avoid strange
				    // a chunk being marked as in the queue after it was already
				    // processed
				    send_to_chunk_meshing_thread(c, MESHING_PRIORITY_NORMAL);
				}
			    }else mesh++;
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

		    // Update atomic variables only once at the end
		    nGenerated += gen;
		    nMeshed += mesh;
		    nMarkUnload += unload;
		}
	    });

	    debug::window::set_parameter("update_chunks_total", (int)chunks.size());
	    debug::window::set_parameter("update_chunks_generated", (int) nGenerated);
	    debug::window::set_parameter("update_chunks_meshed", (int) nMeshed);
	    debug::window::set_parameter("update_chunks_freed", (int) nUnloaded);
	    debug::window::set_parameter("update_chunks_explored", (int) nExplored);
	}
    }

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
	for(const auto& n : chunks){
	    delete n.second;
	}
	chunks.clear();
    }


    glm::vec3 ray_intersect(glm::vec3 startposition, glm::vec3 startdir){
	int old_bx{0}, old_by{0}, old_bz{0};
	int old_px{0}, old_py{0}, old_pz{0};
	Chunk::Chunk* old_chunk{nullptr};
	glm::vec3 old_pos;

	// cast a ray from the camera in the direction pointed by the camera itself
	glm::vec3 origin = startposition;
	glm::vec3 pos = origin;
	glm::vec3 front = startdir;
	for(float t = 0.0; t <= 10.0; t += 0.5){
	    // traverse the ray a block at the time
	    pos = origin + t*front;

	    // get which chunk and block the ray is at
	    int px = ((int)(pos.x))/CHUNK_SIZE;
	    int py = ((int)(pos.y))/CHUNK_SIZE;
	    int pz = ((int)(pos.z))/CHUNK_SIZE;
	    int bx = pos.x - px*CHUNK_SIZE;
	    int by = pos.y - py*CHUNK_SIZE;
	    int bz = pos.z - pz*CHUNK_SIZE;

	    if(bx == old_bx && by == old_by && bz == old_bz) continue;

	    // exit early if the position is invalid or the chunk does not exist
	    if(px < 0 || py < 0 || pz < 0 || px >= 1024 || py >= 1024 || pz >= 1024) continue;

	    ChunkTable::const_accessor a;
	    if(!chunks.find(a, Chunk::calculateIndex(px, py, pz))) continue;
	    Chunk::Chunk* c = a->second;
	    if(!c->isFree() || !c->getState(Chunk::CHUNK_STATE_GENERATED)){
		a.release();
		continue;
	    }

	    Block b = c->getBlock(bx, by, bz);
	    a.release();
	    
	    // if the block is non empty
	    if(b != Block::AIR) return pos;

	    old_chunk = c;
	    old_bx = bx;
	    old_by = by;
	    old_bz = bz;
	    old_px = px;
	    old_py = py;
	    old_pz = pz;
	    old_pos = pos;

	}
	return glm::vec3(-1);
    }

    void blockpick(WorldUpdateMsg& msg){
	//std::cout << glm::to_string(ray_intersect(msg.cameraPos, msg.cameraFront)) << std::endl;
	glm::vec3 ray_pos = ray_intersect(msg.cameraPos, msg.cameraFront);
	if(ray_pos == glm::vec3(-1)) return;

	// Chunk in which the blockpick is happening
	int chunkx = (int)(ray_pos.x) / CHUNK_SIZE;
	int chunky = (int)(ray_pos.y) / CHUNK_SIZE;
	int chunkz = (int)(ray_pos.z) / CHUNK_SIZE;
	// Block (chunk coord) in which the blockpick is happening
	int blockx = ray_pos.x - chunkx*CHUNK_SIZE;
	int blocky = ray_pos.y - chunky*CHUNK_SIZE;
	int blockz = ray_pos.z - chunkz*CHUNK_SIZE;

	// The chunk must exist, otherwise ray_intersect would have returned an error
	// Also, the block must be different from AIR
	
	ChunkTable::accessor a;
	if(!chunks.find(a, Chunk::calculateIndex(chunkx, chunky, chunkz))) return;
	Chunk::Chunk* c = a->second;
	if(!(c->isFree() && c->getState(Chunk::CHUNK_STATE_GENERATED))) return;

	if(msg.msg_type == WorldUpdateMsgType::BLOCKPICK_BREAK){
	    c->setBlock(Block::AIR, blockx, blocky, blockz);
	    send_to_chunk_meshing_thread(c, MESHING_PRIORITY_PLAYER_EDIT);
	}else{
	    // Traverse voxel using Amanatides&Woo traversal algorithm
	    // http://www.cse.yorku.ca/~amana/research/grid.pdf

	    glm::vec3 pos = msg.cameraPos;
	    glm::vec3 front = glm::normalize(pos - ray_pos);

	    // Original chunk in which the blockpick started
	    const int ochunkX=chunkx, ochunkY = chunky, ochunkZ = chunkz;

	    // The ray has equation pos + t*front

	    // Initialize phase
	    // Origin integer voxel coordinates
	    // Avoid floating point accuracy errors: work as close to 0 as possible, translate
	    // everything later
	    int basex = std::floor(ray_pos.x);
	    int basey = std::floor(ray_pos.y);
	    int basez = std::floor(ray_pos.z);
	    double x = ray_pos.x - basex;
	    double y = ray_pos.y - basey;
	    double z = ray_pos.z - basez;

	    auto sign = [=](double f){ return f > 0 ? 1 : f < 0 ? -1 : 0; };
	    auto tmax = [=](double p, double dir){
		int s = sign(dir);

		if(s > 0)
		    return (1 - p) / dir;
		else if(s < 0)
		    return -(p) / dir;
		return 0.0;
	    };


	    // Step
	    int stepX = sign(front.x);
	    int stepY = sign(front.y);
	    int stepZ = sign(front.z);

	    // tMax: the value of t at which the ray crosses the first voxel boundary
	    double tMaxX = tmax(x, front.x);
	    double tMaxY = tmax(y, front.y);
	    double tMaxZ = tmax(z, front.z);

	    // tDelta: how far along the ray along they ray (in units of t) for the _ component of such
	    // a movement to equal the width of a voxel
	    double tDeltaX = stepX / front.x;
	    double tDeltaY = stepY / front.y;
	    double tDeltaZ = stepZ / front.z;

	    for(int i = 0; i < 10; i++){
		if(tMaxX < tMaxY){
		    if(tMaxX < tMaxZ) {
			x += stepX;
			tMaxX += tDeltaX;
		    }else{
			z += stepZ;
			tMaxZ += tDeltaZ;
		    }
		}else{
		    if(tMaxY < tMaxZ){
			y += stepY;
			tMaxY += tDeltaY;
		    }else{
			z += stepZ;
			tMaxZ += tDeltaZ;
		    }
		}

		int realx = basex + x;
		int realy = basey + y;
		int realz = basez + z;

		chunkx = realx / CHUNK_SIZE;
		chunky = realy / CHUNK_SIZE;
		chunkz = realz / CHUNK_SIZE;

		if(chunkx < 0 || chunky < 0 || chunkz < 0 || chunkx > 1023 || chunky > 1023 ||
			chunkz > 1023) continue;

		blockx = realx - chunkx*CHUNK_SIZE;
		blocky = realy - chunky*CHUNK_SIZE;
		blockz = realz - chunkz*CHUNK_SIZE;


		Chunk::Chunk* chunk;
		ChunkTable::accessor b;

		if(chunkx != ochunkX || chunky != ochunkY || chunkz != ochunkZ){
		    if(!chunks.find(b, Chunk::calculateIndex(chunkx, chunky, chunkz)))
			continue;
		    chunk = b->second;
		    if(!(chunk->isFree() && chunk->getState(Chunk::CHUNK_STATE_GENERATED)))
			continue;

		}else{
		    chunk = c;
		}

		if(chunk->getBlock(blockx, blocky, blockz) != Block::AIR) continue;
		chunk->setBlock(msg.block, blockx, blocky, blockz);
		send_to_chunk_meshing_thread(chunk, MESHING_PRIORITY_PLAYER_EDIT);
		break;
	    }
	}

	// Release the chunk in which the blockpick started to avoid locks
	a.release();
	 // When necessary, also mesh nearby chunks
	ChunkTable::accessor a1, a2, b1, b2, c1, c2;
	if(blockx == 0 && chunkx - 1 >= 0 && chunks.find(a1, Chunk::calculateIndex(chunkx - 1, chunky, chunkz)))
	  send_to_chunk_meshing_thread(a1->second, MESHING_PRIORITY_PLAYER_EDIT);
	if(blocky == 0 && chunky - 1 >= 0 && chunks.find(b1, Chunk::calculateIndex(chunkx, chunky - 1, chunkz)))
	  send_to_chunk_meshing_thread(b1->second, MESHING_PRIORITY_PLAYER_EDIT);
	if(blockz == 0 && chunkz - 1 >= 0 && chunks.find(c1, Chunk::calculateIndex(chunkx, chunky, chunkz - 1)))
	  send_to_chunk_meshing_thread(c1->second, MESHING_PRIORITY_PLAYER_EDIT);
	if(blockx == CHUNK_SIZE - 1 && chunkx +1 < 1024 && chunks.find(a2, Chunk::calculateIndex(chunkx +1, chunky, chunkz)))
	  send_to_chunk_meshing_thread(a2->second, MESHING_PRIORITY_PLAYER_EDIT);
	if(blocky == CHUNK_SIZE - 1 && chunky +1 < 1024 && chunks.find(b2, Chunk::calculateIndex(chunkx, chunky +1, chunkz)))
	  send_to_chunk_meshing_thread(b2->second, MESHING_PRIORITY_PLAYER_EDIT);
	if(blockz == CHUNK_SIZE - 1 && chunkz +1 < 1024 && chunks.find(c2, Chunk::calculateIndex(chunkx, chunky, chunkz +1)))
	  send_to_chunk_meshing_thread(c2->second, MESHING_PRIORITY_PLAYER_EDIT);

	// Update debugging information

	debug::window::set_parameter("block_last_action", msg.msg_type ==
		WorldUpdateMsgType::BLOCKPICK_PLACE);
	debug::window::set_parameter("block_last_action_block_type", (int)(msg.msg_type ==
		WorldUpdateMsgType::BLOCKPICK_PLACE ? msg.block : Block::AIR));
	debug::window::set_parameter("block_last_action_x", chunkx*CHUNK_SIZE+blockx);
	debug::window::set_parameter("block_last_action_y", chunky*CHUNK_SIZE+blocky);
	debug::window::set_parameter("block_last_action_z", chunkz*CHUNK_SIZE+blockz);
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

