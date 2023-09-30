#ifndef CAMERA_H
#define CAMERA_H

#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <atomic>

class Camera
{

public:
    Camera()
    {
        view = glm::mat4(1.0f);

	// This matrix needs to be also updated in viewPortCallback whenever it is changed
        projection = glm::perspective(glm::radians(90.0f), 800.0f / 600.0f, 0.1f, 1200.0f);

	posX = cameraPos.x;
	posY = cameraPos.y;
	posZ = cameraPos.z;
    }

    void update(GLFWwindow *window, float deltaTime)
    {
        const float cameraSpeed = 25.0f * deltaTime; // adjust accordingly

        if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
            this->cameraPos += cameraSpeed * cameraFront;
        if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
            this->cameraPos -= cameraSpeed * cameraFront;
        if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
            this->cameraPos -= glm::normalize(glm::cross(cameraFront, cameraUp)) * cameraSpeed;
        if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
            this->cameraPos += glm::normalize(glm::cross(cameraFront, cameraUp)) * cameraSpeed;
        
        if (glfwGetKey(window, GLFW_KEY_Q) == GLFW_PRESS)
            this->cameraPos += cameraSpeed * cameraUp;
        if (glfwGetKey(window, GLFW_KEY_Z) == GLFW_PRESS)
            this->cameraPos -= cameraSpeed * cameraUp;

	posX = cameraPos.x;
	posY = cameraPos.y;
	posZ = cameraPos.z;

        direction.x = cos(glm::radians(yaw)) * cos(glm::radians(pitch));
        direction.y = sin(glm::radians(pitch));
        direction.z = sin(glm::radians(yaw)) * cos(glm::radians(pitch));
        cameraFront = glm::normalize(direction);

        view = glm::lookAt(cameraPos, cameraPos + cameraFront, cameraUp);
    }

    void viewPortCallBack(GLFWwindow *window, int width, int height)
    {
        projection = glm::perspective(glm::radians(80.0f), (float)width / (float)height, 0.1f, 1200.0f);
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

    glm::vec3 getPos() { return cameraPos; }
    glm::vec3 getFront() { return cameraFront; }
    glm::vec3 getUp() { return cameraUp; }
    glm::mat4 getView() { return view; }
    glm::mat4 getProjection() { return projection; }

    float getAtomicPosX() { return posX; }
    float getAtomicPosY() { return posY; }
    float getAtomicPosZ() { return posZ; }

    // Plane extraction as per Gribb&Hartmann
    // 6 planes, each with 4 components (a,b,c,d)
    void getFrustumPlanes(glm::vec4 planes[6], bool normalize)
    {
	glm::mat4 mat = transpose(projection*view);

	// This just compressed the code below
	float ap = mat[3][0], bp = mat[3][1], cp = mat[3][2], dp = mat[3][3];

	planes[0] = glm::vec4(ap + mat[0][0], bp + mat[0][1], cp + mat[0][2], dp + mat[0][3]);
	planes[1] = glm::vec4(ap - mat[0][0], bp - mat[0][1], cp - mat[0][2], dp - mat[0][3]);
	planes[2] = glm::vec4(ap + mat[1][0], bp + mat[1][1], cp + mat[1][2], dp + mat[1][3]);
	planes[3] = glm::vec4(ap - mat[1][0], bp - mat[1][1], cp - mat[1][2], dp - mat[1][3]);
	planes[4] = glm::vec4(ap + mat[2][0], bp + mat[2][1], cp + mat[2][2], dp + mat[2][3]);
	planes[5] = glm::vec4(ap - mat[2][0], bp - mat[2][1], cp - mat[2][2], dp - mat[2][3]);

	if(normalize)
	for(int i = 0; i < 6; i++){
	    float mag = sqrt(planes[i].x + planes[i].x + planes[i].y * planes[i].y +
		planes[i].z*planes[i].z);

	    planes[i] /= mag;
	}
    }


private:
    glm::vec3 cameraPos = glm::vec3(512.0, 256.0f, 512.0f);
    glm::vec3 cameraFront = glm::vec3(0.0f, 0.0f, -1.0f);
    glm::vec3 cameraUp = glm::vec3(0.0f, 1.0f, 0.0f);
    glm::vec3 direction = glm::vec3(0.0f);

    glm::mat4 view, projection;

    float lastX = 400, lastY = 300;
    float yaw, pitch;

    std::atomic<float> posX, posY, posZ;
};

#endif
