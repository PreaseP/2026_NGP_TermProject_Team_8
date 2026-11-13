#include "title.h"
#include "../common/protocol.h" // [11.10] 방 정보와 상태 종류
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
    // [11.10] 로비 배경, 방 버튼, 제목 이미지를 준비한다.
    loadLobbyTexture("Images/Lobby/Lobby_background.png", lobbyBackgroundTextureId);
    loadLobbyTexture("Images/Lobby/lobby_roombutton.png", lobbyRoomButtonTextureId);
    loadLobbyTexture("Images/Lobby/lobby_text.png", lobbyTextTextureId);
    loadStartTexture(); // [11.7] 시작 버튼 텍스처 준비
    loadFontTexture(); // [11.7] 화면 글자 텍스처 준비
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
    unsigned char* data = stbi_load("TitleBackground.png", &width, &height, &channels, 0);
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

// [11.10] PNG 파일을 읽어 화면에서 사용할 텍스처로 만든다.
void TitleScreen::loadLobbyTexture(const char* path, GLuint& textureId) {
    int width = 0, height = 0, channels = 0;
    stbi_set_flip_vertically_on_load(true);
    unsigned char* pixels = stbi_load(path, &width, &height, &channels, STBI_rgb_alpha);
    if (!pixels) {
        std::cerr << "Failed to load lobby image: " << path << std::endl;
        return;
    }

    glGenTextures(1, &textureId);
    glBindTexture(GL_TEXTURE_2D, textureId);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, pixels);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glBindTexture(GL_TEXTURE_2D, 0);
    stbi_image_free(pixels);
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
    default:
        return GameState::TITLE_SCREEN;
    }
    return GameState::TITLE_SCREEN;
}

void TitleScreen::cleanup() {
    // [11.10] 로비 텍스처를 지우고 번호를 0으로 비운다.
    if (lobbyBackgroundTextureId != 0) {
        glDeleteTextures(1, &lobbyBackgroundTextureId);
        lobbyBackgroundTextureId = 0;
    }
    if (lobbyRoomButtonTextureId != 0) {
        glDeleteTextures(1, &lobbyRoomButtonTextureId);
        lobbyRoomButtonTextureId = 0;
    }
    if (lobbyTextTextureId != 0) {
        glDeleteTextures(1, &lobbyTextTextureId);
        lobbyTextTextureId = 0;
    }
    if (fontTextureId != 0) { glDeleteTextures(1, &fontTextureId); fontTextureId = 0; } // [11.7]
    if (startTextureId != 0) { glDeleteTextures(1, &startTextureId); startTextureId = 0; } // [11.7]
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


#include <gl/freeglut.h>

#include <cmath>
#include <cstring>
#pragma comment(lib, "gdi32.lib") // [11.7] 글자 텍스처 생성에 사용한다.
#include <gl/glm/gtc/matrix_transform.hpp>
#include <algorithm>

// [11.7] 기존 VAO와 셰이더를 사용해 메뉴도 사각형으로 그린다.
void TitleScreen::drawUIQuad(GLuint shader, float x, float y, float w, float h,
    GLuint texture, const glm::vec4& color, const glm::vec4& uv) const {
    glm::mat4 model(1.0f);
    model = glm::translate(model, glm::vec3(x + w / 2, y + h / 2, 0));
    model = glm::scale(model, glm::vec3(w / 2, -h / 2, 1));
    glUniformMatrix4fv(glGetUniformLocation(shader, "model"), 1, GL_FALSE, &model[0][0]);
    glUniform4fv(glGetUniformLocation(shader, "uiColor"), 1, &color[0]);
    glUniform4fv(glGetUniformLocation(shader, "uiUVRect"), 1, &uv[0]);
    glUniform1i(glGetUniformLocation(shader, "useTexture"), texture != 0);
    glBindTexture(GL_TEXTURE_2D, texture);
    glBindVertexArray(vao);
    glDrawArrays(GL_TRIANGLES, 0, 6);
}

void TitleScreen::panel(GLuint shader, float x, float y, float w, float h,
    float r, float g, float b, float a) const {
    drawUIQuad(shader, x, y, w, h, 0, glm::vec4(r, g, b, a));
}

// [11.7] 운영체제 글꼴로 영문·숫자 표를 한 번 만들고 GPU에 올린다.
void TitleScreen::loadFontTexture() {
    const int width = 384, height = 192; // 24 x 32 크기의 글자 칸, 16열 x 6행
    HDC dc = CreateCompatibleDC(nullptr);
    if (!dc) return;
    BITMAPINFO info{};
    info.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    info.bmiHeader.biWidth = width;
    info.bmiHeader.biHeight = height;
    info.bmiHeader.biPlanes = 1;
    info.bmiHeader.biBitCount = 32;
    info.bmiHeader.biCompression = BI_RGB;
    void* pixels = nullptr;
    HBITMAP bitmap = CreateDIBSection(dc, &info, DIB_RGB_COLORS, &pixels, nullptr, 0);
    HFONT font = CreateFontA(-24, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
        ANSI_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, ANTIALIASED_QUALITY,
        FIXED_PITCH, "Consolas");
    if (!bitmap || !font) {
        if (bitmap) DeleteObject(bitmap);
        if (font) DeleteObject(font);
        DeleteDC(dc); return;
    }
    HGDIOBJ oldBitmap = SelectObject(dc, bitmap), oldFont = SelectObject(dc, font);
    std::memset(pixels, 0, width * height * 4);
    SetBkMode(dc, TRANSPARENT); SetTextColor(dc, RGB(255, 255, 255));
    for (int i = 0; i < 95; ++i) {
        char letter = static_cast<char>(32 + i);
        TextOutA(dc, (i % 16) * 24, (i / 16) * 32, &letter, 1);
    }
    GdiFlush();
    // 배경은 투명하게, 글자는 흰색으로 저장해 셰이더에서 원하는 색을 입힌다.
    auto* bytes = static_cast<unsigned char*>(pixels);
    for (int i = 0; i < width * height; ++i) {
        unsigned char alpha = bytes[i * 4];
        bytes[i * 4] = bytes[i * 4 + 1] = bytes[i * 4 + 2] = 255;
        bytes[i * 4 + 3] = alpha;
    }
    glGenTextures(1, &fontTextureId);
    glBindTexture(GL_TEXTURE_2D, fontTextureId);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, pixels);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glBindTexture(GL_TEXTURE_2D, 0);
    SelectObject(dc, oldFont); SelectObject(dc, oldBitmap);
    DeleteObject(font); DeleteObject(bitmap); DeleteDC(dc);
}

void TitleScreen::centeredText(GLuint shader, const std::string& text, float y,
    float r, float g, float b, float size) const {
    if (!fontTextureId) return;
    const float scale = size / 24.0f;
    float x = 600.0f - static_cast<float>(text.size()) * 14.0f * scale / 2;
    for (unsigned char ch : text) {
        if (ch < 32 || ch > 126) ch = '?';
        const int index = ch - 32, column = index % 16, row = index / 16;
        glm::vec4 uv(column / 16.0f, 1.0f - (row + 1) / 6.0f,
                     (column + 1) / 16.0f, 1.0f - row / 6.0f);
        drawUIQuad(shader, x, y - size, 24 * scale, 32 * scale, fontTextureId,
            glm::vec4(r, g, b, 1), uv);
        x += 14 * scale;
    }
}

void TitleScreen::loadStartTexture() {
    int w, h, channels;
    stbi_set_flip_vertically_on_load(true);
    unsigned char* pixels = stbi_load("GameStart.png", &w, &h, &channels, 4);
    if (!pixels) return;
    // [11.7] PNG의 투명 여백을 제외해 글자와 클릭 영역을 맞춘다.
    int left = w, right = -1, bottom = h, top = -1;
    for (int y = 0; y < h; ++y) for (int x = 0; x < w; ++x) {
        if (pixels[(y * w + x) * 4 + 3] > 16) {
            left = std::min(left, x); right = std::max(right, x);
            bottom = std::min(bottom, y); top = std::max(top, y);
        }
    }
    if (right >= left && top >= bottom) {
        startUV[0] = static_cast<float>(left) / w;
        startUV[1] = static_cast<float>(bottom) / h;
        startUV[2] = static_cast<float>(right + 1) / w;
        startUV[3] = static_cast<float>(top + 1) / h;
        startAspect = static_cast<float>(right - left + 1) / (top - bottom + 1);
    }
    glGenTextures(1, &startTextureId);
    glBindTexture(GL_TEXTURE_2D, startTextureId);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, pixels);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glBindTexture(GL_TEXTURE_2D, 0);
    stbi_image_free(pixels);
}

// [11.7] 다시 실패하면 효과를 처음부터 재생한다. 크기 변화는 멈춘 지점에서 이어진다.
void TitleScreen::PlayNameError() {
    const int now = glutGet(GLUT_ELAPSED_TIME);
    if (startErrorTime >= 0) startPausedTime += std::min(now - startErrorTime, START_ERROR_MS);
    startErrorTime = now;
}

float TitleScreen::startScale() const {
    const int now = glutGet(GLUT_ELAPSED_TIME);
    int animationTime = now - startPausedTime;
    if (startErrorTime >= 0) {
        // [11.7] 오류 효과 중에는 크기를 고정하고, 끝나면 이어서 움직인다.
        const int elapsed = now - startErrorTime;
        animationTime -= std::min(elapsed, START_ERROR_MS);
    }
    return 1.0f + 0.035f * std::sin(animationTime * 0.0025f);
}

float TitleScreen::startErrorStrength() const {
    if (startErrorTime < 0) return 0.0f;
    const int elapsed = glutGet(GLUT_ELAPSED_TIME) - startErrorTime;
    return std::max(0.0f, 1.0f - elapsed / static_cast<float>(START_ERROR_MS));
}

float TitleScreen::startOffsetX() const {
    if (startErrorTime < 0) return 0.0f;
    const int elapsed = glutGet(GLUT_ELAPSED_TIME) - startErrorTime;
    // [11.7] 최대 12만큼 빠르게 흔들다가 원래 위치로 돌아온다.
    return std::sin(elapsed * 0.09f) * 12.0f * startErrorStrength();
}

bool TitleScreen::hitsStart(int x, int y, int width, int height) const {
    if (width <= 0 || height <= 0) return false;
    const float px = x * 1200.0f / width, py = y * 675.0f / height;
    const float halfWidth = 280.0f * startScale(), halfHeight = halfWidth / startAspect;
    return std::abs(px - (600.0f + startOffsetX())) <= halfWidth && std::abs(py - 535.0f) <= halfHeight;
}

void TitleScreen::renderLoginUI(GLuint shader, const char* name, const std::string& error, bool lobby) const {
    // [11.7] 배경과 같은 셰이더를 사용하고, 메뉴용 투영·투명도만 설정한다.
    const GLboolean depthEnabled = glIsEnabled(GL_DEPTH_TEST);
    const GLboolean blendEnabled = glIsEnabled(GL_BLEND);
    const GLboolean cullEnabled = glIsEnabled(GL_CULL_FACE);
    GLint srcRGB, dstRGB, srcAlpha, dstAlpha;
    glGetIntegerv(GL_BLEND_SRC_RGB, &srcRGB); glGetIntegerv(GL_BLEND_DST_RGB, &dstRGB);
    glGetIntegerv(GL_BLEND_SRC_ALPHA, &srcAlpha); glGetIntegerv(GL_BLEND_DST_ALPHA, &dstAlpha);
    glUseProgram(shader);
    glDisable(GL_DEPTH_TEST); glDisable(GL_CULL_FACE);
    glEnable(GL_BLEND); glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glActiveTexture(GL_TEXTURE0);
    glm::mat4 view(1.0f), projection = glm::ortho(0.0f, 1200.0f, 675.0f, 0.0f, -1.0f, 1.0f);
    glUniformMatrix4fv(glGetUniformLocation(shader, "view"), 1, GL_FALSE, &view[0][0]);
    glUniformMatrix4fv(glGetUniformLocation(shader, "projection"), 1, GL_FALSE, &projection[0][0]);
    glUniform1i(glGetUniformLocation(shader, "texture1"), 0);
    glUniform1i(glGetUniformLocation(shader, "uiMode"), 1);

    if (lobby) {
        panel(shader, 300, 380, 600, 222, 0.035f, 0.075f, 0.11f, 0.92f);
        centeredText(shader, "LOBBY", 425, 0.9f, 0.96f, 1, 24.0f);
        centeredText(shader, std::string("Welcome, ") + name, 468, 1, 1, 1);
        centeredText(shader, "Login successful", 510, 0.55f, 0.9f, 0.75f);
        centeredText(shader, "Room selection is coming next.", 556, 0.75f, 0.82f, 0.87f);
    }
    else {
        // [11.7] 칸에 닉네임과 깜빡이는 커서만 표시한다.
        panel(shader, 420, 400, 360, 50, 0.035f, 0.075f, 0.11f, 0.9f);
        std::string display = name;
        if ((glutGet(GLUT_ELAPSED_TIME) / 500) % 2 == 0) display += "_";
        centeredText(shader, display, 430, 1, 1, 1);
        const float halfWidth = 280.0f * startScale(), halfHeight = halfWidth / startAspect;
        // [11.7] 기존 uiColor로 붉은 기운을 주고 서서히 원래 색으로 돌린다.
        const float tint = 0.65f * startErrorStrength();
        if (startTextureId) {
            drawUIQuad(shader, 600+startOffsetX()-halfWidth, 535-halfHeight, halfWidth*2, halfHeight*2,
                startTextureId, glm::vec4(1, 1.0f-tint, 1.0f-tint, 1.0f),
                glm::vec4(startUV[0], startUV[1], startUV[2], startUV[3]));
        }
        else centeredText(shader, "GAME START", 540, 0.1f, 0.15f, 0.2f, 24.0f);
        panel(shader, 270, 604, 660, 32, 0.035f, 0.075f, 0.11f, 0.88f);
        const std::string status = error.empty() ? "Click GAME START or press Enter" : error;
        centeredText(shader, status, 625, 1, error.empty() ? 1.0f : 0.65f, error.empty() ? 1.0f : 0.6f);
    }
    // [11.7] 다음 배경·게임 출력에는 기존 셰이더 동작을 사용한다.
    glUniform1i(glGetUniformLocation(shader, "uiMode"), 0);
    glBindVertexArray(0); glBindTexture(GL_TEXTURE_2D, 0);
    if (depthEnabled) glEnable(GL_DEPTH_TEST);
    if (cullEnabled) glEnable(GL_CULL_FACE);
    if (!blendEnabled) glDisable(GL_BLEND);
    glBlendFuncSeparate(srcRGB, dstRGB, srcAlpha, dstAlpha);
}

// [11.10] 기존 셰이더와 사각형 그리기로 로비 이미지를 배치한다.
void TitleScreen::renderLobby(GLuint shader, const RoomSummary* rooms, int roomCount,
    int selectedRoomIndex, const std::string& error) const {
    if (!isInitialized) return;

    // [11.10] 화면 좌표와 PNG 투명도를 사용할 준비를 한다.
    const GLboolean depthEnabled = glIsEnabled(GL_DEPTH_TEST);
    const GLboolean blendEnabled = glIsEnabled(GL_BLEND);
    const GLboolean cullEnabled = glIsEnabled(GL_CULL_FACE);
    GLint srcRGB, dstRGB, srcAlpha, dstAlpha;
    glGetIntegerv(GL_BLEND_SRC_RGB, &srcRGB);
    glGetIntegerv(GL_BLEND_DST_RGB, &dstRGB);
    glGetIntegerv(GL_BLEND_SRC_ALPHA, &srcAlpha);
    glGetIntegerv(GL_BLEND_DST_ALPHA, &dstAlpha);
    glUseProgram(shader);
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_CULL_FACE);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glActiveTexture(GL_TEXTURE0);
    glm::mat4 view(1.0f);
    glm::mat4 projection = glm::ortho(0.0f, 1200.0f, 675.0f, 0.0f, -1.0f, 1.0f);
    glUniformMatrix4fv(glGetUniformLocation(shader, "view"), 1, GL_FALSE, &view[0][0]);
    glUniformMatrix4fv(glGetUniformLocation(shader, "projection"), 1, GL_FALSE, &projection[0][0]);
    glUniform1i(glGetUniformLocation(shader, "texture1"), 0);
    glUniform1i(glGetUniformLocation(shader, "uiMode"), 1);

    // [11.10] 배경 위에 제목과 세 칸의 방 버튼 이미지를 그린다.
    const glm::vec4 white(1.0f);
    if (lobbyBackgroundTextureId != 0) {
        drawUIQuad(shader, 0, 0, 1200, 675, lobbyBackgroundTextureId, white);
    }
    else {
        panel(shader, 0, 0, 1200, 675, 0.1f, 0.2f, 0.35f, 1.0f);
    }
    if (lobbyTextTextureId != 0) {
        drawUIQuad(shader, 330, 30.08f, 540, 181.44f, lobbyTextTextureId, white);
    }
    else {
        centeredText(shader, "LOBBY", 145, 1, 1, 1, 28.8f);
    }
    // [11.10] 세 방 버튼의 중심 위치를 유지하며 원래 크기의 약 73.1%로 그린다.
    // [11.10] 목록이 비어 있으면 현재 표시할 방이 없음을 알려 준다.
    if (rooms == nullptr || roomCount <= 0) {
        centeredText(shader, "No rooms available", 360, 1, 1, 1, 22.0f);
    }
    for (int i = 0; rooms != nullptr && i < roomCount && i < MAX_ROOMS; ++i) {
        const float y = 170.0f + i * 145.0f;
        // [11.11] 선택한 방만 중심을 유지하며 5% 크게 그린다.
        const bool selected = i == selectedRoomIndex;
        float scale = 1.0f;
        if (selected) scale = 1.05f;
        const float buttonWidth = 548.27f * scale;
        const float buttonHeight = 182.76f * scale;
        const float frameWidth = 504.41f * scale;
        const float frameHeight = 95.03f * scale;
        const float frameX = 600.0f - frameWidth / 2;
        const float frameY = y + 120.0f - frameHeight / 2;
        if (lobbyRoomButtonTextureId != 0) {
            drawUIQuad(shader, 600.0f - buttonWidth / 2, y + 125.0f - buttonHeight / 2,
                buttonWidth, buttonHeight, lobbyRoomButtonTextureId, white);
        }
        else {
            panel(shader, frameX, frameY, frameWidth, frameHeight, 0.1f, 0.15f, 0.2f, 0.9f);
        }
        if (selected) {
            // [11.11] 버튼 가장자리에 얇은 흰색 테두리 네 면을 그린다.
            const float outlineX = frameX - 4;
            const float outlineY = frameY - 4;
            const float outlineWidth = frameWidth + 8;
            const float outlineHeight = frameHeight + 8;
            panel(shader, outlineX, outlineY, outlineWidth, 2, 1, 1, 1, 0.9f);
            panel(shader, outlineX, outlineY + outlineHeight - 2, outlineWidth, 2, 1, 1, 1, 0.9f);
            panel(shader, outlineX, outlineY, 2, outlineHeight, 1, 1, 1, 0.9f);
            panel(shader, outlineX + outlineWidth - 2, outlineY, 2, outlineHeight, 1, 1, 1, 0.9f);
        }

        // [11.10] 서버에서 받은 방 번호, 인원, 상태를 버튼 중앙에 표시한다.
        const RoomSummary& room = rooms[i];
        const char* stateText = "Unknown";
        switch (room.state) {
        case ROOM_WAITING: stateText = "Waiting"; break;
        case ROOM_PLAYING: stateText = "Playing"; break;
        case ROOM_RESULT: stateText = "Result"; break;
        }
        const std::string roomText = "Room " + std::to_string(room.room_id) + "    " +
            std::to_string(room.player_count) + "/" + std::to_string(MAX_ROOM_PLAYERS) + "    " + stateText;
        centeredText(shader, roomText, y + 132.0f, 1, 1, 1, 22.0f);
    }

    // [11.11] 정상적인 입장 거절은 로비 아래쪽에 이유를 표시한다.
    if (!error.empty()) {
        centeredText(shader, error, 654, 1, 0.6f, 0.55f, 18.0f);
    }

    // [11.10] 로비를 그리기 전의 깊이·투명도 설정으로 돌린다.
    glUniform1i(glGetUniformLocation(shader, "uiMode"), 0);
    glBindVertexArray(0);
    glBindTexture(GL_TEXTURE_2D, 0);
    if (depthEnabled) glEnable(GL_DEPTH_TEST);
    if (cullEnabled) glEnable(GL_CULL_FACE);
    if (!blendEnabled) glDisable(GL_BLEND);
    glBlendFuncSeparate(srcRGB, dstRGB, srcAlpha, dstAlpha);
}

// [11.11] 서버가 확인한 방 번호와 자리 번호를 간단히 표시한다.
void TitleScreen::renderRoomEntry(GLuint shader, int roomId, int slot,
    const RoomPlayerInfo* players) const {
    if (!isInitialized) return;

    // [11.11] 화면 좌표와 PNG 투명도를 사용할 준비를 한다.
    const GLboolean depthEnabled = glIsEnabled(GL_DEPTH_TEST);
    const GLboolean blendEnabled = glIsEnabled(GL_BLEND);
    const GLboolean cullEnabled = glIsEnabled(GL_CULL_FACE);
    GLint srcRGB, dstRGB, srcAlpha, dstAlpha;
    glGetIntegerv(GL_BLEND_SRC_RGB, &srcRGB);
    glGetIntegerv(GL_BLEND_DST_RGB, &dstRGB);
    glGetIntegerv(GL_BLEND_SRC_ALPHA, &srcAlpha);
    glGetIntegerv(GL_BLEND_DST_ALPHA, &dstAlpha);
    glUseProgram(shader);
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_CULL_FACE);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glActiveTexture(GL_TEXTURE0);
    glm::mat4 view(1.0f);
    glm::mat4 projection = glm::ortho(0.0f, 1200.0f, 675.0f, 0.0f, -1.0f, 1.0f);
    glUniformMatrix4fv(glGetUniformLocation(shader, "view"), 1, GL_FALSE, &view[0][0]);
    glUniformMatrix4fv(glGetUniformLocation(shader, "projection"), 1, GL_FALSE, &projection[0][0]);
    glUniform1i(glGetUniformLocation(shader, "texture1"), 0);
    glUniform1i(glGetUniformLocation(shader, "uiMode"), 1);

    if (lobbyBackgroundTextureId != 0) {
        drawUIQuad(shader, 0, 0, 1200, 675, lobbyBackgroundTextureId, glm::vec4(1.0f));
    }
    else {
        panel(shader, 0, 0, 1200, 675, 0.1f, 0.2f, 0.35f, 1.0f);
    }
    // [11.12] 현재 방과 두 자리의 참가자 정보를 표시한다.
    panel(shader, 240, 150, 720, 375, 0.035f, 0.075f, 0.11f, 0.94f);
    centeredText(shader, "WAITING ROOM", 200, 1, 1, 1, 28.0f);
    const std::string location = "Room " + std::to_string(roomId) + "    Your slot: " + std::to_string(slot);
    centeredText(shader, location, 235, 0.75f, 0.88f, 1, 20.0f);
    for (int i = 0; i < MAX_ROOM_PLAYERS; ++i) {
        const float y = 260.0f + i * 120.0f;
        const bool mySlot = i == slot;
        panel(shader, 280, y, 640, 105, 0.1f, mySlot ? 0.23f : 0.15f, mySlot ? 0.33f : 0.2f, 0.95f);
        std::string seat = "Slot " + std::to_string(i);
        if (mySlot) seat += " (You)";
        centeredText(shader, seat, y + 25, 0.75f, 0.88f, 1, 18.0f);
        if (players == nullptr || !players[i].connected) {
            centeredText(shader, "Waiting for player...", y + 67, 0.7f, 0.75f, 0.8f, 20.0f);
            continue;
        }
        const RoomPlayerInfo& player = players[i];
        const char* characterText = "Not selected";
        switch (player.character) {
        case CHAR_STEVE: characterText = "Steve"; break;
        case CHAR_ALEX: characterText = "Alex"; break;
        }
        centeredText(shader, player.name, y + 58, 1, 1, 1, 22.0f);
        centeredText(shader, characterText, y + 88, 0.85f, 0.9f, 0.95f, 18.0f);
    }

    // [11.11] 입장 안내를 그리기 전의 깊이·투명도 설정으로 돌린다.
    glUniform1i(glGetUniformLocation(shader, "uiMode"), 0);
    glBindVertexArray(0);
    glBindTexture(GL_TEXTURE_2D, 0);
    if (depthEnabled) glEnable(GL_DEPTH_TEST);
    if (cullEnabled) glEnable(GL_CULL_FACE);
    if (!blendEnabled) glDisable(GL_BLEND);
    glBlendFuncSeparate(srcRGB, dstRGB, srcAlpha, dstAlpha);
}
