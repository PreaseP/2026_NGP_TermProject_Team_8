#pragma once
#include <gl/glew.h>
#include <gl/glm/glm.hpp>
#include <gl/glm/ext.hpp>
#include <gl/glm/gtc/matrix_transform.hpp>
#include <vector>
#include <cmath>

class Snow;
class Map;

class Snowball
{
private:
    GLuint vao, vbo, nbo, tbo;
    GLsizei vertexCount;
    bool isInitialized;

    glm::vec3 position;
    glm::vec3 velocity;
    glm::vec3 acceleration;
    float gravity;
    float radius;
    float lifeTime;
    float maxLifeTime;
    bool isActive;

    void generateSphere(float radius, int segments, std::vector<glm::vec3>& vertices,
        std::vector<glm::vec3>& normals, std::vector<glm::vec2>& texCoords);

    void initializeBuffers();

    bool checkWallCollision(const Map& gameMap) const;
    bool checkSnowCollision(const Snow& snowSystem) const;

public:
    Snowball();
    Snowball(const glm::vec3& startPos, const glm::vec3& direction, float speed = 10.0f, float radius = 0.1f);
    ~Snowball();

    Snowball(const Snowball& other);
    Snowball& operator=(const Snowball& other);

    void update(float deltaTime, Snow& snowSystem, const Map& gameMap);

    void render(GLuint shaderProgram, const glm::vec3& color = glm::vec3(1.0f, 1.0f, 1.0f)) const;

    bool getIsActive() const { return isActive; }
    glm::vec3 getPosition() const { return position; }
    float getRadius() const { return radius; }

    bool checkCollision(const glm::vec3& point, float distance) const;

    void destroy() { isActive = false; }

    static Snowball createFromPlayer(const glm::vec3& playerPos, const glm::vec3& playerFront,
        const glm::vec3& playerUp = glm::vec3(0, 1, 0));
};
