#pragma once
#include <gl/glew.h>
#include <gl/glm/glm.hpp>
#include <vector>

enum class LightType {
    DIRECTIONAL = 0,
    POINT = 1,
    SPOT = 2
};

struct Light {
    LightType type;
    glm::vec3 position;
    glm::vec3 direction;
    glm::vec3 color;
    float intensity;
    
    float constant;
    float linear;
    float quadratic;
    
    float cutOff;
    float outerCutOff;
    
    Light();
    
    Light(const glm::vec3& direction, const glm::vec3& color, float intensity = 1.0f);
    
    Light(const glm::vec3& position, const glm::vec3& color, float intensity = 1.0f, 
          float constant = 1.0f, float linear = 0.09f, float quadratic = 0.032f);
    
    Light(const glm::vec3& position, const glm::vec3& direction, const glm::vec3& color, 
          float intensity = 1.0f, float cutOff = 12.5f, float outerCutOff = 17.5f);
};

class LightManager {
private:
    std::vector<Light> lights;
    glm::vec3 ambientColor;
    float ambientStrength;
    
public:
    LightManager();
    
    void addLight(const Light& light);
    void removeLight(size_t index);
    void clearLights();
    
    void setAmbientLight(const glm::vec3& color, float strength);
    
    Light& getLight(size_t index);
    const Light& getLight(size_t index) const;
    size_t getLightCount() const;
    
    void updateLightPosition(size_t index, const glm::vec3& newPos);
    void updateLightDirection(size_t index, const glm::vec3& newDir);
    
    void applyLighting(GLuint shaderProgram, const glm::vec3& viewPos) const;
    
    void setupDefaultLighting();
    void setupCameraFollowLight(const glm::vec3& cameraPos);
    void setupSceneStaticLight();
};

