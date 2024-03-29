#include "main.hpp"

#include <iostream>
#include <thread>

#define GLOBALS_DEFINER
#include "globals.hpp"
#undef GLOBALS_DEFINER
#include "chunkmanager.hpp"
#include "controls.hpp"
#include "debugwindow.hpp"
#include "renderer.hpp"
#include "shader.hpp"
#include "spacefilling.hpp"

float deltaTime = 0.0f; // Time between current frame and last frame
float lastFrame = 0.0f; // Time of last frame
float lastFPSFrame = 0.0f;
int frames = 0;

int main()
{

    /* Init glfw*/
    glfwInit();
    /* Major version */
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    /* Minor version */
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    /* Use Core profile */
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    // glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);

    /* create window */
    GLFWwindow *window = glfwCreateWindow(800, 600, "LearnOpenGL", NULL, NULL);
    if (window == NULL)
    {
        std::cout << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);
    glfwSwapInterval(0);

    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
    {
        std::cout << "Failed to initialize GLAD" << std::endl;
        return -1;
    }

    glViewport(0, 0, 800, 600);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
    glfwSetCursorPosCallback(window, mouse_callback);
    //glEnable(GL_FRAMEBUFFER_SRGB); //gamma correction done in fragment shader
    //glEnable(GL_CULL_FACE); //GL_BACK GL_CCW by default

    debug::window::set_parameter("gpu_vendor", glGetString(GL_VENDOR));
    debug::window::set_parameter("gpu_renderer", glGetString(GL_RENDERER));

    for(int i = 0; i < 360; i++){
	sines[i] = sin(3.14 / 180 * i);
	cosines[i] = cos(3.14 / 180 * i);
    }

    SpaceFilling::initLUT();
    controls::init();
    chunkmanager::init();
    chunkmesher::init();
    debug::window::init(window);
    renderer::init(window);

    while (!glfwWindowShouldClose(window))
    {
        // DeltaTime
        float currentFrame = glfwGetTime();
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;

	debug::window::set_parameter("frametime", deltaTime);
        // FPS Counter
        frames++;
        if(currentFrame - lastFPSFrame >= 1.0f){
            //std::cout << "FPS: " << frames << " Frametime: " << deltaTime << std::endl;
	    debug::window::set_parameter("fps", frames);
            frames = 0;
            lastFPSFrame = currentFrame;
        }

        glClearColor(0.431f, 0.694f, 1.0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	// Input handling
        // Only close event is handles by main
	if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
	    glfwSetWindowShouldClose(window, true);
	// the rest of input processing is handled by controls.cpp

        // Input processing
	controls::update(window);

        // Camera
        theCamera.update(window, deltaTime);
	debug::window::set_parameter("px", theCamera.getPos().x);
	debug::window::set_parameter("py", theCamera.getPos().y);
	debug::window::set_parameter("pz", theCamera.getPos().z);
	debug::window::set_parameter("cx", (int)(theCamera.getPos().x / CHUNK_SIZE));
	debug::window::set_parameter("cy", (int)(theCamera.getPos().y / CHUNK_SIZE));
	debug::window::set_parameter("cz", (int)(theCamera.getPos().z / CHUNK_SIZE));
	debug::window::set_parameter("lx", theCamera.getFront().x);
	debug::window::set_parameter("ly", theCamera.getFront().y);
	debug::window::set_parameter("lz", theCamera.getFront().z);

	// Render pass
	renderer::render();

        // Swap buffers to avoid tearing
        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    // Stop threads and wait for them to finish
    chunkmanager::stop();

    // Cleanup allocated memory
    chunkmanager::destroy();
    renderer::destroy();

    glfwTerminate();
    return 0;
}

void framebuffer_size_callback(GLFWwindow *window, int width, int height)
{
    glViewport(0, 0, width, height);
    theCamera.viewPortCallBack(window, width, height);
    renderer::framebuffer_size_callback(window, width, height);
}

void mouse_callback(GLFWwindow *window, double xpos, double ypos)
{
    theCamera.mouseCallback(window, xpos, ypos);
}
