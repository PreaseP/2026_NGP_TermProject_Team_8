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

private:
    void initializeBuffers();
    void renderText(const std::string& text, float x, float y, float scale) const;
    void loadTitleTexture();

    GLuint vao;
    GLuint vbo;
    bool isInitialized;

    GLuint titleTextureId = 0;
    bool textureLoaded = false;
};