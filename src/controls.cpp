#include "controls.hpp"

#include "camera.hpp"
#include "chunkmanager.hpp"
#include "globals.hpp"
#include "renderer.hpp"

namespace controls{
    WorldUpdateMsgQueue MsgQueue;
    float lastBlockPick=0.0;
    bool blockpick = false;
    bool cursor = false;

    void init(){
    }
    
    void update(GLFWwindow* window){
	// Reset blockping timeout have passed
	float current_time = glfwGetTime();

	/* BlockPicking */
	// Reset blockpicking if enough time has passed
	if(current_time - lastBlockPick > BLOCKPICK_TIMEOUT) blockpick = false;
	// Reset blockpicking if both mouse buttons are released
	if(glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_1) == GLFW_RELEASE && glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_2) == GLFW_RELEASE) blockpick = false;
	// Process block picking if a mouse button is pressed
	if((glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_1) == GLFW_PRESS ||
		glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_2 == GLFW_PRESS)) && !blockpick){

	    // Start timeout for next block pick action
	    blockpick =  true;
	    lastBlockPick = current_time;

	    // Construct the message to send to chunkmanager

	    // WorldUpdateMsg is allocated on the stack
	    // unlike ChunkMeshData, the fields of WorldUpdateMsg are few and light, so there's no
	    // problem in passing them by value each time.
	    // It also has the advantage of having less memory to manage, since I'm not allocating
	    // anything on the heap

	    WorldUpdateMsg msg{};
	    msg.cameraPos = theCamera.getPos();
	    msg.cameraFront = theCamera.getFront();
	    msg.time = current_time;
	    msg.msg_type = glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_1) == GLFW_PRESS ?
	    WorldUpdateMsgType::BLOCKPICK_PLACE : WorldUpdateMsgType::BLOCKPICK_BREAK;

	    // Send to chunk manager
	    chunkmanager::getWorldUpdateQueue().push(msg);
	}

	/* SCREENSHOTS */
	if(glfwGetKey(window, GLFW_KEY_F2) == GLFW_PRESS) renderer::saveScreenshot();
	if(glfwGetKey(window, GLFW_KEY_F3) == GLFW_PRESS) renderer::saveScreenshot(true);
	if(glfwGetKey(window, GLFW_KEY_M) == GLFW_PRESS) {
	    cursor = !cursor;
	    glfwSetInputMode(window, GLFW_CURSOR, cursor ? GLFW_CURSOR_NORMAL : GLFW_CURSOR_DISABLED);
	}
    }

    WorldUpdateMsgQueue& getWorldUpdateQueue(){ return MsgQueue; }
};
