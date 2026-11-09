#pragma once
#include <gl/glew.h>
#include <gl/glm/glm.hpp>
#include <string>

enum class GameState {
    TITLE_SCREEN,
    FIRST_PERSON_MODE,
    THIRD_PERSON_MODE,
    FINISH_SCREEN
};

class TitleScreen {
public:
    TitleScreen();
    ~TitleScreen();

    void initialize();
    void render(GLuint shaderProgram) const;
    GameState handleKeyInput(unsigned char key) const;
    void cleanup();
    // [11.7] 로그인 입력·버튼과 로그인 성공 후 로비 안내를 그린다.
    void renderLoginUI(GLuint shaderProgram, const char* name, const std::string& error, bool lobby) const;

private:
    void initializeBuffers();
    void renderText(const std::string& text, float x, float y, float scale) const;
    void loadTitleTexture();
    void loadFontTexture(); // [11.7] 글자도 같은 셰이더로 그리도록 텍스처로 준비한다.
    void drawUIQuad(GLuint shader, float x, float y, float w, float h,
        GLuint texture, const glm::vec4& color, const glm::vec4& uv = glm::vec4(0, 0, 1, 1)) const;
    void panel(GLuint shader, float x, float y, float w, float h, float r, float g, float b, float a) const;
    void centeredText(GLuint shader, const std::string& text, float y, float r, float g, float b, float size = 18.0f) const;
    GLuint fontTextureId = 0;

    GLuint vao;
    GLuint vbo;
    bool isInitialized;

    GLuint titleTextureId = 0;
    bool textureLoaded = false;
};
