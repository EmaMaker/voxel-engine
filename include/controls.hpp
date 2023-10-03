#ifndef CONTROLS_H
#define CONTROLS_H

#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include "worldupdatemessage.h"

#define BLOCKPICK_TIMEOUT 0.1f

namespace controls{
    void init();
    void update(GLFWwindow* window);

    WorldUpdateMsgQueue& getWorldUpdateQueue();
};

#endif
