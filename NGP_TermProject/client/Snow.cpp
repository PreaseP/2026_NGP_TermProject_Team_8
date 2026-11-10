#include "Snow.h"
#include "Map.h"
#include "stb_image.h"
#include <iostream>
#include <cmath>
#include <algorithm>

Snow::Snow(const std::string& texturePath) : texturePath(texturePath)
{
}

Snow::~Snow()
{
    cleanup();
}

// [OnExit 6] Shared by OnExit and the destructor; reset IDs to prevent double deletion.
void Snow::cleanup()
{
    if (vao != 0) {
        glDeleteVertexArrays(1, &vao);
        glDeleteBuffers(1, &vbo);
        glDeleteBuffers(1, &nbo);
        glDeleteBuffers(1, &tbo);
        if (alphaVBO != 0) glDeleteBuffers(1, &alphaVBO);
    }

    if (textureLoaded && textureID != 0) {
        glDeleteTextures(1, &textureID);
    }
    vao = vbo = nbo = tbo = alphaVBO = textureID = 0;
    textureLoaded = false;
    totalVertexCount = 0;
    snowData.clear();
    needsUpdate = true;
}

void Snow::loadTexture() const
{
    if (textureLoaded) return;

    glGenTextures(1, &textureID);
    glBindTexture(GL_TEXTURE_2D, textureID);

    // 텍스처 파라미터 설정
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    int width, height, channels;
    stbi_set_flip_vertically_on_load(true);
    unsigned char* data = stbi_load(texturePath.c_str(), &width, &height, &channels, 0);

    if (data) {
        GLenum format;
        if (channels == 1)
            format = GL_RED;
        else if (channels == 3)
            format = GL_RGB;
        else if (channels == 4)
            format = GL_RGBA;
        else
            format = GL_RGB;

        glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);
        glGenerateMipmap(GL_TEXTURE_2D);

        std::cout << "눈 텍스처 로드 성공: " << texturePath << " (" << width << "x" << height << ", " << channels << " channels)" << std::endl;
        textureLoaded = true;
    }
    else {
        std::cerr << "눈 텍스처 로드 실패: " << texturePath << std::endl;
        unsigned char whitePixel[3] = { 255, 255, 255 };
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, 1, 1, 0, GL_RGB, GL_UNSIGNED_BYTE, whitePixel);
        textureLoaded = true;
    }

    stbi_image_free(data);
    glBindTexture(GL_TEXTURE_2D, 0);
}

bool Snow::isValidGroundPosition(float x, float z) const
{
    if (x < 0 || x >= MAP_WIDTH * BLOCK_SIZE) {
        return false;
    }

    bool frontGroundArea = (z >= 0 && z < 5.0f * BLOCK_SIZE);
  
    bool backGroundArea = (z >= 10.0f * BLOCK_SIZE && z < MAP_DEPTH * BLOCK_SIZE);

    return frontGroundArea || backGroundArea;
}

void Snow::addSnowAt(float x, float z)
{
    std::cout << "애니메이션과 함께 눈 생성 시도: (" << x << ", " << z << ")" << std::endl;

    if (!isValidGroundPosition(x, z)) {
        std::cout << "Ground가 아닌 위치입니다: (" << x << ", " << z << ")" << std::endl;
        return;
    }

    auto gridPos = worldToGrid(x, z);
    int targetGridX = gridPos.first;
    int targetGridZ = gridPos.second;

    if (!canSnowBeGenerated(targetGridX, targetGridZ)) {
        std::cout << "눈 생성 불가: 조건을 만족하지 않음" << std::endl;
        return;
    }

    auto& snowAnimData = snowData[{targetGridX, targetGridZ}];
    
    float previousHeight = snowAnimData.targetHeight;
    
    float newTargetHeight = previousHeight + 0.5f;
    if (newTargetHeight > 3.0f) {
        newTargetHeight = 3.0f;
        std::cout << "이미 최대 높이에 도달했습니다." << std::endl;
        return;
    }

    snowAnimData.targetHeight = newTargetHeight;
    snowAnimData.currentHeight = previousHeight;
    snowAnimData.animationTime = 0.0f;
    snowAnimData.animationDuration = defaultAnimationDuration;
    snowAnimData.isAnimating = true;
    
    snowAnimData.alphaValue = (previousHeight > 0.0f) ? 1.0f : 0.0f;
    
    needsUpdate = true;

    std::cout << "눈 애니메이션 시작! - 그리드(" << targetGridX << ", " << targetGridZ
              << ") 시작 높이: " << previousHeight << " -> 목표 높이: " << newTargetHeight << std::endl;
}

void Snow::updateAnimations(float deltaTime)
{
    bool hasActiveAnimations = false;
    
    for (auto& pair : snowData) {
        auto& animData = pair.second;
        
        if (animData.isAnimating) {
            hasActiveAnimations = true;
            animData.animationTime += deltaTime;
            
            float progress = animData.animationTime / animData.animationDuration;
            progress = std::min(progress, 1.0f);
            
            float startHeight = animData.targetHeight - 0.5f;
            if (startHeight < 0.0f) startHeight = 0.0f;
            
            float easedProgress = easeOutQuart(progress);
            float heightDifference = animData.targetHeight - startHeight;
            animData.currentHeight = startHeight + (heightDifference * easedProgress);
            
            if (startHeight <= 0.0f) {
                float fadeProgress = easeInOut(progress);
                animData.alphaValue = fadeProgress;
            } else {
                animData.alphaValue = 1.0f;
            }
            
            if (progress >= 1.0f) {
                animData.isAnimating = false;
                animData.currentHeight = animData.targetHeight;
                animData.alphaValue = 1.0f;
                std::cout << "눈 애니메이션 완료 - 시작: " << startHeight 
                         << " -> 최종 높이: " << animData.targetHeight << std::endl;
            }
            
            needsUpdate = true;
        }
    }
    
    if (hasActiveAnimations) {
        needsUpdate = true;
    }
}

float Snow::easeOutQuart(float t) const
{
    return 1.0f - std::pow(1.0f - t, 4.0f);
}

float Snow::easeInOut(float t) const
{
    if (t < 0.5f) {
        return 2.0f * t * t;
    }
    else {
        return 1.0f - std::pow(-2.0f * t + 2.0f, 2.0f) / 2.0f;
    }
}

float Snow::getSnowHeightAt(int gridX, int gridZ) const
{
    auto it = snowData.find({ gridX, gridZ });
    return (it != snowData.end()) ? it->second.currentHeight : 0.0f;
}

float Snow::getSnowHeightAtWorld(float x, float z) const
{
    auto gridPos = worldToGrid(x, z);
    return getSnowHeightAt(gridPos.first, gridPos.second);
}

std::pair<int, int> Snow::worldToGrid(float x, float z) const
{
    int gridX = static_cast<int>(std::floor(x / BLOCK_SIZE));
    int gridZ = static_cast<int>(std::floor(z / BLOCK_SIZE));
    return { gridX, gridZ };
}

glm::vec3 Snow::gridToWorld(int gridX, int gridZ, float height) const
{
    float worldX = gridX * BLOCK_SIZE;
    float worldZ = gridZ * BLOCK_SIZE;

    const float GROUND_TOP = 0.5f;
    float worldY = GROUND_TOP + height / 2.0f;

    return glm::vec3(worldX, worldY, worldZ);
}

void Snow::updateBuffers() const
{
    if (vao == 0) {
        glGenVertexArrays(1, &vao);
        glGenBuffers(1, &vbo);
        glGenBuffers(1, &nbo);
        glGenBuffers(1, &tbo);
        glGenBuffers(1, &alphaVBO);
    }

    std::vector<glm::vec3> allVertices, allNormals;
    std::vector<glm::vec2> allTexCoords;
    std::vector<float> allAlphas;

    for (const auto& pair : snowData) {
        int gridX = pair.first.first;
        int gridZ = pair.first.second;
        const auto& animData = pair.second;

        if (animData.currentHeight <= 0.0f) continue;

        glm::vec3 worldPos = gridToWorld(gridX, gridZ, animData.currentHeight);

        generateSnowBlock(worldPos.x, worldPos.y, worldPos.z,
            BLOCK_SIZE, animData.currentHeight, BLOCK_SIZE,
            allVertices, allNormals, allTexCoords, allAlphas,
            animData.alphaValue);
    }

    totalVertexCount = static_cast<GLsizei>(allVertices.size());

    if (totalVertexCount > 0) {
        glBindVertexArray(vao);

        // 버텍스 위치 데이터
        glBindBuffer(GL_ARRAY_BUFFER, vbo);
        glBufferData(GL_ARRAY_BUFFER, allVertices.size() * sizeof(glm::vec3),
            allVertices.data(), GL_STATIC_DRAW);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(glm::vec3), (void*)0);

        // 법선 데이터
        glBindBuffer(GL_ARRAY_BUFFER, nbo);
        glBufferData(GL_ARRAY_BUFFER, allNormals.size() * sizeof(glm::vec3),
            allNormals.data(), GL_STATIC_DRAW);
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(glm::vec3), (void*)0);

        // 텍스처 좌표 데이터
        glBindBuffer(GL_ARRAY_BUFFER, tbo);
        glBufferData(GL_ARRAY_BUFFER, allTexCoords.size() * sizeof(glm::vec2),
            allTexCoords.data(), GL_STATIC_DRAW);
        glEnableVertexAttribArray(2);
        glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(glm::vec2), (void*)0);

        // 알파 값 데이터
        glBindBuffer(GL_ARRAY_BUFFER, alphaVBO);
        glBufferData(GL_ARRAY_BUFFER, allAlphas.size() * sizeof(float),
            allAlphas.data(), GL_STATIC_DRAW);
        glEnableVertexAttribArray(3);
        glVertexAttribPointer(3, 1, GL_FLOAT, GL_FALSE, sizeof(float), (void*)0);

        glBindVertexArray(0);
        glBindBuffer(GL_ARRAY_BUFFER, 0);
    }
}

void Snow::generateSnowBlock(float x, float y, float z, float width, float height, float depth,
    std::vector<glm::vec3>& vertices, std::vector<glm::vec3>& normals,
    std::vector<glm::vec2>& texCoords, std::vector<float>& alphas,
    float alpha) const
{
    float halfWidth = width / 2.0f;
    float halfHeight = height / 2.0f;
    float halfDepth = depth / 2.0f;

    glm::vec3 v[8] = {
        {x - halfWidth, y - halfHeight, z - halfDepth},
        {x + halfWidth, y - halfHeight, z - halfDepth},
        {x + halfWidth, y + halfHeight, z - halfDepth},
        {x - halfWidth, y + halfHeight, z - halfDepth},
        {x - halfWidth, y - halfHeight, z + halfDepth},
        {x + halfWidth, y - halfHeight, z + halfDepth},
        {x + halfWidth, y + halfHeight, z + halfDepth},
        {x - halfWidth, y + halfHeight, z + halfDepth}
    };

    int faces[6][6] = {
        {0, 3, 2, 2, 1, 0}, // 전면
        {4, 5, 6, 6, 7, 4}, // 후면
        {0, 4, 7, 7, 3, 0}, // 좌측면
        {1, 2, 6, 6, 5, 1}, // 우측면
        {3, 7, 6, 6, 2, 3}, // 윗면
        {0, 1, 5, 5, 4, 0}  // 아랫면
    };

    glm::vec3 faceNormals[6] = {
        {0, 0, -1}, {0, 0, 1}, {-1, 0, 0},
        {1, 0, 0}, {0, 1, 0}, {0, -1, 0}
    };

    float heightRatio = std::min(height / 3.0f, 1.0f);
    float sideTexTop = heightRatio;

    for (int face = 0; face < 6; ++face) {
        for (int vertex = 0; vertex < 6; ++vertex) {
            vertices.push_back(v[faces[face][vertex]]);
            normals.push_back(faceNormals[face]);
            alphas.push_back(alpha);

            glm::vec2 texCoord;
            if (face == 4 || face == 5) { // 윗면/아랫면
                switch (vertex % 4) {
                case 0: texCoord = { 0.0f, 0.0f }; break;
                case 1: texCoord = { 1.0f, 0.0f }; break;
                case 2: texCoord = { 1.0f, 1.0f }; break;
                case 3: texCoord = { 0.0f, 1.0f }; break;
                }
            }
            else { // 측면
                switch (vertex % 4) {
                case 0: texCoord = { 0.0f, 0.0f }; break;
                case 1: texCoord = { 1.0f, 0.0f }; break;
                case 2: texCoord = { 1.0f, sideTexTop }; break;
                case 3: texCoord = { 0.0f, sideTexTop }; break;
                }
            }
            texCoords.push_back(texCoord);
        }
    }
}

void Snow::render(GLuint shaderProgram) const
{
    if (snowData.empty()) return;

    if (needsUpdate) {
        updateBuffers();
        needsUpdate = false;
    }

    if (!textureLoaded) {
        loadTexture();
    }

    if (totalVertexCount > 0) {
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

        if (textureLoaded && textureID != 0) {
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, textureID);
        }


        GLint modelLoc = glGetUniformLocation(shaderProgram, "model");
        GLint useTextureLoc = glGetUniformLocation(shaderProgram, "useTexture");
        GLint vColorLoc = glGetUniformLocation(shaderProgram, "vColor");

        glm::mat4 model = glm::mat4(1.0f);
        if (modelLoc != -1) glUniformMatrix4fv(modelLoc, 1, GL_FALSE, &model[0][0]);
        if (useTextureLoc != -1) glUniform1i(useTextureLoc, textureLoaded ? 1 : 0);
        if (vColorLoc != -1) glUniform3f(vColorLoc, 1.0f, 1.0f, 1.0f);

        glBindVertexArray(vao);
        glDrawArrays(GL_TRIANGLES, 0, totalVertexCount);
        glBindVertexArray(0);

        glDisable(GL_BLEND);
        glBindTexture(GL_TEXTURE_2D, 0);
    }
}

void Snow::clearAll()
{
    std::cout << "모든 눈 제거: " << snowData.size() << "개 블록" << std::endl;
    snowData.clear();
    needsUpdate = true;
    totalVertexCount = 0;
}

bool Snow::canSnowBeGenerated(int gridX, int gridZ) const
{
    if (getSnowHeightAt(gridX, gridZ) >= 3.0f) {
        std::cout << "이미 최대 높이의 눈이 쌓여있음" << std::endl;
        return false;
    }

    if (isAdjacentToWall(gridX, gridZ)) {
        std::cout << "벽에 인접한 위치 - 눈 생성 가능" << std::endl;
        return true;
    }

    if (isAdjacentToMaxHeightSnow(gridX, gridZ)) {
        std::cout << "최대 높이 눈에 인접한 위치 - 눈 생성 가능" << std::endl;
        return true;
    }

    std::cout << "격자(" << gridX << ", " << gridZ << ")는 생성 조건을 만족하지 않음" << std::endl;
    return false;
}

bool Snow::isAdjacentToMaxHeightSnow(int gridX, int gridZ) const
{
    int directions[4][2] = {
        {0, 1},   // 위쪽
        {0, -1},  // 아래쪽
        {1, 0},   // 오른쪽
        {-1, 0}   // 왼쪽
    };

    for (int i = 0; i < 4; i++) {
        int checkX = gridX + directions[i][0];
        int checkZ = gridZ + directions[i][1];

        float adjacentHeight = getSnowHeightAt(checkX, checkZ);
        if (adjacentHeight >= 1.0f) {
            std::cout << "인접 위치(" << checkX << ", " << checkZ << ")에 최대 높이 눈 존재 (높이: "
                << adjacentHeight << ")" << std::endl;
            return true;
        }
    }

    return false;
}

bool Snow::isAdjacentToWall(int gridX, int gridZ) const
{
    if (gridZ >= 0 && gridZ < 5) {
        if (gridZ == 0) {
            std::cout << "앞쪽 벽(z=0)에 인접" << std::endl;
            return true;
        }

        if (gridX == 0 || gridX == 9) { // MAP_WIDTH-1 = 9
            std::cout << "좌우 벽(x=" << gridX << ")에 인접" << std::endl;
            return true;
        }

        if (gridZ == 4) {
            std::cout << "중간 공간 경계(z=4)에 인접" << std::endl;
            return true;
        }
    }
    else if (gridZ >= 10 && gridZ < 15) {
        if (gridZ == 14) {
            std::cout << "뒤쪽 벽(z=14)에 인접" << std::endl;
            return true;
        }

        if (gridX == 0 || gridX == 9) {
            std::cout << "좌우 벽(x=" << gridX << ")에 인접" << std::endl;
            return true;
        }

        if (gridZ == 10) {
            std::cout << "중간 공간 경계(z=10)에 인접" << std::endl;
            return true;
        }
    }

    return false;
}

bool Snow::isAdjacentToExistingSnow(int gridX, int gridZ) const
{
    return isAdjacentToMaxHeightSnow(gridX, gridZ);
}