#include "Snowball.h"
#include "Snow.h"
#include "Map.h"
#include <iostream>

const float SNOWBALL_BLOCK_SIZE = 1.0f;
const int SNOWBALL_MAP_WIDTH = 10; 
const int SNOWBALL_MAP_DEPTH = 15;

Snowball::Snowball() 
    : vao(0), vbo(0), nbo(0), tbo(0), vertexCount(0), isInitialized(false),
      position(0.0f), velocity(0.0f), acceleration(0.0f), gravity(-9.8f),
      radius(0.1f), lifeTime(0.0f), maxLifeTime(5.0f), isActive(false)
{
}

Snowball::Snowball(const glm::vec3& startPos, const glm::vec3& direction, float speed, float radius)
    : vao(0), vbo(0), nbo(0), tbo(0), vertexCount(0), isInitialized(false),
      position(startPos), gravity(-9.8f), radius(radius), lifeTime(0.0f), 
      maxLifeTime(5.0f), isActive(true)
{
    velocity = glm::normalize(direction) * speed;
    acceleration = glm::vec3(0.0f, gravity, 0.0f);
    
    initializeBuffers();
}

Snowball::~Snowball()
{
    if (isInitialized) {
        glDeleteVertexArrays(1, &vao);
        glDeleteBuffers(1, &vbo);
        glDeleteBuffers(1, &nbo);
        glDeleteBuffers(1, &tbo);
    }
}

Snowball::Snowball(const Snowball& other)
    : vao(0), vbo(0), nbo(0), tbo(0), vertexCount(0), isInitialized(false),
      position(other.position), velocity(other.velocity), acceleration(other.acceleration),
      gravity(other.gravity), radius(other.radius), lifeTime(other.lifeTime),
      maxLifeTime(other.maxLifeTime), isActive(other.isActive)
{
    if (other.isInitialized) {
        initializeBuffers();
    }
}

Snowball& Snowball::operator=(const Snowball& other)
{
    if (this != &other) {
        if (isInitialized) {
            glDeleteVertexArrays(1, &vao);
            glDeleteBuffers(1, &vbo);
            glDeleteBuffers(1, &nbo);
            glDeleteBuffers(1, &tbo);
            isInitialized = false;
        }
        
        position = other.position;
        velocity = other.velocity;
        acceleration = other.acceleration;
        gravity = other.gravity;
        radius = other.radius;
        lifeTime = other.lifeTime;
        maxLifeTime = other.maxLifeTime;
        isActive = other.isActive;
        
        vao = vbo = nbo = tbo = 0;
        vertexCount = 0;
        
        if (other.isInitialized) {
            initializeBuffers();
        }
    }
    return *this;
}

void Snowball::generateSphere(float radius, int segments, std::vector<glm::vec3>& vertices, 
                             std::vector<glm::vec3>& normals, std::vector<glm::vec2>& texCoords)
{
    vertices.clear();
    normals.clear();
    texCoords.clear();

    const float PI = 3.14159265359f;

    for (int i = 0; i <= segments; ++i) {
        float lat = static_cast<float>(i) / segments * PI - PI / 2.0f;
        float cosLat = cos(lat);
        float sinLat = sin(lat);

        for (int j = 0; j <= segments; ++j) {
            float lon = static_cast<float>(j) / segments * 2.0f * PI;
            float cosLon = cos(lon);
            float sinLon = sin(lon);

            glm::vec3 vertex(radius * cosLat * cosLon, radius * sinLat, radius * cosLat * sinLon);
            glm::vec3 normal = glm::normalize(vertex);
            glm::vec2 texCoord(static_cast<float>(j) / segments, static_cast<float>(i) / segments);

            vertices.push_back(vertex);
            normals.push_back(normal);
            texCoords.push_back(texCoord);
        }
    }

    std::vector<glm::vec3> finalVertices, finalNormals;
    std::vector<glm::vec2> finalTexCoords;

    for (int i = 0; i < segments; ++i) {
        for (int j = 0; j < segments; ++j) {
            int current = i * (segments + 1) + j;
            int next = current + segments + 1;

            // √π π¯¬∞ ªÔ∞¢«¸
            finalVertices.push_back(vertices[current]);
            finalVertices.push_back(vertices[next]);
            finalVertices.push_back(vertices[current + 1]);
            
            finalNormals.push_back(normals[current]);
            finalNormals.push_back(normals[next]);
            finalNormals.push_back(normals[current + 1]);
            
            finalTexCoords.push_back(texCoords[current]);
            finalTexCoords.push_back(texCoords[next]);
            finalTexCoords.push_back(texCoords[current + 1]);

            // µŒ π¯¬∞ ªÔ∞¢«¸
            finalVertices.push_back(vertices[next]);
            finalVertices.push_back(vertices[next + 1]);
            finalVertices.push_back(vertices[current + 1]);
            
            finalNormals.push_back(normals[next]);
            finalNormals.push_back(normals[next + 1]);
            finalNormals.push_back(normals[current + 1]);
            
            finalTexCoords.push_back(texCoords[next]);
            finalTexCoords.push_back(texCoords[next + 1]);
            finalTexCoords.push_back(texCoords[current + 1]);
        }
    }

    vertices = finalVertices;
    normals = finalNormals;
    texCoords = finalTexCoords;
}

void Snowball::initializeBuffers()
{
    std::vector<glm::vec3> vertices, normals;
    std::vector<glm::vec2> texCoords;
    
    generateSphere(radius, 16, vertices, normals, texCoords);
    vertexCount = static_cast<GLsizei>(vertices.size());

    // OpenGL πˆ∆€ ª˝º∫
    glGenVertexArrays(1, &vao);
    glGenBuffers(1, &vbo);
    glGenBuffers(1, &nbo);
    glGenBuffers(1, &tbo);

    glBindVertexArray(vao);

    // πˆ≈ÿΩ∫ ¿ßƒ° µ•¿Ã≈Õ
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(glm::vec3), vertices.data(), GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(glm::vec3), (void*)0);

    // π˝º± µ•¿Ã≈Õ
    glBindBuffer(GL_ARRAY_BUFFER, nbo);
    glBufferData(GL_ARRAY_BUFFER, normals.size() * sizeof(glm::vec3), normals.data(), GL_STATIC_DRAW);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(glm::vec3), (void*)0);

    // ≈ÿΩ∫√≥ ¡¬«• µ•¿Ã≈Õ
    glBindBuffer(GL_ARRAY_BUFFER, tbo);
    glBufferData(GL_ARRAY_BUFFER, texCoords.size() * sizeof(glm::vec2), texCoords.data(), GL_STATIC_DRAW);
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(glm::vec2), (void*)0);

    glBindVertexArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    isInitialized = true;
}

bool Snowball::checkSnowCollision(const Snow& snowSystem) const
{
    int gridX = static_cast<int>(std::floor(position.x / SNOWBALL_BLOCK_SIZE));
    int gridZ = static_cast<int>(std::floor(position.z / SNOWBALL_BLOCK_SIZE));
    
    for (int dx = -1; dx <= 1; ++dx) {
        for (int dz = -1; dz <= 1; ++dz) {
            int checkGridX = gridX + dx;
            int checkGridZ = gridZ + dz;
            
            float snowHeight = snowSystem.getSnowHeightAt(checkGridX, checkGridZ);
            if (snowHeight > 0.0f) {
                float snowWorldX = checkGridX * SNOWBALL_BLOCK_SIZE;
                float snowWorldZ = checkGridZ * SNOWBALL_BLOCK_SIZE;
                
                float snowCenterX = snowWorldX + SNOWBALL_BLOCK_SIZE / 2.0f;
                float snowCenterZ = snowWorldZ + SNOWBALL_BLOCK_SIZE / 2.0f;
                
                const float GROUND_TOP = 0.5f;
                float snowBottomY = GROUND_TOP;
                float snowTopY = GROUND_TOP + snowHeight;
                float snowCenterY = GROUND_TOP + snowHeight / 2.0f;
                
                float distanceX = abs(position.x - snowCenterX);
                float distanceZ = abs(position.z - snowCenterZ);
                float distanceY = abs(position.y - snowCenterY);
                
                float blockHalfWidth = SNOWBALL_BLOCK_SIZE / 2.0f;
                float blockHalfHeight = snowHeight / 2.0f;
                
                bool collisionX = distanceX <= (blockHalfWidth + radius);
                bool collisionZ = distanceZ <= (blockHalfWidth + radius);
                bool collisionY = distanceY <= (blockHalfHeight + radius);
                
                if (collisionX && collisionZ && collisionY) {
                    std::cout << "=== ¥´ √Êµπ ∞®¡ˆ ===" << std::endl;
                    std::cout << "¥´µ¢¿Ã ¿ßƒ°: (" << position.x << ", " << position.y << ", " << position.z << ")" << std::endl;
                    std::cout << "∞›¿⁄(" << checkGridX << ", " << checkGridZ << ") ¥´ ≥Ù¿Ã: " << snowHeight << std::endl;
                    std::cout << "¥´ ∫Ì∑œ ¡ﬂΩ…: (" << snowCenterX << ", " << snowCenterY << ", " << snowCenterZ << ")" << std::endl;
                    std::cout << "¥´ ∫Ì∑œ π¸¿ß Y: " << snowBottomY << " ~ " << snowTopY << std::endl;
                    std::cout << "∞≈∏Æ X/Z/Y: " << distanceX << "/" << distanceZ << "/" << distanceY << std::endl;
                    return true;
                }
            }
        }
    }
    return false;
}

void Snowball::update(float deltaTime, Snow& snowSystem, const Map& gameMap)
{
    if (!isActive) return;

    lifeTime += deltaTime;
    if (lifeTime >= maxLifeTime) {
        isActive = false;
        std::cout << "¥´µ¢¿Ã ºˆ∏Ì ∏∏∑·" << std::endl;
        return;
    }

    glm::vec3 prevPosition = position;

    velocity += acceleration * deltaTime;
    position += velocity * deltaTime;

    static int debugCounter = 0;
    if (++debugCounter % 30 == 0) {
        std::cout << "¥´µ¢¿Ã ¿ßƒ°: (" << position.x << ", " << position.y << ", " << position.z << ")" << std::endl;
    }

    if (checkWallCollision(gameMap)) {
        glm::vec3 collisionPoint = position;
        
        float snowX = collisionPoint.x;
        float snowZ = collisionPoint.z;
        
        if (snowX < 0) snowX = 0.5f * SNOWBALL_BLOCK_SIZE;
        if (snowX >= SNOWBALL_MAP_WIDTH * SNOWBALL_BLOCK_SIZE) snowX = (SNOWBALL_MAP_WIDTH - 0.5f) * SNOWBALL_BLOCK_SIZE;
        
        if (snowZ < 0) {
            snowZ = 0.5f * SNOWBALL_BLOCK_SIZE;
        } else if (snowZ >= 5.0f * SNOWBALL_BLOCK_SIZE && snowZ < 10.0f * SNOWBALL_BLOCK_SIZE) {
            if (snowZ < 7.5f * SNOWBALL_BLOCK_SIZE) {
                snowZ = (5.0f - 0.5f) * SNOWBALL_BLOCK_SIZE;
            } else {
                snowZ = 10.5f * SNOWBALL_BLOCK_SIZE;
            }
        } else if (snowZ >= SNOWBALL_MAP_DEPTH * SNOWBALL_BLOCK_SIZE) {
            snowZ = (SNOWBALL_MAP_DEPTH - 0.5f) * SNOWBALL_BLOCK_SIZE;
        }
        
        snowSystem.addSnowAt(snowX, snowZ);
        
        isActive = false;
        std::cout << "¥´µ¢¿Ã∞° ∫Æø° √Êµπ! ¡∂¡§µ» ¿ßƒ°(" << snowX << ", " << snowZ << ")ø° ¥´ ª˝º∫" << std::endl;
        return;
    }

    if (checkSnowCollision(snowSystem)) {
        snowSystem.addSnowAt(position.x, position.z);
        
        isActive = false;
        std::cout << "¥´µ¢¿Ã∞° ±‚¡∏ ¥´ø° √Êµπ! ¿ßƒ°(" << position.x << ", " << position.z << ")ø° ¥´ √ﬂ∞°" << std::endl;
        return;
    }

    const float GROUND_TOP = 0.5f;
    if (position.y - radius <= GROUND_TOP) {
        isActive = false;
        std::cout << "¥´µ¢¿Ã∞° Groundø° ¬¯¡ˆ«œø© ªÁ∂Û¡¸ (¥´ ª˝º∫ æ»«‘)" << std::endl;
        return;
    }

    if (position.x < -2.0f || position.x > (SNOWBALL_MAP_WIDTH + 2) * SNOWBALL_BLOCK_SIZE || 
        position.z < -2.0f || position.z > (SNOWBALL_MAP_DEPTH + 2) * SNOWBALL_BLOCK_SIZE) {
        isActive = false;
        std::cout << "¥´µ¢¿Ã∞° ∏  øµø™¿ª π˛æÓ≥™º≠ ªÁ∂Û¡¸" << std::endl;
        return;
    }
}

bool Snowball::checkWallCollision(const Map& gameMap) const
{
    const Wall& wall = gameMap.getWall();
    
    static bool debugPrinted = false;
    if (!debugPrinted) {
        std::cout << "√— ∫Æ ∫Ì∑œ ∞≥ºˆ: " << wall.getBlockCount() << std::endl;
        debugPrinted = true;
    }
    
    for (size_t i = 0; i < wall.getBlockCount(); ++i) {
        const Block& block = wall.getBlock(i);
        
        glm::vec3 blockPos(block.getX(), block.getY(), block.getZ());
        float blockSize = block.getSize();
        
        float collisionMargin = radius + 0.1f;
        
        glm::vec3 blockMin = blockPos - glm::vec3(blockSize / 2.0f + collisionMargin);
        glm::vec3 blockMax = blockPos + glm::vec3(blockSize / 2.0f + collisionMargin);
        
        if (position.x >= blockMin.x && position.x <= blockMax.x &&
            position.y >= blockMin.y && position.y <= blockMax.y &&
            position.z >= blockMin.z && position.z <= blockMax.z) {
            
            std::cout << "∫Æ √Êµπ ∞®¡ˆ!" << std::endl;
            std::cout << "¥´µ¢¿Ã ¿ßƒ°: (" << position.x << ", " << position.y << ", " << position.z << ")" << std::endl;
            std::cout << "∫Ì∑œ ¿ßƒ°: (" << blockPos.x << ", " << blockPos.y << ", " << blockPos.z << ")" << std::endl;
            std::cout << "∫Ì∑œ ≈©±‚: " << blockSize << std::endl;
            return true;
        }
    }
    
    return false;
}

void Snowball::render(GLuint shaderProgram, const glm::vec3& color) const
{
    if (!isActive || !isInitialized) return;

    // ∏µ® «‡∑ƒ
    glm::mat4 model = glm::translate(glm::mat4(1.0f), position);
    
    // π˝º± «‡∑ƒ
    glm::mat3 normalMatrix = glm::mat3(glm::transpose(glm::inverse(model)));
    
    GLint modelLoc = glGetUniformLocation(shaderProgram, "model");
    GLint normalMatrixLoc = glGetUniformLocation(shaderProgram, "normalMatrix");
    GLint vColorLoc = glGetUniformLocation(shaderProgram, "vColor");
    GLint useTextureLoc = glGetUniformLocation(shaderProgram, "useTexture");
    
    if (modelLoc != -1) {
        glUniformMatrix4fv(modelLoc, 1, GL_FALSE, &model[0][0]);
    }
    if (normalMatrixLoc != -1) {
        glUniformMatrix3fv(normalMatrixLoc, 1, GL_FALSE, &normalMatrix[0][0]);
    }
    if (vColorLoc != -1) {
        glUniform3f(vColorLoc, color.r, color.g, color.b);
    }
    if (useTextureLoc != -1) {
        glUniform1i(useTextureLoc, 0);
    }

    glBindVertexArray(vao);
    glDrawArrays(GL_TRIANGLES, 0, vertexCount);
    glBindVertexArray(0);
}

bool Snowball::checkCollision(const glm::vec3& point, float distance) const
{
    if (!isActive) return false;
    
    float dist = glm::distance(position, point);
    return dist <= (radius + distance);
}

Snowball Snowball::createFromPlayer(const glm::vec3& playerPos, const glm::vec3& playerFront, const glm::vec3& playerUp)
{
    glm::vec3 startPos = playerPos + playerFront * 1.0f + playerUp * 0.5f;
    
    glm::vec3 direction = playerFront + playerUp * 0.3f;
    
    std::cout << "¥´µ¢¿Ã ª˝º∫ - Ω√¿€ ¿ßƒ°: (" << startPos.x << ", " << startPos.y << ", " << startPos.z 
              << ") πÊ«‚: (" << direction.x << ", " << direction.y << ", " << direction.z << ")" << std::endl;
    
    return Snowball(startPos, direction, 12.0f, 0.15f);
}
