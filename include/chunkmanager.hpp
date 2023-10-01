#ifndef CHUNKMANAGER_H
#define CHUNKMANAGER_H

#include "chunk.hpp"

#include <oneapi/tbb/concurrent_hash_map.h>
#include <oneapi/tbb/concurrent_queue.h>
#include <oneapi/tbb/concurrent_priority_queue.h>

#include <unordered_map>
#include <thread>

#include "globals.hpp"

// Seconds to be passed outside of render distance for a chunk to be destroyed
#define UNLOAD_TIMEOUT 5

#define	MESHING_PRIORITY_NORMAL 0
#define MESHING_PRIORITY_PLAYER_EDIT 10
#define GENERATION_PRIORITY_NORMAL 0

namespace chunkmanager
{
    typedef oneapi::tbb::concurrent_hash_map<uint32_t, Chunk::Chunk*> ChunkTable;
    typedef oneapi::tbb::concurrent_queue<int> IntQueue;

    //typedef std::unordered_map<int32_t, Chunk::Chunk*> ChunkTable;
    typedef std::pair<Chunk::Chunk*, uint8_t> ChunkPQEntry;
    // The comparing function to use
    struct compare_f {
	bool operator()(const ChunkPQEntry& u, const ChunkPQEntry& v) const {
	    return u.second > v.second;
	}
    };
    typedef oneapi::tbb::concurrent_priority_queue<ChunkPQEntry, compare_f> ChunkPriorityQueue;

    void init();
    void blockpick(bool place);
    int32_t calculateIndex(int16_t i, int16_t j, int16_t k);
    
    void stop();
    void destroy();
    std::array<std::array<int16_t, 3>, chunks_volume>& getChunksIndices();
    Block getBlockAtPos(int x, int y, int z);
    void update();
    void primary_thread_update();
}

#endif
