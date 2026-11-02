#include "title.h"
#include <vector>
#include <iostream>

#include "stb_image.h"

TitleScreen::TitleScreen() : vao(0), vbo(0), isInitialized(false) {}

TitleScreen::~TitleScreen() {
    cleanup();
}

void TitleScreen::initialize() {
    if (isInitialized) return;

    initializeBuffers();
    loadTitleTexture();
    isInitialized = true;

    std::cout << "타이틀 화면 초기화 완료" << std::endl;
}

void TitleScreen::initializeBuffers() {
    std::vector<float> vertices = {
        -1.0f,  1.0f,   0.0f, 1.0f,
        -1.0f, -1.0f,   0.0f, 0.0f,
         1.0f, -1.0f,   1.0f, 0.0f,

        -1.0f,  1.0f,   0.0f, 1.0f,
         1.0f, -1.0f,   1.0f, 0.0f,
         1.0f,  1.0f,   1.0f, 1.0f
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

void TitleScreen::loadTitleTexture() {
    if (textureLoaded) return;

    int width = 0, height = 0, channels = 0;
    stbi_set_flip_vertically_on_load(true);
    unsigned char* data = stbi_load("Title.png", &width, &height, &channels, 0);
    if (!data) {
        std::cerr << "Title.png 로드 실패, 단색 배경으로 대체됩니다." << std::endl;
        textureLoaded = false;
        return;
    }

    GLenum format = GL_RGB;
    if (channels == 1) format = GL_RED;
    else if (channels == 3) format = GL_RGB;
    else if (channels == 4) format = GL_RGBA;

    glGenTextures(1, &titleTextureId);
    glBindTexture(GL_TEXTURE_2D, titleTextureId);
    glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);
    glGenerateMipmap(GL_TEXTURE_2D);

    // 텍스처 파라미터
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    glBindTexture(GL_TEXTURE_2D, 0);
    stbi_image_free(data);

    textureLoaded = true;
    std::cout << "Title.png 텍스처 로드 성공: " << width << "x" << height << " (" << channels << " channels)" << std::endl;
}

void TitleScreen::render(GLuint shaderProgram) const {
    if (!isInitialized) return;

    glDisable(GL_DEPTH_TEST);
    glUseProgram(shaderProgram);

    glm::mat4 model = glm::mat4(1.0f);
    glm::mat4 view = glm::mat4(1.0f);
    glm::mat4 projection = glm::mat4(1.0f);

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

    if (textureLoaded && textureLoc != -1 && useTextureLoc != -1) {
        glUniform1i(useTextureLoc, 1);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, titleTextureId);
        glUniform1i(textureLoc, 0);
    }
    else {
        if (useTextureLoc != -1) glUniform1i(useTextureLoc, 0);
        if (vColorLoc != -1) glUniform3f(vColorLoc, 0.1f, 0.1f, 0.3f);
    }

    glBindVertexArray(vao);
    glDrawArrays(GL_TRIANGLES, 0, 6);
    glBindVertexArray(0);

    if (textureLoaded) {
        glBindTexture(GL_TEXTURE_2D, 0);
    }

    renderText("MINECRAFT SNOWBALL FIGHT", 0.0f, 0.3f, 2.0f);
    renderText("1 - First Person Mode", 0.0f, 0.0f, 1.0f);
    renderText("3 - Third Person Mode", 0.0f, -0.2f, 1.0f);
    renderText("ESC - Exit", 0.0f, -0.4f, 1.0f);

    glEnable(GL_DEPTH_TEST);
}

void TitleScreen::renderText(const std::string& text, float x, float y, float scale) const {
    static bool textDisplayed = false;
    if (!textDisplayed) {
        std::cout << "\n=== " << text << " ===" << std::endl;
        textDisplayed = true;
    }
}

GameState TitleScreen::handleKeyInput(unsigned char key) const {
    switch (key) {
    case '1':
        std::cout << "1인칭 모드로 게임 시작!" << std::endl;
        return GameState::FIRST_PERSON_MODE;
    case '3':
        std::cout << "3인칭 모드로 게임 시작!" << std::endl;
        return GameState::THIRD_PERSON_MODE;
    case 27:
    case 'q': case 'Q':
        std::cout << "게임 종료" << std::endl;
        // [OnExit 5] Main.cpp handles exit keys before calling this menu handler.
        return GameState::TITLE_SCREEN;
        break;
    default:
        return GameState::TITLE_SCREEN;
    }
    return GameState::TITLE_SCREEN;
}

void TitleScreen::cleanup() {
    if (isInitialized) {
        if (vao != 0) {
            glDeleteVertexArrays(1, &vao);
            vao = 0;
        }
        if (vbo != 0) {
            glDeleteBuffers(1, &vbo);
            vbo = 0;
        }
        if (titleTextureId != 0) {
            glDeleteTextures(1, &titleTextureId);
            titleTextureId = 0;
        }
        isInitialized = false;
        textureLoaded = false;
    }
}