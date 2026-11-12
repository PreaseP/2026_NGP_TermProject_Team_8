#pragma once
#include <gl/glew.h>
#include <gl/glm/glm.hpp>
#include <string>

struct RoomSummary; // [11.10] 화면에서 읽을 방 요약 정보

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
    void renderLobby(GLuint shaderProgram, const RoomSummary* rooms, int roomCount,
        int selectedRoomIndex) const; // [11.10] 로비 이미지와 방 정보를 그린다. [11.11] 선택 위치도 받는다.
    GameState handleKeyInput(unsigned char key) const;
    void cleanup();
    // [11.7] 로그인 입력·버튼과 로그인 성공 후 로비 안내를 그린다.
    void renderLoginUI(GLuint shaderProgram, const char* name, const std::string& error, bool lobby) const;
    bool hitsStart(int x, int y, int width, int height) const;
    void PlayNameError(); // [11.7] 버튼을 잠깐 붉게 만들고 좌우로 흔든다.

private:
    void initializeBuffers();
    void renderText(const std::string& text, float x, float y, float scale) const;
    void loadTitleTexture();
    void loadStartTexture(); // [11.7] 투명 PNG 버튼을 불러온다.
    void loadLobbyTexture(const char* path, GLuint& textureId); // [11.10] 로비 PNG를 텍스처로 준비한다.
    void loadFontTexture(); // [11.7] 글자도 같은 셰이더로 그리도록 텍스처로 준비한다.
    void drawUIQuad(GLuint shader, float x, float y, float w, float h,
        GLuint texture, const glm::vec4& color, const glm::vec4& uv = glm::vec4(0, 0, 1, 1)) const;
    void panel(GLuint shader, float x, float y, float w, float h, float r, float g, float b, float a) const;
    void centeredText(GLuint shader, const std::string& text, float y, float r, float g, float b, float size = 18.0f) const;
    GLuint fontTextureId = 0;
    float startScale() const; // [11.7] 표시와 클릭 검사에 같은 크기를 사용한다.
    float startErrorStrength() const; // [11.7] 남은 오류 효과의 세기: 1에서 0으로 줄어든다.
    float startOffsetX() const; // [11.7] 출력과 클릭 검사에 같은 흔들림을 사용한다.
    const int START_ERROR_MS = 450; // [11.7] 오류 효과 시간
    int startErrorTime = -1; // [11.7] 효과 시작 시각, -1이면 아직 발생하지 않았다.
    int startPausedTime = 0; // [11.7] 이전 오류 효과로 크기 변화를 멈춘 시간
    GLuint startTextureId = 0;
    float startUV[4] = {0, 0, 1, 1};
    float startAspect = 5.5f;

    GLuint vao;
    GLuint vbo;
    bool isInitialized;

    GLuint titleTextureId = 0;
    GLuint lobbyBackgroundTextureId = 0; // [11.10] 로비 배경
    GLuint lobbyRoomButtonTextureId = 0; // [11.10] 방 버튼
    GLuint lobbyTextTextureId = 0; // [11.10] 로비 제목
    bool textureLoaded = false;
};
