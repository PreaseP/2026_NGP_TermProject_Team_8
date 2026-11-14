#pragma once
#include <gl/glew.h>
#include <gl/glm/glm.hpp>
#include <string>

struct RoomSummary; // [11.10] 화면에서 읽을 방 요약 정보
struct RoomPlayerInfo; // [11.12] 화면에서 읽을 참가자 정보

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
        int selectedRoomIndex, const std::string& error) const; // [11.10] 로비 이미지와 방 정보를 그린다. [11.11] 선택 위치와 오류 안내도 받는다.
    void renderRoomEntry(GLuint shaderProgram, int roomId, int slot,
        const RoomPlayerInfo* players) const; // [11.11] 입장 성공과 지정된 방·자리를 표시한다. [11.12] 참가자 정보도 표시한다.
    GameState handleKeyInput(unsigned char key) const;
    void cleanup();
    // [11.7] 로그인 입력·버튼과 로그인 성공 후 로비 안내를 그린다.
    void renderLoginUI(GLuint shaderProgram, const char* name, const std::string& error, bool lobby) const;
    bool hitsStart(int x, int y, int width, int height) const;
    // [11.12] 마우스 위치를 기억하고 나가기 버튼 안인지 확인한다.
    void SetMousePosition(int x, int y);
    bool IsMouseOverBackButton(int x, int y, int width, int height) const;
    // [11.14] 화살표는 내 화면의 캐릭터 미리보기만 바꾼다.
    bool IsMouseOverCharacterArrow(int x, int y, int width, int height, bool left) const;
    void ChangeCharacterPreview(int direction, int serverCharacter);
    void ResetCharacterPreview();
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
    // [11.14] centerX를 생략하면 기존처럼 화면 중앙에 표시한다.
    void centeredText(GLuint shader, const std::string& text, float y, float r, float g, float b,
        float size = 18.0f, float centerX = 600.0f) const;
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
    GLuint lobbyRoomSelectedTextureId = 0; // [11.12] 선택된 방 버튼 이미지
    GLuint lobbyTextTextureId = 0; // [11.10] 로비 제목
    // [11.12] 대기실 나가기 버튼의 기본/마우스 올림 이미지
    GLuint roomBackTextureId = 0;
    GLuint roomBackHoverTextureId = 0;
    // [11.14] 제목 1개와 ROOM 1~3 이미지의 텍스처 번호
    GLuint roomTitleTextureId = 0;
    GLuint roomNumberTextureIds[3] = {};
    // [11.14] 나·상대 카드 프레임과 이름표
    GLuint roomPlayerFrameTextureId = 0;
    GLuint roomOpponentFrameTextureId = 0;
    GLuint roomPlayerLabelTextureId = 0;
    GLuint roomOpponentLabelTextureId = 0;
    // [11.14] 서버 캐릭터 값에 따라 표시할 이미지
    GLuint roomCharacterSteveTextureId = 0;
    GLuint roomCharacterAlexTextureId = 0;
    GLuint roomCharacterUnknownTextureId = 0;
    // [11.14] 화살표 배열은 왼쪽 0, 오른쪽 1이다.
    GLuint roomCharacterArrowTextureIds[2] = {};
    GLuint roomCharacterArrowHoverTextureIds[2] = {};
    int previewCharacter = -1; // -1이면 서버 값을 보되, 내 미선택은 Steve로 표시한다.
    const float CHARACTER_ARROW_LEFT_X = 210.0f;
    const float CHARACTER_ARROW_RIGHT_X = 430.0f;
    const float CHARACTER_ARROW_Y = 470.0f;
    const float CHARACTER_ARROW_SIZE = 60.0f;
    int roomPointerX = -1;
    int roomPointerY = -1;
    const float ROOM_BACK_X = 40.0f;
    const float ROOM_BACK_Y = 40.0f;
    const float ROOM_BACK_SIZE = 80.0f;
    bool textureLoaded = false;
};