#ifndef DEBUG_WINDOW_H
#define DEBUG_WINDOW_H

#include <any>
#include <string>
#include <GLFW/glfw3.h>

namespace debug{
    namespace window {
	void init(GLFWwindow* window);
	void prerender();
	void render();
	void destroy();

	void set_parameter(std::string key, std::any value);
    }
}

#endif
