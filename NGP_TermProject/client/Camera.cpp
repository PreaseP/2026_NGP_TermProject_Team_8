#include "Camera.h"
#include <iostream>
#include <algorithm>
#include <cmath>
#include <gl/freeglut.h>

Camera::Camera(glm::vec3 pos, glm::vec3 worldUpVec, float yawAngle, float pitchAngle)
    : position(pos)
    , worldUp(worldUpVec)
    , yaw(yawAngle)
    , pitch(pitchAngle)
    , movementSpeed(5.0f)
    , mouseSensitivity(0.1f)
    , fov(45.0f)
{
    updateCameraVectors();
}

glm::mat4 Camera::getViewMatrix() const
{
    return glm::lookAt(position, position + front, up);
}

glm::mat4 Camera::getProjectionMatrix(float screenWidth, float screenHeight) const
{
    float aspect = screenWidth / screenHeight;
    return glm::perspective(glm::radians(fov), aspect, 0.1f, 100.0f);
}

void Camera::processSpecialKeyboard(int key)
{
    float rotationSpeed = 2.0f;

    switch (key) {
    case GLUT_KEY_UP:
        pitch += rotationSpeed;
        if (pitch > 89.0f) pitch = 89.0f;
        break;
    case GLUT_KEY_DOWN:
        pitch -= rotationSpeed;
        if (pitch < -89.0f) pitch = -89.0f;
        break;
    case GLUT_KEY_LEFT:
        yaw -= rotationSpeed;
        break;
    case GLUT_KEY_RIGHT:
        yaw += rotationSpeed;
        break;
    }

    updateCameraVectors();
}

void Camera::processMouseScroll(int direction)
{
    float scrollSpeed = 0.5f;
    
    if (direction > 0) { // 휠 위로
        position += front * scrollSpeed;
    } else if (direction < 0) { // 휠 아래로
        position -= front * scrollSpeed;
    }
}

void Camera::processMouseMovement(float xOffset, float yOffset, bool constrainPitch)
{
    xOffset *= mouseSensitivity;
    yOffset *= mouseSensitivity;

    yaw += xOffset;
    pitch += yOffset;

    if (constrainPitch) {
        if (pitch > 89.0f) pitch = 89.0f;
        if (pitch < -89.0f) pitch = -89.0f;
    }

    updateCameraVectors();
}

void Camera::adjustSpeed(float speedDelta)
{
    movementSpeed = std::max(1.0f, std::min(20.0f, movementSpeed + speedDelta));
    std::cout << "카메라 속도: " << movementSpeed << std::endl;
}

void Camera::reset()
{
    position = glm::vec3(5.0f, 8.0f, 12.0f);
    yaw = -90.0f;
    pitch = -20.0f;
    movementSpeed = 5.0f;
    fov = 45.0f;
    updateCameraVectors();
    std::cout << "카메라 위치 리셋" << std::endl;
}

void Camera::setPosition(const glm::vec3& pos)
{
    position = pos;
}

void Camera::setAngles(float yawAngle, float pitchAngle)
{
    yaw = yawAngle;
    pitch = pitchAngle;
    updateCameraVectors();
}

void Camera::updateCameraVectors()
{
    glm::vec3 frontVec;
    frontVec.x = cos(glm::radians(yaw)) * cos(glm::radians(pitch));
    frontVec.y = sin(glm::radians(pitch));
    frontVec.z = sin(glm::radians(yaw)) * cos(glm::radians(pitch));
    front = glm::normalize(frontVec);

    right = glm::normalize(glm::cross(front, worldUp));
    up = glm::normalize(glm::cross(right, front));
}
