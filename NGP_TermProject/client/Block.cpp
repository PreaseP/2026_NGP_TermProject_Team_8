#include "Block.h"
#include <vector>
#include <gl/glm/glm.hpp>
#include <gl/glm/ext.hpp>
#include <gl/glm/gtc/matrix_transform.hpp>
#include <iostream>

#include "stb_image.h"

Block::Block() : x(0.0f), y(0.0f), z(0.0f), size(10.0f), texturePath("oak_planks.png")
{
    
}

Block::Block(float x, float y, float z, float size, const std::string& texturePath) 
    : x(x), y(y), z(z), size(size), texturePath(texturePath)
{
    
}

Block::Block(const Block& other) 
    : x(other.x), y(other.y), z(other.z), size(other.size), texturePath(other.texturePath)
{
 
}

Block& Block::operator=(const Block& other)
{
    if (this != &other) {
        if (isInitialized) {
            glDeleteVertexArrays(1, &vao);
            glDeleteBuffers(1, &vbo);
            glDeleteBuffers(1, &nbo);
            glDeleteBuffers(1, &tbo);
            if (textureLoaded && texture != 0) {
                glDeleteTextures(1, &texture);
            }
            isInitialized = false;
            textureLoaded = false;
            vao = vbo = nbo = tbo = texture = 0;
            count = 0;
        }
        
        x = other.x;
        y = other.y;
        z = other.z;
        size = other.size;
        texturePath = other.texturePath;
    }
    return *this;
}

Block::~Block()
{
    if (isInitialized) {
        glDeleteVertexArrays(1, &vao);
        glDeleteBuffers(1, &vbo);
        glDeleteBuffers(1, &nbo);
        glDeleteBuffers(1, &tbo);
        if (textureLoaded && texture != 0) {
            glDeleteTextures(1, &texture);
        }
    }
}

void Block::setPosition(float x, float y, float z)
{
    this->x = x;
    this->y = y;
    this->z = z;
}

void Block::setSize(float size)
{
    this->size = size;
    if (isInitialized) {
        initialize();
    }
}

void Block::setTexture(const std::string& texturePath)
{
    this->texturePath = texturePath;
    if (textureLoaded) {
        if (texture != 0) {
            glDeleteTextures(1, &texture);
            texture = 0;
        }
        textureLoaded = false;
    }
}

void Block::loadTexture() const
{
    if (textureLoaded) return;

    glGenTextures(1, &texture);
    glBindTexture(GL_TEXTURE_2D, texture);

    // 텍스처 파라미터 설정
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    // 이미지 로드
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
            format = GL_RGB; // 기본값

        glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);
        glGenerateMipmap(GL_TEXTURE_2D);
        
        std::cout << "텍스처 로드 성공: " << texturePath << " (" << width << "x" << height << ", " << channels << " channels)" << std::endl;
        textureLoaded = true;
    }
    else {
        std::cerr << "텍스처 로드 실패: " << texturePath << std::endl;
        // 기본 텍스처 생성
        unsigned char whitePixel[3] = {255, 255, 255};
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, 1, 1, 0, GL_RGB, GL_UNSIGNED_BYTE, whitePixel);
        textureLoaded = true;
    }
    
    stbi_image_free(data);
    glBindTexture(GL_TEXTURE_2D, 0);
}

void Block::initialize() const
{
    if (isInitialized) {
        glDeleteVertexArrays(1, &vao);
        glDeleteBuffers(1, &vbo);
        glDeleteBuffers(1, &nbo);
        glDeleteBuffers(1, &tbo);
    }

    // 큐브 버텍스 생성
    float half = size / 2.0f;
    std::vector<glm::vec3> vertices = {
        // Front face
        {-half, -half, +half}, {+half, -half, +half}, {+half, +half, +half},
        {+half, +half, +half}, {-half, +half, +half}, {-half, -half, +half},
        
        // Back face  
        {+half, -half, -half}, {-half, -half, -half}, {-half, +half, -half},
        {-half, +half, -half}, {+half, +half, -half}, {+half, -half, -half},
        
        // Left face
        {-half, -half, -half}, {-half, -half, +half}, {-half, +half, +half},
        {-half, +half, +half}, {-half, +half, -half}, {-half, -half, -half},
        
        // Right face
        {+half, -half, +half}, {+half, -half, -half}, {+half, +half, -half},
        {+half, +half, -half}, {+half, +half, +half}, {+half, -half, +half},
        
        // Top face
        {-half, +half, +half}, {+half, +half, +half}, {+half, +half, -half},
        {+half, +half, -half}, {-half, +half, -half}, {-half, +half, +half},
        
        // Bottom face
        {-half, -half, -half}, {+half, -half, -half}, {+half, -half, +half},
        {+half, -half, +half}, {-half, -half, +half}, {-half, -half, -half}
    };

    std::vector<glm::vec3> normals = {
        // Front
        {0, 0, 1}, {0, 0, 1}, {0, 0, 1}, {0, 0, 1}, {0, 0, 1}, {0, 0, 1},
        // Back
        {0, 0, -1}, {0, 0, -1}, {0, 0, -1}, {0, 0, -1}, {0, 0, -1}, {0, 0, -1},
        // Left
        {-1, 0, 0}, {-1, 0, 0}, {-1, 0, 0}, {-1, 0, 0}, {-1, 0, 0}, {-1, 0, 0},
        // Right
        {1, 0, 0}, {1, 0, 0}, {1, 0, 0}, {1, 0, 0}, {1, 0, 0}, {1, 0, 0},
        // Top
        {0, 1, 0}, {0, 1, 0}, {0, 1, 0}, {0, 1, 0}, {0, 1, 0}, {0, 1, 0},
        // Bottom
        {0, -1, 0}, {0, -1, 0}, {0, -1, 0}, {0, -1, 0}, {0, -1, 0}, {0, -1, 0}
    };

    // 텍스처 좌표
    std::vector<glm::vec2> texCoords = {
        // Front face
        {0.0f, 0.0f}, {1.0f, 0.0f}, {1.0f, 1.0f},
        {1.0f, 1.0f}, {0.0f, 1.0f}, {0.0f, 0.0f},
        
        // Back face
        {0.0f, 0.0f}, {1.0f, 0.0f}, {1.0f, 1.0f},
        {1.0f, 1.0f}, {0.0f, 1.0f}, {0.0f, 0.0f},
        
        // Left face
        {0.0f, 0.0f}, {1.0f, 0.0f}, {1.0f, 1.0f},
        {1.0f, 1.0f}, {0.0f, 1.0f}, {0.0f, 0.0f},
        
        // Right face
        {0.0f, 0.0f}, {1.0f, 0.0f}, {1.0f, 1.0f},
        {1.0f, 1.0f}, {0.0f, 1.0f}, {0.0f, 0.0f},
        
        // Top face
        {0.0f, 0.0f}, {1.0f, 0.0f}, {1.0f, 1.0f},
        {1.0f, 1.0f}, {0.0f, 1.0f}, {0.0f, 0.0f},
        
        // Bottom face
        {0.0f, 0.0f}, {1.0f, 0.0f}, {1.0f, 1.0f},
        {1.0f, 1.0f}, {0.0f, 1.0f}, {0.0f, 0.0f}
    };

    // 버퍼 생성
    glGenVertexArrays(1, &vao);
    glGenBuffers(1, &vbo);
    glGenBuffers(1, &nbo);
    glGenBuffers(1, &tbo);

    glBindVertexArray(vao);

    // 버텍스 위치 데이터
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(glm::vec3), vertices.data(), GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(glm::vec3), (void*)0);

    // 법선 데이터
    glBindBuffer(GL_ARRAY_BUFFER, nbo);
    glBufferData(GL_ARRAY_BUFFER, normals.size() * sizeof(glm::vec3), normals.data(), GL_STATIC_DRAW);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(glm::vec3), (void*)0);

    // 텍스처 좌표 데이터
    glBindBuffer(GL_ARRAY_BUFFER, tbo);
    glBufferData(GL_ARRAY_BUFFER, texCoords.size() * sizeof(glm::vec2), texCoords.data(), GL_STATIC_DRAW);
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(glm::vec2), (void*)0);

    glBindVertexArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    count = static_cast<GLsizei>(vertices.size());
    isInitialized = true;
    
    loadTexture();
}

void Block::render(GLuint shaderProgram, const glm::vec3& color) const
{
    if (!isInitialized) {
        initialize();
    }

    if (textureLoaded && texture != 0) {
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, texture);
    }

    glm::mat4 model = glm::translate(glm::mat4(1.0f), glm::vec3(x, y, z));
    
    glm::mat3 normalMatrix = glm::mat3(glm::transpose(glm::inverse(model)));
    
    GLint modelLoc = glGetUniformLocation(shaderProgram, "model");
    GLint normalMatrixLoc = glGetUniformLocation(shaderProgram, "normalMatrix");
    GLint vColorLoc = glGetUniformLocation(shaderProgram, "vColor");
    GLint useTextureLoc = glGetUniformLocation(shaderProgram, "useTexture");
    GLint textureLoc = glGetUniformLocation(shaderProgram, "texture1");
    
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
        glUniform1i(useTextureLoc, textureLoaded ? 1 : 0);
    }
    if (textureLoc != -1) {
        glUniform1i(textureLoc, 0);
    }

    glBindVertexArray(vao);
    glDrawArrays(GL_TRIANGLES, 0, count);
    glBindVertexArray(0);
    
    glBindTexture(GL_TEXTURE_2D, 0);
}
