#include "controls.hpp"
#include "renderer.hpp"

namespace controls{
    WorldUpdateMsgQueue MsgQueue;
    float lastBlockPick=0.0;
    bool blockpick = false;
    bool cursor = false;

    void init(){
    }
    
    void update(GLFWwindow* window){
	// Reset blockping timeout if 200ms have passed
	if(glfwGetTime() - lastBlockPick > 0.1) blockpick = false;

	if(glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_2) == GLFW_PRESS && !blockpick){
	    //chunkmanager::blockpick(false);
	    blockpick=true;
	    lastBlockPick=glfwGetTime();
	}

	if(glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_1) == GLFW_PRESS && !blockpick){
	    //chunkmanager::blockpick(true);
	    blockpick=true;
	    lastBlockPick=glfwGetTime();
	}

	// Reset blockpicking if enough time has passed
	if(glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_1) == GLFW_RELEASE && glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_2) == GLFW_RELEASE) blockpick = false;

	if(glfwGetKey(window, GLFW_KEY_F2) == GLFW_PRESS) renderer::saveScreenshot();
	if(glfwGetKey(window, GLFW_KEY_F3) == GLFW_PRESS) renderer::saveScreenshot(true);
	if(glfwGetKey(window, GLFW_KEY_M) == GLFW_PRESS) {
	    cursor = !cursor;
	    glfwSetInputMode(window, GLFW_CURSOR, cursor ? GLFW_CURSOR_NORMAL : GLFW_CURSOR_DISABLED);
	}
    }

    WorldUpdateMsgQueue& getWorldUpdateQueue(){ return MsgQueue; }
};
