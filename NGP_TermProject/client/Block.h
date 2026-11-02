#pragma once
#include <gl/glew.h>
#include <gl/glm/glm.hpp>
#include <vector>
#include <string>

class Block
{
private:
    float x, y, z;
    float size;
    
    mutable GLuint vao = 0;
    mutable GLuint vbo = 0;
    mutable GLuint nbo = 0;
    mutable GLuint tbo = 0;
    mutable GLuint texture = 0;
    mutable GLsizei count = 0;
    mutable bool isInitialized = false;

    std::string texturePath;
    mutable bool textureLoaded = false;

public:
    Block();
    Block(float x, float y, float z, float size = 1.0f, const std::string& texturePath = "oak_planks.png");
    
    Block(const Block& other);
    Block& operator=(const Block& other);
    
    float getX() const { return x; }
    float getY() const { return y; }
    float getZ() const { return z; }
    float getSize() const { return size; }
    
    void setPosition(float x, float y, float z);
    void setSize(float size);
    void setTexture(const std::string& texturePath);
    
    void loadTexture() const;
    
    void initialize() const;
    void render(GLuint shaderProgram, const glm::vec3& color = glm::vec3(0.5f, 0.5f, 0.5f)) const;
    
    ~Block();
};

