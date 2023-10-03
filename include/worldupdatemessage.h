#ifndef WORLD_UPDATE_MSG_H
#define WORLD_UPDATE_MSG_H

#include <glm/glm.hpp>
#include <oneapi/tbb/concurrent_queue.h>

enum class WorldUpdateMsgType{
    BLOCKPICK_PLACE,
    BLOCKPICK_BREAK
};

typedef struct WorldUpdateMsg{
    WorldUpdateMsgType msg_type;
    glm::vec3 cameraPos;
    glm::vec3 cameraFront;
    float time;
} WorldUpdateMsg;

typedef oneapi::tbb::concurrent_queue<WorldUpdateMsg> WorldUpdateMsgQueue;

#endif
