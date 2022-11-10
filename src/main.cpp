#include <glad/glad.h>
#include <GLFW/glfw3.h>

#define GLOBALS_DEFINER
#include "globals.hpp"
#undef GLOBALS_DEFINER

#include <iostream>

#include "chunkmanager.hpp"
#include "main.hpp"
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

    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
    {
        std::cout << "Failed to initialize GLAD" << std::endl;
        return -1;
    }

    glViewport(0, 0, 800, 600);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
    glfwSetCursorPosCallback(window, mouse_callback);
    glEnable(GL_DEPTH_TEST);

    // SpaceFilling::initLUT();
    chunkmanager::init();

    while (!glfwWindowShouldClose(window))
    {
        // DeltaTime
        float currentFrame = glfwGetTime();
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;

        // FPS Counter
        // frames++;
        // if(currentFrame - lastFPSFrame >= 1.0f){
        //     std::cout << "FPS: " << frames << std::endl;
        //     frames = 0;
        //     lastFPSFrame = currentFrame;
        // }

        glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // Input processing
        processInput(window);

        // Camera
        theCamera.update(window, deltaTime);

        // ChunkManager
        chunkmanager::update(deltaTime);

        // Swap buffers to avoid tearing
        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    glfwTerminate();
    return 0;
}

void framebuffer_size_callback(GLFWwindow *window, int width, int height)
{
    glViewport(0, 0, width, height);
    theCamera.viewPortCallBack(window, width, height);
}

void mouse_callback(GLFWwindow *window, double xpos, double ypos)
{
    theCamera.mouseCallback(window, xpos, ypos);
}

void processInput(GLFWwindow *window)
{
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);
}