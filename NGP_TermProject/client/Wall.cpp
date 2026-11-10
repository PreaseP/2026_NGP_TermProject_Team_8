#include "Wall.h"
#include <gl/glew.h>
#include <gl/glm/glm.hpp>

Wall::Wall()
{
}

void Wall::addBlock(const Block& block)
{
    blocks.push_back(block);
}

void Wall::addBlock(float x, float y, float z, float size)
{
    blocks.emplace_back(x, y, z, size);
}

void Wall::createHorizontalWall(float startX, float y, float z, int length, float blockSize)
{
    for (int i = 0; i < length; ++i) {
        float x = startX + i * blockSize;
        addBlock(x, y, z, blockSize);
    }
}

void Wall::createVerticalWall(float x, float y, float startZ, int length, float blockSize)
{
    for (int i = 0; i < length; ++i) {
        float z = startZ + i * blockSize;
        addBlock(x, y, z, blockSize);
    }
}

void Wall::render() const
{
    extern GLuint shaderProgramID;
    extern glm::vec3 cameraPos;
    
    GLint lightPosLoc = glGetUniformLocation(shaderProgramID, "lightPos");
    GLint lightColorLoc = glGetUniformLocation(shaderProgramID, "lightColor");
    GLint viewPosLoc = glGetUniformLocation(shaderProgramID, "viewPos");
    GLint vColorLoc = glGetUniformLocation(shaderProgramID, "vColor");
    GLint lightingEnabledLoc = glGetUniformLocation(shaderProgramID, "lightingEnabled");
    
    if (lightingEnabledLoc != -1) {
        glUniform1i(lightingEnabledLoc, 1); // true
    }
    
    if (lightPosLoc != -1) {
        glm::vec3 lightPos(5.0f, 8.0f, 7.5f);
        glUniform3fv(lightPosLoc, 1, &lightPos[0]);
    }
    
    if (lightColorLoc != -1) {
        glm::vec3 lightColor(1.5f, 1.5f, 1.5f);
        glUniform3fv(lightColorLoc, 1, &lightColor[0]);
    }
    
    if (viewPosLoc != -1) {
        glUniform3fv(viewPosLoc, 1, &cameraPos[0]);
    }
    
    glm::vec3 wallColor(0.6f, 0.4f, 0.2f);
    
    for (const auto& block : blocks) {
        block.render(shaderProgramID, wallColor);
    }
}
