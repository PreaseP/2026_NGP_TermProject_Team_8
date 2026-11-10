#include "finish.h"
#include "stb_image.h"
#include <vector>
#include <iostream>

FinishScreen::FinishScreen() {}
FinishScreen::~FinishScreen() { cleanup(); }

void FinishScreen::initialize() {
    if (isInitialized) return;
    initializeBuffers();
    isInitialized = true;
}

void FinishScreen::initializeBuffers() {
    const std::vector<float> vertices = {
        -1.0f,  1.0f,  0.0f, 1.0f,
        -1.0f, -1.0f,  0.0f, 0.0f,
         1.0f, -1.0f,  1.0f, 0.0f,

        -1.0f,  1.0f,  0.0f, 1.0f,
         1.0f, -1.0f,  1.0f, 0.0f,
         1.0f,  1.0f,  1.0f, 1.0f
    };

    glGenVertexArrays(1, &vao);
    glGenBuffers(1, &vbo);

    glBindVertexArray(vao);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(float), vertices.data(), GL_STATIC_DRAW);

    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);

    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(2 * sizeof(float)));

    glBindVertexArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
}

void FinishScreen::setWinner(Winner w) {
    winner = w;
    loadTextureForWinner();
}

void FinishScreen::loadTextureForWinner() {
    if (textureId != 0) {
        glDeleteTextures(1, &textureId);
        textureId = 0;
        textureLoaded = false;
    }

    const char* path = nullptr;
    switch (winner) {
    case Winner::STEVE: path = "steve_win.png"; break;
    case Winner::ALEX:  path = "alex_win.png";  break;
    default:            path = nullptr;         break;
    }

    if (!path) {
        std::cerr << "FinishScreen: 승자 텍스처 없음, 단색 배경 사용" << std::endl;
        textureLoaded = false;
        return;
    }

    int width = 0, height = 0, channels = 0;
    stbi_set_flip_vertically_on_load(true);
    unsigned char* data = stbi_load(path, &width, &height, &channels, 0);
    if (!data) {
        std::cerr << "FinishScreen: 텍스처 로드 실패: " << path << std::endl;
        textureLoaded = false;
        return;
    }

    GLenum format = GL_RGB;
    if (channels == 1) format = GL_RED;
    else if (channels == 3) format = GL_RGB;
    else if (channels == 4) format = GL_RGBA;

    glGenTextures(1, &textureId);
    glBindTexture(GL_TEXTURE_2D, textureId);
    glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);
    glGenerateMipmap(GL_TEXTURE_2D);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    glBindTexture(GL_TEXTURE_2D, 0);
    stbi_image_free(data);

    textureLoaded = true;
    std::cout << "FinishScreen: 텍스처 로드 성공: " << path << " (" << width << "x" << height << ", " << channels << "ch)" << std::endl;
}

void FinishScreen::render(GLuint shaderProgram) const {
    if (!isInitialized) return;

    glDisable(GL_DEPTH_TEST);
    glUseProgram(shaderProgram);

    glm::mat4 model(1.0f), view(1.0f), projection(1.0f);

    GLint modelLoc = glGetUniformLocation(shaderProgram, "model");
    GLint viewLoc = glGetUniformLocation(shaderProgram, "view");
    GLint projLoc = glGetUniformLocation(shaderProgram, "projection");
    GLint useTextureLoc = glGetUniformLocation(shaderProgram, "useTexture");
    GLint lightingEnabledLoc = glGetUniformLocation(shaderProgram, "lightingEnabled");
    GLint vColorLoc = glGetUniformLocation(shaderProgram, "vColor");
    GLint textureLoc = glGetUniformLocation(shaderProgram, "texture1");

    if (modelLoc != -1) glUniformMatrix4fv(modelLoc, 1, GL_FALSE, &model[0][0]);
    if (viewLoc != -1) glUniformMatrix4fv(viewLoc, 1, GL_FALSE, &view[0][0]);
    if (projLoc != -1) glUniformMatrix4fv(projLoc, 1, GL_FALSE, &projection[0][0]);
    if (lightingEnabledLoc != -1) glUniform1i(lightingEnabledLoc, 0);

    if (textureLoaded && useTextureLoc != -1 && textureLoc != -1) {
        glUniform1i(useTextureLoc, 1);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, textureId);
        glUniform1i(textureLoc, 0);
    }
    else {
        if (useTextureLoc != -1) glUniform1i(useTextureLoc, 0);
        if (vColorLoc != -1) glUniform3f(vColorLoc, 0.05f, 0.05f, 0.05f);
    }

    glBindVertexArray(vao);
    glDrawArrays(GL_TRIANGLES, 0, 6);
    glBindVertexArray(0);

    if (textureLoaded) glBindTexture(GL_TEXTURE_2D, 0);
    glEnable(GL_DEPTH_TEST);
}

void FinishScreen::cleanup() {
    if (vao) { glDeleteVertexArrays(1, &vao); vao = 0; }
    if (vbo) { glDeleteBuffers(1, &vbo); vbo = 0; }
    if (textureId) { glDeleteTextures(1, &textureId); textureId = 0; }
    isInitialized = false;
    textureLoaded = false;
}