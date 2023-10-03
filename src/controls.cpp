#include "controls.hpp"
#include "renderer.hpp"

namespace controls{
    float lastBlockPick=0.0;
    bool blockpick = false;
    bool cursor = false;

    void init(){
    }
    
    void update(GLFWwindow* window){
	float current_time = glfwGetTime();

	// Reset blockpicking timeout has passed
	if(current_time - lastBlockPick > BLOCKPICK_TIMEOUT) blockpick = false;
	// Reset blockpicking if both mouse buttons are released
	if(glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_1) == GLFW_RELEASE && glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_2) == GLFW_RELEASE) blockpick = false;

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

	if(glfwGetKey(window, GLFW_KEY_F2) == GLFW_PRESS) renderer::saveScreenshot();
	if(glfwGetKey(window, GLFW_KEY_F3) == GLFW_PRESS) renderer::saveScreenshot(true);
	if(glfwGetKey(window, GLFW_KEY_M) == GLFW_PRESS) {
	    cursor = !cursor;
	    glfwSetInputMode(window, GLFW_CURSOR, cursor ? GLFW_CURSOR_NORMAL : GLFW_CURSOR_DISABLED);
	}
    }
};
