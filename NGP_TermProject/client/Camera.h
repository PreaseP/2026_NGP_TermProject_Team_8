#pragma once

#include <gl/glm/glm.hpp>
#include <gl/glm/gtc/matrix_transform.hpp>

class Camera {
public:
    glm::vec3 position;
    glm::vec3 front;
    glm::vec3 up;
    glm::vec3 right;
    glm::vec3 worldUp;

    float yaw;
    float pitch;

    float movementSpeed;
    float mouseSensitivity;
    float fov;

    Camera(glm::vec3 pos = glm::vec3(3.0f, 18.0f, 7.0f), 
           glm::vec3 worldUpVec = glm::vec3(0.0f, 1.0f, 0.0f),
           float yawAngle = 0.0f, 
           float pitchAngle = -85.0f);

    glm::mat4 getViewMatrix() const;

    glm::mat4 getProjectionMatrix(float screenWidth, float screenHeight) const;

    void processKeyboard(char key, float deltaTime);

    void processSpecialKeyboard(int key);

    void processMouseScroll(int direction);

    void processMouseMovement(float xOffset, float yOffset, bool constrainPitch = true);

    void adjustSpeed(float speedDelta);

    void reset();

    void setPosition(const glm::vec3& pos);
    void setAngles(float yawAngle, float pitchAngle);

private:
    void updateCameraVectors();
};

