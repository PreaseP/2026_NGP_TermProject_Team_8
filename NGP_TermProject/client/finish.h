#pragma once
#include <gl/glew.h>
#include <gl/glm/glm.hpp>
#include <string>

enum class Winner {
    NONE,
    STEVE,
    ALEX
};

class FinishScreen {
public:
    FinishScreen();
    ~FinishScreen();

    void initialize();
    void render(GLuint shaderProgram) const;
    void cleanup();

    void setWinner(Winner w);

private:
    void initializeBuffers();
    void loadTextureForWinner();

    GLuint vao = 0;
    GLuint vbo = 0;
    GLuint textureId = 0;
    bool isInitialized = false;
    bool textureLoaded = false;

    Winner winner = Winner::NONE;
};