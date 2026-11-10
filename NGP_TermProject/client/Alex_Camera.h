#pragma once
#include <gl/glm/glm.hpp>
#include <gl/glm/gtc/matrix_transform.hpp>

class Alex_Camera
{
private:
    glm::vec3 position;
    glm::vec3 front;
    glm::vec3 up;
    glm::vec3 right;
    glm::vec3 worldUp;
    
    float yaw;
    float pitch;
    
    float movementSpeed;
    float mouseSensitivity;
    float zoom;
    
    void updateCameraVectors();

public:
    Alex_Camera(glm::vec3 position = glm::vec3(0.0f, 0.0f, 0.0f), 
                glm::vec3 up = glm::vec3(0.0f, 1.0f, 0.0f),
                float yaw = -90.0f, float pitch = 0.0f);
    
    void updateFromCharacterPosition(const glm::vec3& characterPosition);
    
    glm::mat4 getViewMatrix() const;
    
    glm::mat4 getProjectionMatrix(float screenWidth, float screenHeight) const;
    
    void processKeyboard(int key, float deltaTime);
    
    void processMouseMovement(float xOffset, float yOffset, bool constrainPitch = true);
    
    void processMouseScroll(float yOffset);
    
    glm::vec3 getPosition() const { return position; }
    glm::vec3 getFront() const { return front; }
    glm::vec3 getUp() const { return up; }
    float getZoom() const { return zoom; }
};

