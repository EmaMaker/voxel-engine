#ifndef CAMERA_H
#define CAMERA_H

#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <iostream>

#include "chunk.hpp"

class Camera
{

public:
    Camera()
    {
        view = glm::mat4(1.0f);
        // note that we're translating the scene in the reverse direction of where we want to move
        projection = glm::perspective(glm::radians(45.0f), 800.0f / 600.0f, 0.1f, 100.0f);
    }

    void update(GLFWwindow *window, float deltaTime)
    {
        const float cameraSpeed = 10.0f * deltaTime; // adjust accordingly
        if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
            this->cameraPos += cameraSpeed * cameraFront;
        if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
            this->cameraPos -= cameraSpeed * cameraFront;
        if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
            this->cameraPos -= glm::normalize(glm::cross(cameraFront, cameraUp)) * cameraSpeed;
        if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
            this->cameraPos += glm::normalize(glm::cross(cameraFront, cameraUp)) * cameraSpeed;

        direction.x = cos(glm::radians(yaw)) * cos(glm::radians(pitch));
        direction.y = sin(glm::radians(pitch));
        direction.z = sin(glm::radians(yaw)) * cos(glm::radians(pitch));
        cameraFront = glm::normalize(direction);

        view = glm::lookAt(cameraPos, cameraPos + cameraFront, cameraUp);
    }

    void viewPortCallBack(GLFWwindow *window, int width, int height)
    {
        projection = glm::perspective(glm::radians(45.0f), (float)width / (float)height, 0.1f, 100.0f);
    }

    void mouseCallback(GLFWwindow *window, double xpos, double ypos)
    {
        float xoffset = xpos - lastX;
        float yoffset = lastY - ypos; // reversed since y-coordinates range from bottom to top
        lastX = xpos;
        lastY = ypos;

        const float sensitivity = 0.1f;
        xoffset *= sensitivity;
        yoffset *= sensitivity;

        yaw += xoffset;
        pitch += yoffset;

        if (pitch > 89.0f)
            pitch = 89.0f;
        if (pitch < -89.0f)
            pitch = -89.0f;
    }

    glm::vec3 getPos()
    {
        return cameraPos;
    }

    glm::vec3 getFront()
    {
        return cameraFront;
    }

    glm::vec3 getUp()
    {
        return cameraUp;
    }

    glm::mat4 getView()
    {
        return view;
    }

    glm::mat4 getProjection()
    {
        return projection;
    }

private:
    glm::vec3 cameraPos = static_cast<float>(CHUNK_SIZE) * glm::vec3(0.0f);
    glm::vec3 cameraFront = glm::vec3(0.0f, 0.0f, -1.0f);
    glm::vec3 cameraUp = glm::vec3(0.0f, 1.0f, 0.0f);
    glm::vec3 direction = glm::vec3(0.0f);

    glm::mat4 view, projection;

    float lastX = 400, lastY = 300;
    float yaw, pitch;
};

#endif