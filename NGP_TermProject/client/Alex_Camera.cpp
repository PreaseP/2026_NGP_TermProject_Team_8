#include "Alex_Camera.h"
#include <gl/freeglut.h>
#include <algorithm>

const float YAW = -90.0f;
const float PITCH = 0.0f;
const float SPEED = 2.5f;
const float SENSITIVITY = 0.1f;
const float ZOOM = 45.0f;

Alex_Camera::Alex_Camera(glm::vec3 position, glm::vec3 up, float yaw, float pitch)
    : front(glm::vec3(0.0f, 0.0f, -1.0f)), movementSpeed(SPEED), mouseSensitivity(SENSITIVITY), zoom(ZOOM)
{
    this->position = position;
    this->worldUp = up;
    this->yaw = yaw;
    this->pitch = pitch;
    updateCameraVectors();
}

void Alex_Camera::updateFromCharacterPosition(const glm::vec3& characterPosition)
{
    // 캐릭터의 머리 부분에 카메라 위치 설정
    position = characterPosition + glm::vec3(0.0f, 0.8f, 0.0f);
}

glm::mat4 Alex_Camera::getViewMatrix() const
{
    return glm::lookAt(position, position + front, up);
}

glm::mat4 Alex_Camera::getProjectionMatrix(float screenWidth, float screenHeight) const
{
    float aspectRatio = screenWidth / screenHeight;
    return glm::perspective(glm::radians(zoom), aspectRatio, 0.1f, 100.0f);
}

void Alex_Camera::processKeyboard(int key, float deltaTime)
{
    float velocity = movementSpeed * deltaTime;
    
    switch(key) {
        case GLUT_KEY_UP:
            pitch += velocity * 10.0f;
            break;
        case GLUT_KEY_DOWN:
            pitch -= velocity * 10.0f;
            break;
        case GLUT_KEY_LEFT:
            yaw -= velocity * 10.0f;
            break;
        case GLUT_KEY_RIGHT:
            yaw += velocity * 10.0f;
            break;
    }
    
    if (pitch > 89.0f)
        pitch = 89.0f;
    if (pitch < -89.0f)
        pitch = -89.0f;
    
    updateCameraVectors();
}

void Alex_Camera::processMouseMovement(float xOffset, float yOffset, bool constrainPitch)
{
    xOffset *= mouseSensitivity;
    yOffset *= mouseSensitivity;

    yaw += xOffset;
    pitch += yOffset;

    if (constrainPitch) {
        if (pitch > 89.0f)
            pitch = 89.0f;
        if (pitch < -89.0f)
            pitch = -89.0f;
    }

    updateCameraVectors();
}

void Alex_Camera::processMouseScroll(float yOffset)
{
    zoom -= (float)yOffset;
    if (zoom < 1.0f)
        zoom = 1.0f;
    if (zoom > 45.0f)
        zoom = 45.0f;
}

void Alex_Camera::updateCameraVectors()
{
    glm::vec3 newFront;
    newFront.x = cos(glm::radians(yaw)) * cos(glm::radians(pitch));
    newFront.y = sin(glm::radians(pitch));
    newFront.z = sin(glm::radians(yaw)) * cos(glm::radians(pitch));
    front = glm::normalize(newFront);
    
    right = glm::normalize(glm::cross(front, worldUp));
    up = glm::normalize(glm::cross(right, front));
}
