#pragma once
#include <gl/glew.h>
#include <gl/glm/glm.hpp>
#include <gl/glm/ext.hpp>
#include <gl/glm/gtc/matrix_transform.hpp>
#include <map>
#include <vector>
#include <string>

struct SnowAnimationData {
    float targetHeight;
    float currentHeight;
    float animationTime;
    float animationDuration;
    bool isAnimating;
    float alphaValue;

    SnowAnimationData() : targetHeight(0.0f), currentHeight(0.0f),
        animationTime(0.0f), animationDuration(1.0f),
        isAnimating(false), alphaValue(0.0f) {
    }
};

class Snow
{
private:
    std::map<std::pair<int, int>, SnowAnimationData> snowData;

    mutable GLuint vao = 0;
    mutable GLuint vbo = 0;
    mutable GLuint nbo = 0;
    mutable GLuint tbo = 0;
    mutable GLuint alphaVBO = 0;
    mutable GLsizei totalVertexCount = 0;
    mutable bool needsUpdate = true;

    mutable GLuint textureID = 0;
    mutable bool textureLoaded = false;
    std::string texturePath;

    float defaultAnimationDuration = 1.5f;
    float growthRate = 2.0f;

    std::pair<int, int> worldToGrid(float x, float z) const;
    glm::vec3 gridToWorld(int gridX, int gridZ, float height) const;

    void updateBuffers() const;
    void generateSnowBlock(float x, float y, float z, float width, float height, float depth,
        std::vector<glm::vec3>& vertices, std::vector<glm::vec3>& normals,
        std::vector<glm::vec2>& texCoords, std::vector<float>& alphas,
        float alpha) const;

    void loadTexture() const;

    bool canSnowBeGenerated(int gridX, int gridZ) const;
    bool isAdjacentToWall(int gridX, int gridZ) const;
    bool isAdjacentToExistingSnow(int gridX, int gridZ) const;
    bool isAdjacentToMaxHeightSnow(int gridX, int gridZ) const;

    float easeOutQuart(float t) const;
    float easeInOut(float t) const;

public:
    Snow(const std::string& texturePath = "snow.png");
    ~Snow();

    // [OnExit 6] Release GL resources before the window is destroyed. Safe to repeat.
    void cleanup();

    void addSnowAt(float x, float z);

    void updateAnimations(float deltaTime);

    float getSnowHeightAt(int gridX, int gridZ) const;

    float getSnowHeightAtWorld(float x, float z) const;

    void render(GLuint shaderProgram) const;

    void clearAll();

    bool isValidGroundPosition(float x, float z) const;

    void setAnimationDuration(float duration) { defaultAnimationDuration = duration; }
    void setGrowthRate(float rate) { growthRate = rate; }
};
