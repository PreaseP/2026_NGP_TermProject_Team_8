#define _CRT_SECURE_NO_WARNINGS
#define STB_IMAGE_IMPLEMENTATION

// [11.6] Winsock2가 OpenGL의 Windows 헤더보다 먼저 포함되도록 한다.
#include "GameClient.h"
#include <iostream>
#include <gl/glew.h>
#include <gl/freeglut.h>
#include <gl/freeglut_ext.h>
#include <gl/glm/glm.hpp>
#include <gl/glm/ext.hpp>
#include <gl/glm/gtc/matrix_transform.hpp>
#include <vector>
#include <utility>
#include <cmath>
#include <fstream>
#include <sstream>
#include <string>
#include <limits>
#include <algorithm>
#include <cstdlib>
#include "Map.h"
#include "Light.h"
#include "Steve.h"
#include "Alex.h"
#include "Camera.h"
#include "Steve_Camera.h"
#include "Alex_Camera.h"
#include "Snowball.h"
#include "Snow.h"
#include "KeyManager.h"
#include "title.h"
#include "finish.h"

#define WinX 1280
#define WinY 720

char* filetobuf(const char* file)
{
	FILE* fptr;
	long length;
	char* buf;

	fptr = fopen(file, "rb");
	if (!fptr)
		return NULL;
	fseek(fptr, 0, SEEK_END);
	length = ftell(fptr);
	buf = (char*)malloc(length + 1);
	fseek(fptr, 0, SEEK_SET);
	fread(buf, length, 1, fptr);
	fclose(fptr);
	buf[length] = 0;
	return buf;
}

glm::vec3 cameraPos = glm::vec3(5.0f, 8.0f, 12.0f);
glm::vec3 cameraFront = glm::vec3(0.0f, 0.0f, -1.0f);
glm::vec3 cameraUp = glm::vec3(0.0f, 1.0f, 0.0f);

static GameState currentGameState = GameState::TITLE_SCREEN;
static TitleScreen titleScreen;
static FinishScreen finishScreen;
static Winner winner = Winner::NONE;

// static Camera camera;
static Steve_Camera steveCamera;
static Alex_Camera alexCamera;
static LightManager lightManager;
static Map gameMap;
static Steve::Character* steve = nullptr;
static Alex::Character* alex = nullptr;
static Snow snowSystem("snow.png");
static KeyManager input;
// [11.6] 시작 함수와 타이머에서 같은 클라이언트를 사용한다.
static GameClient gameClient;

static Steve::Character* matchSteve[MAX_ROOM_PLAYERS]{};
static Alex::Character* matchAlex[MAX_ROOM_PLAYERS]{};
// [11.23] 모든 네트워크 눈덩이는 같은 구 모양으로 그린다.
static Snowball* matchSnowball = nullptr;
static Snow matchSnowWalls("snow.png"); // [11.26] 경기에서 받은 눈 벽을 그린다.
static GLuint landingMarkerVao = 0, landingMarkerVbo = 0;
static bool matchSceneInitialized = false;
static Part matchShield; // [12.1] 두 플레이어가 같은 방패 모양을 사용한다.
static Part shieldBlockRing; // [12.1] 방패가 공격을 막을 때 그릴 고리

static void ReleasePart(Part& part)
{
    glDeleteVertexArrays(1, &part.vao);
    glDeleteBuffers(1, &part.vbo);
    glDeleteBuffers(1, &part.nbo);
    glDeleteBuffers(1, &part.tbo);
    part = {};
}

template<class CharacterType>
void ReleaseCharacter(CharacterType*& character) {
    if (!character) return;
    for (Part* part : { &character->gBody, &character->gHead,
        &character->gArmL, &character->gArmR, &character->gLegL,
        &character->gLegR, &character->gBoundingBox }) {
        ReleasePart(*part);
    }
    glDeleteTextures(1, &character->textureID);
    delete character;
    character = nullptr;
}

void ClearMatchScene() {
    for (int slot = 0; slot < MAX_ROOM_PLAYERS; ++slot) {
        ReleaseCharacter(matchSteve[slot]);
        ReleaseCharacter(matchAlex[slot]);
    }
    // [11.23] 경기 화면을 정리할 때 눈덩이의 그래픽 자원도 비운다.
    delete matchSnowball;
    matchSnowball = nullptr;
    matchSnowWalls.cleanup(); // [11.26] 경기 전환과 창 종료 때 눈 벽을 정리한다.
    ReleasePart(matchShield); // [12.1] 경기 화면과 함께 방패 모양도 정리한다.
    ReleasePart(shieldBlockRing);
    // [11.24] 경기 화면과 함께 조준점 그리기 자료도 비운다.
    glDeleteVertexArrays(1, &landingMarkerVao);
    glDeleteBuffers(1, &landingMarkerVbo);
    landingMarkerVao = landingMarkerVbo = 0;
    matchSceneInitialized = false;
}

static bool steveThrowFlag = false;
static bool alexThrowFlag = false;

static glm::vec3 stevePosition = glm::vec3(3.0f, 0.8f, 5.0f);
static glm::vec3 alexPosition = glm::vec3(7.0f, 0.8f, 5.0f);

// static bool splitScreenMode = false;

bool cameraLightMode = true;
enum CharacterSelection { STEVE, ALEX };
static CharacterSelection activeCharacter = STEVE;

bool firstMouse = true;
float lastX = WinX / 2.0f;
float lastY = WinY / 2.0f;

static std::vector<Snowball> snowballs;

static bool steveCharging = false;
static float steveChargeStartTime = 0.0f;

static bool alexCharging = false;
static float alexChargeStartTime = 0.0f;

static float steveLastChargeTime = 0.0f;
static float alexLastChargeTime = 0.0f;


static float maxChargeTime = 1.0f;
static float minSpeed = 2.0f;
static float maxSpeed = 15.0f;

static GLuint faceVAO = 0, faceVBO = 0;
static bool faceBuffersInitialized = false;
static GLuint steveFaceTextureID = 0;
static GLuint alexFaceTextureID = 0;
static bool faceTexturesLoaded = false;

// [11.25] 경기 HUD에서 사용할 PNG를 한 번만 불러와 함께 사용한다.
struct GameUITextures {
    GLuint stevePortrait = 0;
    GLuint alexPortrait = 0;
    GLuint fullHeart = 0;
    GLuint emptyHeart = 0;
    GLuint fullHeartFlash = 0;
    GLuint emptyHeartFlash = 0;
    GLuint itemSlot = 0;
    GLuint itemShield = 0; // [11.30] 방어막 아이콘
    GLuint itemPowerUp = 0; // [11.30] 공격 강화 아이콘
};
static GameUITextures gameUITextures;
static GLuint powerUpBorderTexture = 0; // [12.1] 내 화면 가장자리
static GLuint powerUpAuraTexture = 0; // [12.1] 강화한 플레이어의 발밑
static GLuint poweredSnowballTexture = 0; // [12.1] 던진 강화 눈덩이의 붉은 눈 무늬

// [변경 1] 창과 그래픽 준비를 맡는 함수 선언. argc는 GLUT가 갱신할 수 있어 참조로 받는다.
bool InitOpenGL(int& argc, char** argv);

// [통합 종료] 창 닫기 콜백 전용 정리 함수.
void OnExit();

void OnMouseButton(int button, int state, int x, int y);
void OnMouseMove(int x, int y);
void OnMouseEnterLeave(int state); // [11.12] 창 밖으로 나가면 버튼 효과를 끈다.
void Keyboard(unsigned char key, int x, int y);
void KeyboardUp(unsigned char key, int x, int y);
void SpecialKeyboard(int key, int x, int y);
void SpecialKeyboardUp(int key, int x, int y);
void TimerFunction(int value);

void make_shaderProgram();
GLvoid Drawscene();
// GLvoid drawSplitScreen();
GLvoid renderWorld(const glm::mat4& view, const glm::mat4& projection);
GLvoid Reshape(int w, int h);

void initializeGame();

void fireSteveSnowball();
void fireAlexSnowball();

bool checkSnowballCharacterCollision(const Snowball& snowball, const glm::vec3& characterPos, const glm::vec3& boundingBoxSize);
void checkAllSnowballCollisions();

void initializeFaceBuffers();
void loadFaceTextures();
GLuint loadTexture(const char* path, GLint minFilter = GL_LINEAR, bool whiteFallback = true);
void renderCharacterFace(GLuint textureID, float x, float y, float size, int screenWidth, int screenHeight);
// void drawCharacterFaces(int winW, int winH);

//--- 셰이더 관련 변수들
GLint width, height;
GLuint shaderProgramID;
GLuint vertexShader;
GLuint fragmentShader;
GLchar* vertexSource, * fragmentSource;

// [변경 2 시작] 원래 main() 안에 있던 창 생성 ~ 타이틀 초기화를 그대로 옮겼다.
// 준비에 성공하면 true, 실패하면 false를 반환한다. 게임 진행 로직은 포함하지 않는다.
bool InitOpenGL(int& argc, char** argv)
{
	//--- 윈도우생성하기
	glutInit(&argc, argv);
	glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGBA | GLUT_DEPTH);
	glutInitWindowPosition(100, 100);
	glutInitWindowSize(WinX, WinY);
	glutCreateWindow("Minecraft Snowball Fight");
	// [종료 변경 2] X로 닫아도 프로세스를 즉시 끝내지 않고 main()으로 돌아오게 한다.
	glutSetOption(GLUT_ACTION_ON_WINDOW_CLOSE, GLUT_ACTION_GLUTMAINLOOP_RETURNS);

	std::cout << "GLUT 초기화 완료" << std::endl;

	//--- GLEW 초기화하기
	glewExperimental = GL_TRUE;
	GLenum glewStatus = glewInit();
	if (glewStatus != GLEW_OK) {
		std::cerr << "GLEW 초기화 실패: " << glewGetErrorString(glewStatus) << std::endl;
		return false;
	}
	// [통합 종료 1] GL 함수가 준비된 뒤, 실제 정리는 창 닫기 콜백 한 곳에만 등록한다.
	// [11.3] 창 X로 닫으면 freeglut이 이 콜백을 호출한다.
	glutCloseFunc(OnExit);

	// OpenGL 버전 확인
	std::cout << "OpenGL Version: " << glGetString(GL_VERSION) << std::endl;
	std::cout << "GLSL Version: " << glGetString(GL_SHADING_LANGUAGE_VERSION) << std::endl;

	//--- 세이더읽어와서세이더프로그램만들기
	make_shaderProgram();

	// 셰이더 프로그램이 제대로 생성되었는지 확인
	if (shaderProgramID == 0) {
		std::cerr << "셰이더 프로그램 생성 실패" << std::endl;
		return false;
	}

	std::cout << "셰이더 프로그램 생성 완료" << std::endl;

	// 타이틀 화면 초기화
	titleScreen.initialize();
	// [11.25] 경기 화면 위에 올릴 이미지도 미리 불러온다.
	gameUITextures.stevePortrait = loadTexture("Images/Hud/portrait_steve.png");
	gameUITextures.alexPortrait = loadTexture("Images/Hud/portrait_alex.png");
	gameUITextures.fullHeart = loadTexture("Images/Hud/heart_full.png");
	// [11.25] 줄어든 체력을 표시할 빈 하트도 준비한다.
	gameUITextures.emptyHeart = loadTexture("Images/Hud/heart_empty.png");
	// [11.25] 피격으로 깜빡일 때 쓸 흰 테두리 하트를 준비한다.
	gameUITextures.fullHeartFlash = loadTexture("Images/Hud/heart_full_flash.png");
	gameUITextures.emptyHeartFlash = loadTexture("Images/Hud/heart_empty_flash.png");
	gameUITextures.itemSlot = loadTexture("Images/Hud/item_slot.png");
	// [11.30] HUD 크기로 줄일 때 밉맵으로 아이콘을 그린다.
	gameUITextures.itemShield = loadTexture("Images/Hud/item_shield.png", GL_LINEAR_MIPMAP_LINEAR);
	gameUITextures.itemPowerUp = loadTexture("Images/Hud/item_power_up.png", GL_LINEAR_MIPMAP_LINEAR);
	// [12.1] 공격 강화가 켜져 있는 동안 그릴 두 이미지를 준비한다.
	powerUpBorderTexture = loadTexture("Images/Item/item_border_power_up.png", GL_LINEAR, false);
	powerUpAuraTexture = loadTexture("Images/Item/item_aura_power_up.png", GL_LINEAR_MIPMAP_LINEAR, false);
	poweredSnowballTexture = loadTexture("Images/Item/snow_power_up.png", GL_LINEAR_MIPMAP_LINEAR, false);
	return true;
}

// [변경 2 끝]

// [통합 종료] freeglut이 창을 닫을 때 한 번 호출한다. 직접 호출하지 않는다.
void OnExit()
{
	gameClient.Shutdown(); // [11.9] 창이 닫히면 네트워크부터 정리한다.

	input.clearAll();
	steveCharging = alexCharging = false;
	steveThrowFlag = alexThrowFlag = false;

	snowballs.clear();
	snowSystem.cleanup();
	titleScreen.cleanup();
	finishScreen.cleanup();

	// 기존 캐릭터에는 GL 자원 소멸자가 없으므로 종료 시 여기서 회수한다.
	ReleaseCharacter(steve);
	ReleaseCharacter(alex);
	ClearMatchScene();

	// Block의 기존 대입 연산자가 이전 GL 자원을 해제한다.
	// 빈 블록으로 바꿔 정적 객체 소멸 시 닫힌 창의 GL 자원을 다시 건드리지 않게 한다.
	for (Ground* ground : { &gameMap.getFrontGround(), &gameMap.getBackGround() }) {
		for (int x = 0; x < ground->getWidth(); ++x)
			for (int z = 0; z < ground->getDepth(); ++z)
				ground->getBlock(x, z) = Block();
	}
	gameMap.getWall().clear();

	glDeleteVertexArrays(1, &faceVAO);
	glDeleteBuffers(1, &faceVBO);
	glDeleteTextures(1, &steveFaceTextureID);
	glDeleteTextures(1, &alexFaceTextureID);
	// [11.25] 창이 닫힐 때 HUD 이미지도 정리한다.
	const GLuint textureIds[] = { gameUITextures.stevePortrait, gameUITextures.alexPortrait,
		gameUITextures.fullHeart, gameUITextures.emptyHeart, gameUITextures.fullHeartFlash,
		gameUITextures.emptyHeartFlash, gameUITextures.itemSlot, gameUITextures.itemShield,
		gameUITextures.itemPowerUp, powerUpBorderTexture, powerUpAuraTexture, poweredSnowballTexture };
	glDeleteTextures(static_cast<GLsizei>(sizeof(textureIds) / sizeof(textureIds[0])), textureIds);
	gameUITextures = {};
	faceVAO = faceVBO = steveFaceTextureID = alexFaceTextureID = 0;
	faceBuffersInitialized = faceTexturesLoaded = false;

	glUseProgram(0);
	if (shaderProgramID) glDeleteProgram(shaderProgramID);
	if (vertexShader) glDeleteShader(vertexShader);
	if (fragmentShader) glDeleteShader(fragmentShader);
	shaderProgramID = vertexShader = fragmentShader = 0;
	std::free(vertexSource);
	std::free(fragmentSource);
	vertexSource = fragmentSource = nullptr;

	std::cout << "종료 정리 완료" << std::endl;
}

// [종료 변경 3 끝]

int main(int argc, char** argv)
{
	std::cout << "프로그램 시작" << std::endl;

	// [변경 3] 길게 있던 초기화 코드 대신 위 함수를 호출한다. 실패 시 기존처럼 종료한다.
	if (!InitOpenGL(argc, argv)) {
		// [통합 종료 2] 초기화 실패도 창 닫기 경로로 처리한다.
		glutDestroyWindow(glutGetWindow());
		glutMainLoopEvent(); // 예약된 창 닫기를 처리하고, 등록되어 있으면 OnExit 호출
		return 1;
	}

	// [11.6] 일단 같은 PC의 서버에 접속한다. 팀 서버 연결 시 주소 변경
	if (!gameClient.Init(gameClient.GetServerAddress())) {
		std::cerr << "클라이언트 네트워크 초기화 실패" << std::endl;
	}

	std::cout << "\n=== MINECRAFT SNOWBALL FIGHT ===" << std::endl;
	std::cout << "닉네임 입력 후 GAME START 클릭 또는 Enter: 로그인" << std::endl; // [11.7]
	std::cout << "종료: 창의 X 버튼" << std::endl;

	//--- 콜백 등록
	glutDisplayFunc(Drawscene);
	glutReshapeFunc(Reshape);
	glutKeyboardFunc(Keyboard);
	glutKeyboardUpFunc(KeyboardUp);
	glutSpecialFunc(SpecialKeyboard);
	glutSpecialUpFunc(SpecialKeyboardUp);
	glutMouseFunc(OnMouseButton);
	glutPassiveMotionFunc(OnMouseMove);
	glutMotionFunc(OnMouseMove); // [11.12] 누른 채 움직일 때도 위치 갱신
	glutEntryFunc(OnMouseEnterLeave);
	glutTimerFunc(16, TimerFunction, 1);
	glutMainLoop();

	return 0;
}

GLuint loadTexture(const char* path, GLint minFilter, bool whiteFallback)
{
	GLuint textureID;
	glGenTextures(1, &textureID);
	glBindTexture(GL_TEXTURE_2D, textureID);

	// 텍스처 파라미터 설정
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, minFilter);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

	int width, height, channels;
	stbi_set_flip_vertically_on_load(true);
	unsigned char* data = stbi_load(path, &width, &height, &channels, 0);

	if (data) {
		GLenum format;
		if (channels == 1)
			format = GL_RED;
		else if (channels == 3)
			format = GL_RGB;
		else if (channels == 4)
			format = GL_RGBA;
		else
			format = GL_RGB;

		// [12.1] PNG가 넘긴 픽셀은 행 사이에 빈 바이트가 없이 붙어 있다.
		GLint previousAlignment;
		glGetIntegerv(GL_UNPACK_ALIGNMENT, &previousAlignment);
		glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
		glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);
		glPixelStorei(GL_UNPACK_ALIGNMENT, previousAlignment);
		glGenerateMipmap(GL_TEXTURE_2D);

		std::cout << "텍스처 로드 성공: " << path << " (" << width << "x" << height << ", " << channels << " channels)" << std::endl;
	}
	else {
		std::cerr << "텍스처 로드 실패: " << path << std::endl;
		if (!whiteFallback) {
			glDeleteTextures(1, &textureID);
			return 0;
		}
		unsigned char whitePixel[3] = { 255, 255, 255 };
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, 1, 1, 0, GL_RGB, GL_UNSIGNED_BYTE, whitePixel);
	}

	stbi_image_free(data);
	glBindTexture(GL_TEXTURE_2D, 0);

	return textureID;
}

void loadFaceTextures()
{
	if (faceTexturesLoaded) return;

	steveFaceTextureID = loadTexture("steve_face.png");
	alexFaceTextureID = loadTexture("alex_face.png");

	faceTexturesLoaded = true;
	std::cout << "얼굴 텍스처 로드 완료" << std::endl;
}

void initializeFaceBuffers()
{
	if (faceBuffersInitialized) return;

	float quadVertices[] = {
		-1.0f,  1.0f,  0.0f, 1.0f,
		-1.0f, -1.0f,  0.0f, 0.0f,
		 1.0f, -1.0f,  1.0f, 0.0f,

		-1.0f,  1.0f,  0.0f, 1.0f,
		 1.0f, -1.0f,  1.0f, 0.0f,
		 1.0f,  1.0f,  1.0f, 1.0f
	};

	glGenVertexArrays(1, &faceVAO);
	glGenBuffers(1, &faceVBO);

	glBindVertexArray(faceVAO);

	glBindBuffer(GL_ARRAY_BUFFER, faceVBO);
	glBufferData(GL_ARRAY_BUFFER, sizeof(quadVertices), quadVertices, GL_STATIC_DRAW);

	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);

	glEnableVertexAttribArray(2);
	glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(2 * sizeof(float)));

	glBindVertexArray(0);

	faceBuffersInitialized = true;
	std::cout << "얼굴 렌더링 버퍼 초기화 완료" << std::endl;
}

void renderCharacterFace(GLuint textureID, float x, float y, float size, int screenWidth, int screenHeight)
{
	if (!faceBuffersInitialized) {
		initializeFaceBuffers();
	}

	glDisable(GL_DEPTH_TEST);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, textureID);

	glm::mat4 projection = glm::ortho(0.0f, (float)screenWidth, 0.0f, (float)screenHeight);
	glm::mat4 view = glm::mat4(1.0f);
	glm::mat4 model = glm::mat4(1.0f);

	model = glm::translate(model, glm::vec3(x, y, 0.0f));
	model = glm::scale(model, glm::vec3(size, size, 1.0f));

	GLint modelLoc = glGetUniformLocation(shaderProgramID, "model");
	GLint viewLoc = glGetUniformLocation(shaderProgramID, "view");
	GLint projLoc = glGetUniformLocation(shaderProgramID, "projection");
	GLint useTextureLoc = glGetUniformLocation(shaderProgramID, "useTexture");
	GLint lightingEnabledLoc = glGetUniformLocation(shaderProgramID, "lightingEnabled");
	GLint vColorLoc = glGetUniformLocation(shaderProgramID, "vColor");
	GLint textureLoc = glGetUniformLocation(shaderProgramID, "texture1");

	if (modelLoc != -1) glUniformMatrix4fv(modelLoc, 1, GL_FALSE, &model[0][0]);
	if (viewLoc != -1) glUniformMatrix4fv(viewLoc, 1, GL_FALSE, &view[0][0]);
	if (projLoc != -1) glUniformMatrix4fv(projLoc, 1, GL_FALSE, &projection[0][0]);
	if (useTextureLoc != -1) glUniform1i(useTextureLoc, 1);
	if (lightingEnabledLoc != -1) glUniform1i(lightingEnabledLoc, 0);
	if (vColorLoc != -1) glUniform3f(vColorLoc, 1.0f, 1.0f, 1.0f);
	if (textureLoc != -1) glUniform1i(textureLoc, 0);

	glBindVertexArray(faceVAO);
	glDrawArrays(GL_TRIANGLES, 0, 6);
	glBindVertexArray(0);

	glEnable(GL_DEPTH_TEST);
	glDisable(GL_BLEND);
}

// void drawCharacterFaces(int winW, int winH)
// {
// 	if (!splitScreenMode || currentGameState != GameState::FIRST_PERSON_MODE) return;
// 	if (!faceTexturesLoaded) return;
// 	const float faceSize = 40.0f;
// 	const float margin = 40.0f;
// 	float steveX = margin;
// 	float steveY = winH - faceSize - margin;
// 	float alexX = margin;
// 	float alexY = winH - faceSize - margin;
// 	glViewport(0, 0, winW / 2, winH);
// 	renderCharacterFace(steveFaceTextureID, steveX, steveY, faceSize, winW / 2, winH);
// 	glViewport(winW / 2, 0, winW - winW / 2, winH);
// 	renderCharacterFace(alexFaceTextureID, alexX, alexY, faceSize, winW - winW / 2, winH);
// 	glViewport(0, 0, winW, winH);
// }

constexpr ULONGLONG powerUpPulsePeriodMs = 1500;

// [12.1] 현재 시각을 한 주기 안의 각도로 바꾼다.
static float PowerUpAnimationAngle(ULONGLONG nowMs, ULONGLONG periodMs)
{
    return glm::two_pi<float>() * static_cast<float>(nowMs % periodMs) / static_cast<float>(periodMs);
}

// [11.25] 경기 화면 위에 내 초상화, 하트 5개, 빈 아이템 틀을 그린다.
static void DrawGameUI(CHARACTER_TYPE character, int hp, int winWidth, int winHeight, bool powerUpActive)
{
    initializeFaceBuffers();

    // [11.25] 1200×675 배치를 기준으로 창 크기에 맞춰 같은 비율로 줄인다.
    const float scale = std::min(winWidth / 1200.0f, winHeight / 675.0f);
    const glm::mat4 projection = glm::ortho(0.0f, float(winWidth), 0.0f, float(winHeight));
    const glm::mat4 view(1.0f);
    const GLint modelLoc = glGetUniformLocation(shaderProgramID, "model");
    const GLint uiModeLoc = glGetUniformLocation(shaderProgramID, "uiMode");
    const GLint uiColorLoc = glGetUniformLocation(shaderProgramID, "uiColor");

    const GLboolean blendEnabled = glIsEnabled(GL_BLEND);
    GLint srcRgb, dstRgb, srcAlpha, dstAlpha;
    glGetIntegerv(GL_BLEND_SRC_RGB, &srcRgb);
    glGetIntegerv(GL_BLEND_DST_RGB, &dstRgb);
    glGetIntegerv(GL_BLEND_SRC_ALPHA, &srcAlpha);
    glGetIntegerv(GL_BLEND_DST_ALPHA, &dstAlpha);
    glDisable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    glUniformMatrix4fv(glGetUniformLocation(shaderProgramID, "projection"), 1, GL_FALSE, &projection[0][0]);
    glUniformMatrix4fv(glGetUniformLocation(shaderProgramID, "view"), 1, GL_FALSE, &view[0][0]);
    glUniform1i(uiModeLoc, 1);
    glUniform1i(glGetUniformLocation(shaderProgramID, "useTexture"), 1);
    glUniform4f(glGetUniformLocation(shaderProgramID, "uiUVRect"), 0.0f, 0.0f, 1.0f, 1.0f);
    glUniform4f(uiColorLoc, 1.0f, 1.0f, 1.0f, 1.0f);
    glActiveTexture(GL_TEXTURE0);
    glBindVertexArray(faceVAO);

    const auto drawImage = [&](GLuint texture, float left, float top, float width, float height) {
        const float halfWidth = width / 2.0f;
        const float halfHeight = height / 2.0f;
        glm::mat4 model = glm::translate(glm::mat4(1.0f),
            glm::vec3(left + halfWidth, winHeight - top - halfHeight, 0.0f));
        model = glm::scale(model, glm::vec3(halfWidth, halfHeight, 1.0f));
        glUniformMatrix4fv(modelLoc, 1, GL_FALSE, &model[0][0]);
        glBindTexture(GL_TEXTURE_2D, texture);
        glDrawArrays(GL_TRIANGLES, 0, 6);
    };

    // [12.1] 내 강화가 켜져 있으면 창 전체에 테두리 이미지를 깔고 그 위에 HUD를 그린다.
    if (powerUpActive && powerUpBorderTexture != 0) {
        // [12.1] 테두리의 알파 배율만 0.8에서 1.0 사이로 바꾼다.
        const float angle = PowerUpAnimationAngle(GetTickCount64(), powerUpPulsePeriodMs);
        const float opacity = 0.9f + 0.1f * std::sin(angle);
        glUniform4f(uiColorLoc, 1.0f, 1.0f, 1.0f, opacity);
        drawImage(powerUpBorderTexture, 0.0f, 0.0f, float(winWidth), float(winHeight));
        glUniform4f(uiColorLoc, 1.0f, 1.0f, 1.0f, 1.0f);
    }

    // [11.25] 왼쪽 위 기준 위치를 기존 사각형의 중심 좌표로 바꾼다.
    const auto drawIcon = [&](GLuint texture, float left, float top, float size) {
        drawImage(texture, left * scale, top * scale, size * scale, size * scale);
    };

    const GLuint portrait = character == CHAR_STEVE ? gameUITextures.stevePortrait : gameUITextures.alexPortrait;
    drawIcon(portrait, 22.0f, 24.0f, 96.0f);
    // [11.25] 첫 하트의 위치를 맞추고 하트끼리 가깝게 붙인다.
    constexpr float heartSize = 56.0f;
    constexpr float heartSpacing = 55.0f;
    constexpr int heartCount = 5;
    constexpr int hpPerHeart = MAX_HP / heartCount;
    const bool flashHearts = gameClient.ShouldFlashHearts();
    const GLuint fullHeart = flashHearts ? gameUITextures.fullHeartFlash : gameUITextures.fullHeart;
    const GLuint emptyHeart = flashHearts ? gameUITextures.emptyHeartFlash : gameUITextures.emptyHeart;
    for (int heart = 0; heart < heartCount; ++heart) {
        // [11.25] 이 칸에 남은 체력이 있으면 가득 찬 하트를 그린다.
        const GLuint texture = hp > heart * hpPerHeart ? fullHeart : emptyHeart;
        drawIcon(texture, 142.5f + heart * heartSpacing, 40.0f, heartSize);
    }
    constexpr float itemSlotLeft = 22.0f;
    constexpr float itemSlotTop = 134.0f;
    constexpr float itemSlotSize = 96.0f;
    drawIcon(gameUITextures.itemSlot, itemSlotLeft, itemSlotTop, itemSlotSize);

    // [11.30] 보유한 아이템의 아이콘을 칸 가운데에 그린다.
    ITEM_TYPE heldItem{};
    const bool hasItem = gameClient.TryGetHeldItem(heldItem);
    if (hasItem) {
        constexpr float iconInset = 8.0f;
        const GLuint icon = heldItem == ITEM_SHIELD ? gameUITextures.itemShield : gameUITextures.itemPowerUp;
        drawIcon(icon, itemSlotLeft + iconInset, itemSlotTop + iconInset, itemSlotSize - 2.0f * iconInset);
    }

    // [11.27] 지급 주기를 작은 숫자와 차오르는 막대로 표시한다.
    const int remainingMs = gameClient.GetItemGrantRemainingMs();
    const int seconds = (remainingMs + 999) / 1000;
    const float progress = 1.0f - remainingMs / static_cast<float>(ITEM_INTERVAL_MS);
    const glm::vec4 gaugeColor = seconds <= 3
        ? glm::vec4(0.57f, 0.91f, 0.77f, 1.0f)
        : glm::vec4(0.90f, 0.66f, 0.32f, 1.0f);
    const auto drawPanel = [&](float left, float top, float width, float height, const glm::vec4& color) {
        glm::mat4 model = glm::translate(glm::mat4(1.0f),
            glm::vec3((left + width / 2.0f) * scale, winHeight - (top + height / 2.0f) * scale, 0.0f));
        model = glm::scale(model, glm::vec3(width * scale / 2.0f, height * scale / 2.0f, 1.0f));
        glUniformMatrix4fv(modelLoc, 1, GL_FALSE, &model[0][0]);
        glUniform4fv(uiColorLoc, 1, &color[0]);
        glDrawArrays(GL_TRIANGLES, 0, 6);
    };
    glUniform1i(glGetUniformLocation(shaderProgramID, "useTexture"), 0);
    drawPanel(18.0f, 236.0f, 104.0f, 42.0f, glm::vec4(0.08f, 0.09f, 0.11f, 0.92f));
    drawPanel(26.0f, 265.0f, 88.0f, 5.0f, glm::vec4(0.23f, 0.25f, 0.29f, 1.0f));
    drawPanel(26.0f, 265.0f, 88.0f * progress, 5.0f, gaugeColor);

    // [11.27] 기존 메뉴의 영문 글꼴을 HUD 크기에 맞춰 재사용한다.
    const glm::mat4 textProjection = glm::ortho(0.0f, float(winWidth), float(winHeight), 0.0f);
    glUniformMatrix4fv(glGetUniformLocation(shaderProgramID, "projection"), 1, GL_FALSE, &textProjection[0][0]);
    titleScreen.centeredText(shaderProgramID, "NEXT " + std::to_string(seconds) + "s",
        258.0f * scale, 0.95f, 0.96f, 0.98f, 18.0f * scale, 70.0f * scale);
    glBindVertexArray(0);
    glBindTexture(GL_TEXTURE_2D, 0);
    glUniform1i(uiModeLoc, 0);
    glBlendFuncSeparate(srcRgb, dstRgb, srcAlpha, dstAlpha);
    if (!blendEnabled) glDisable(GL_BLEND);
    glEnable(GL_DEPTH_TEST);
}

void initializeGame() {
	lightManager.setupDefaultLighting();

	gameMap.initialize();
	std::cout << "맵 초기화 완료" << std::endl;

	steve = new Steve::Character("steve.png");
	std::cout << "Steve 캐릭터 초기화 완료" << std::endl;

	alex = new Alex::Character("alex.png");
	std::cout << "Alex 캐릭터 초기화 완료" << std::endl;

	snowSystem.clearAll();
	std::cout << "눈 초기화 완료" << std::endl;

	initializeFaceBuffers();
	loadFaceTextures();

// 	if (currentGameState == GameState::FIRST_PERSON_MODE) {
// 		splitScreenMode = true;
// 		std::cout << "\n=== 1인칭 모드 조작법 ===" << std::endl;
// 		std::cout << "V: 분할 화면 모드 토글" << std::endl;
// 		std::cout << "WASD: Steve 1인칭 시점 회전" << std::endl;
// 		std::cout << "화살표 키: Alex 1인칭 시점 회전" << std::endl;
// 		std::cout << "IJKL: 선택된 캐릭터 이동 (앞/왼/뒤/오른)" << std::endl;
// 		std::cout << "TAB: 캐릭터 선택 (Steve/Alex)" << std::endl;
// 	}
// 	else {
// 		splitScreenMode = false;
// 		std::cout << "\n=== 3인칭 모드 조작법 ===" << std::endl;
// 		std::cout << "V: 분할 화면 모드 토글" << std::endl;
// 		std::cout << "화살표 키: 카메라 회전" << std::endl;
// 		std::cout << "IJKL: 선택된 캐릭터 이동 (앞/왼/뒤/오른)" << std::endl;
// 		std::cout << "TAB: 캐릭터 선택 (Steve/Alex)" << std::endl;
// 	}

	std::cout << "E: Steve 눈덩이 발사" << std::endl;
	std::cout << "O: Alex 눈덩이 발사" << std::endl;
	std::cout << "X: 모든 눈 제거" << std::endl;
	std::cout << "종료: 창의 X 버튼" << std::endl;

	finishScreen.initialize();
}

void Keyboard(unsigned char key, int x, int y)
{
	if (gameClient.GetState() == CLIENT_PLAYING) {
		gameClient.OnKeyDown(key);
		return;
	}
	// [11.6]
	// [11.7] 로비에서 기존 로컬 게임 시작을 막는다.
	if (gameClient.GetState() == CLIENT_LOGIN || gameClient.GetState() == CLIENT_LOBBY) { // [11.11] Enter 입력 전달
		gameClient.OnKeyDown(key);
		glutPostRedisplay();
		return;
	}
	// [11.11] 대기실 입력은 다음 단계에서 연결한다.
	if (gameClient.GetState() == CLIENT_ROOM) { // [11.12] Esc 퇴장 입력을 전달한다.
		gameClient.OnKeyDown(key);
		glutPostRedisplay();
		return;
	}

	if (currentGameState == GameState::TITLE_SCREEN) {
		GameState newState = titleScreen.handleKeyInput(key);
		if (newState != GameState::TITLE_SCREEN) {
			currentGameState = newState;
			initializeGame();
		}
		glutPostRedisplay();
		return;
	}

	if (currentGameState == GameState::FINISH_SCREEN) {
		if (key == 'r' || key == 'R') {
			if (steve) { delete steve; steve = nullptr; }
			if (alex) { delete alex; alex = nullptr; }
			snowballs.clear();
			snowSystem.clearAll();

			winner = Winner::NONE;
			currentGameState = GameState::TITLE_SCREEN;

			titleScreen.initialize();

			glutPostRedisplay();
		}
		return;
	}

	input.pressKey(static_cast<int>(key));

	switch (key) {
// 	case 'v': case 'V':
// 		splitScreenMode = !splitScreenMode;
// 		std::cout << "분할 화면 모드: " << (splitScreenMode ? "ON" : "OFF") << std::endl;
// 		break;

	case 9:
		activeCharacter = (activeCharacter == STEVE) ? ALEX : STEVE;
		std::cout << "현재 선택된 캐릭터: " << (activeCharacter == STEVE ? "Steve" : "Alex") << std::endl;
		break;
	case 'e': case 'E': // Steve 눈덩이 차징 시작
		if (!steveCharging && steve && steve->armState < 2) {
			steveCharging = true;
			steveChargeStartTime = glutGet(GLUT_ELAPSED_TIME) / 1000.0f;
			steve->enterCharge();
			std::cout << "Steve 던지기 준비..." << std::endl;
		}
		break;
	case 'o': case 'O': // Alex 눈덩이 차징 시작
		if (!alexCharging && alex && alex->armState < 2) {
			alexCharging = true;
			alexChargeStartTime = glutGet(GLUT_ELAPSED_TIME) / 1000.0f;
			alex->enterCharge();
			std::cout << "Alex 던지기 준비..." << std::endl;
		}
		break;
	case 'x': case 'X':
		snowSystem.clearAll();
		std::cout << "모든 눈 제거됨" << std::endl;
		break;
	default:
		break;
	}

	glutPostRedisplay();
}

void KeyboardUp(unsigned char key, int x, int y)
{
	if (gameClient.GetState() == CLIENT_PLAYING) {
		gameClient.OnKeyUp(key);
		return;
	}
	// [11.6]
	if (gameClient.GetState() == CLIENT_LOGIN) return;

	if (currentGameState == GameState::TITLE_SCREEN) {
		return;
	}

	input.releaseKey(static_cast<int>(key));

	switch (key) {
	case 'e': case 'E': // Steve 눈덩이 발사
		if (steveCharging) {
			fireSteveSnowball();
			if (steve) steve->enterThrow();
		}
		break;
	case 'o': case 'O': // Alex 눈덩이 발사
		if (alexCharging) {
			fireAlexSnowball();
			if (alex) alex->enterThrow();
		}
		break;
	}

	glutPostRedisplay();
}


void fireSteveSnowball()
{
    if (!steve) return;
    float currentTime = glutGet(GLUT_ELAPSED_TIME) / 1000.0f;
    float chargeTime = currentTime - steveChargeStartTime;
    float chargeRatio = std::min(chargeTime / maxChargeTime, 1.0f);

    float speed = minSpeed + (maxSpeed - minSpeed) * chargeRatio;
    speed *= steve->throwingSpeed;
    speed = std::min(speed, maxSpeed);

    glm::vec3 shootDirection = steveCamera.getFront();
    glm::vec3 startPos = steve->pos + shootDirection * 1.0f + glm::vec3(0, 1, 0) * 0.5f;
    glm::vec3 direction = shootDirection + glm::vec3(0, 1, 0) * 0.3f;

    Snowball newSnowball(startPos, direction, speed, 0.15f);
    snowballs.push_back(newSnowball);
    steveCharging = false;
}

void fireAlexSnowball()
{
	if (!alex) return;

	float currentTime = glutGet(GLUT_ELAPSED_TIME) / 1000.0f;
	float chargeTime = currentTime - alexChargeStartTime;
	float chargeRatio = std::min(chargeTime / maxChargeTime, 1.0f);
	float speed = minSpeed + (maxSpeed - minSpeed) * chargeRatio;

	speed *= alex->throwingSpeed;
	speed = std::min(speed, maxSpeed);

	glm::vec3 shootDirection = alexCamera.getFront();
	glm::vec3 startPos = alex->pos + shootDirection * 1.0f + glm::vec3(0, 1, 0) * 0.5f;
	glm::vec3 direction = shootDirection + glm::vec3(0, 1, 0) * 0.3f;

	Snowball newSnowball(startPos, direction, speed, 0.15f);
	snowballs.push_back(newSnowball);

	std::cout << "Alex 눈덩이 발사! 차징 시간: " << chargeTime << "초, 속도: " << speed
		<< " (현재 " << snowballs.size() << "개)" << std::endl;

	alexCharging = false;
}

bool checkSnowballCharacterCollision(const Snowball& snowball, const glm::vec3& characterPos, const glm::vec3& boundingBoxSize)
{
	if (!snowball.getIsActive()) return false;

	glm::vec3 snowballPos = snowball.getPosition();
	float snowballRadius = snowball.getRadius();

	glm::vec3 characterMin = characterPos - boundingBoxSize / 2.0f;
	glm::vec3 characterMax = characterPos + boundingBoxSize / 2.0f;

	float closestX = std::max(characterMin.x, std::min(snowballPos.x, characterMax.x));
	float closestY = std::max(characterMin.y, std::min(snowballPos.y, characterMax.y));
	float closestZ = std::max(characterMin.z, std::min(snowballPos.z, characterMax.z));

	glm::vec3 closestPoint(closestX, closestY, closestZ);

	glm::vec3 distance = snowballPos - closestPoint;
	float distanceSquared = glm::dot(distance, distance);

	return distanceSquared <= (snowballRadius * snowballRadius);
}

void checkAllSnowballCollisions()
{
	for (const auto& snowball : snowballs) {
		if (!snowball.getIsActive()) continue;

		if (steve && checkSnowballCharacterCollision(snowball, steve->pos, steve->boundingBoxSize)) {
			std::cout << "\n=== 경기 종료 ===\nSteve가 눈덩이에 맞았습니다!" << std::endl;

			winner = Winner::ALEX;
			finishScreen.setWinner(winner);
			currentGameState = GameState::FINISH_SCREEN;
			return;
		}

		if (alex && checkSnowballCharacterCollision(snowball, alex->pos, alex->boundingBoxSize)) {
			std::cout << "\n=== 경기 종료 ===\nAlex가 눈덩이에 맞았습니다!" << std::endl;

			winner = Winner::STEVE;
			finishScreen.setWinner(winner);
			currentGameState = GameState::FINISH_SCREEN;
			return;
		}
	}
}

void SpecialKeyboard(int key, int x, int y)
{
	// [11.11] 로비에서는 위·아래 방향키로 방 선택만 바꾼다.
	if (gameClient.GetState() == CLIENT_LOBBY) {
		if (key == GLUT_KEY_UP) gameClient.MoveRoomSelection(-1);
		else if (key == GLUT_KEY_DOWN) gameClient.MoveRoomSelection(1);
		glutPostRedisplay();
		return;
	}

	if (currentGameState == GameState::TITLE_SCREEN) {
		return;
	}

	input.pressSpecial(key);

// 	float deltaTime = 0.016f;
// 	if (!splitScreenMode || currentGameState == GameState::THIRD_PERSON_MODE) {
// 		camera.processSpecialKeyboard(key);
// 		cameraPos = camera.position;
// 		cameraFront = camera.front;
// 		cameraUp = camera.up;
// 	}

	glutPostRedisplay();
}

void SpecialKeyboardUp(int key, int x, int y)
{
	if (currentGameState == GameState::TITLE_SCREEN) {
		return;
	}

	input.releaseSpecial(key);
	glutPostRedisplay();
}

void OnMouseButton(int button, int state, int x, int y)
{
    // [11.20] 경기 중 마우스 입력을 차징과 던지기 처리로 넘긴다.
    if (gameClient.GetState() == CLIENT_PLAYING) {
        gameClient.OnMouse(button, state);
        return;
    }
	// [11.7] 버튼 클릭 로그인 요청
	if (gameClient.GetState() == CLIENT_LOGIN) {
		if (button == GLUT_LEFT_BUTTON && state == GLUT_DOWN) {
			const int width = glutGet(GLUT_WINDOW_WIDTH);
			const int height = glutGet(GLUT_WINDOW_HEIGHT);
			if (titleScreen.IsMouseOverLoginInput(x, y, width, height, true)) {
				gameClient.FocusServerAddress(true);
			}
			else if (titleScreen.IsMouseOverLoginInput(x, y, width, height, false)) {
				gameClient.FocusServerAddress(false);
			}
			else if (titleScreen.hitsStart(x, y, width, height)) {
				gameClient.OnKeyDown('\r'); // 엔터키랑 같은 처리
			}
		}
		glutPostRedisplay();
		return;
	}
	if (gameClient.GetState() == CLIENT_LOBBY) return;
	// [11.12] 나가기 버튼 클릭도 Esc와 같은 퇴장 처리로 보낸다.
	if (gameClient.GetState() == CLIENT_ROOM) {
		if (button == GLUT_LEFT_BUTTON && state == GLUT_DOWN) {
			const int width = glutGet(GLUT_WINDOW_WIDTH);
			const int height = glutGet(GLUT_WINDOW_HEIGHT);
			titleScreen.SetMousePosition(x, y);
			if (titleScreen.IsMouseOverBackButton(x, y, width, height)) {
				gameClient.OnKeyDown(27);
			}
			else {
				// [11.14] 화살표 클릭은 서버 전송 없이 내 미리보기만 바꾼다.
				const RoomPlayerInfo* me = gameClient.GetRoomPlayer(gameClient.GetMySlot());
				if (me != nullptr && me->connected) {
					const bool confirmed = gameClient.IsCharacterConfirmed();
					if (!confirmed && titleScreen.IsMouseOverCharacterArrow(x, y, width, height, true)) {
						titleScreen.ChangeCharacterPreview(-1, me->character);
					}
					else if (!confirmed && titleScreen.IsMouseOverCharacterArrow(x, y, width, height, false)) {
						titleScreen.ChangeCharacterPreview(1, me->character);
					}
					else if (!confirmed && titleScreen.IsMouseOverCharacterConfirm(x, y, width, height)) {
						// [11.14] 현재 표시 중인 캐릭터를 기존 송신 버퍼에 등록한다.
						C2S_SelectCharacter request{};
						request.type = C2S_SELECT_CHARACTER;
						request.character = static_cast<CHARACTER_TYPE>(titleScreen.GetPreviewCharacter(me->character));
						gameClient.QueueRequest(&request, sizeof(request));
					}
					else if (titleScreen.IsMouseOverGameStartButton(x, y, width, height)) {
						C2S_GameStart request{};
						request.type = C2S_GAME_START;
						gameClient.QueueRequest(&request, sizeof(request));
					}
				}
			}
		}
		glutPostRedisplay();
		return;
	}
	if (currentGameState == GameState::TITLE_SCREEN) {
		return;
	}

// 	// 마우스 휠로 줌 인/아웃
// 	if (button == 3) { // 휠 위로
// 		if (splitScreenMode) {
// 			steveCamera.processMouseScroll(1);
// 			alexCamera.processMouseScroll(1);
// 		}
// 		else {
// 			camera.processMouseScroll(1);
// 			cameraPos = camera.position;
// 		}
// 		glutPostRedisplay();
// 	}
// 	else if (button == 4) { // 휠 아래로
// 		if (splitScreenMode) {
// 			steveCamera.processMouseScroll(-1);
// 			alexCamera.processMouseScroll(-1);
// 		}
// 		else {
// 			camera.processMouseScroll(-1);
// 			cameraPos = camera.position;
// 		}
// 		glutPostRedisplay();
// 	}

}

void OnMouseMove(int x, int y)
{
    // [11.12] 마우스 위치를 전달하면 대기실을 그릴 때 이미지를 고른다.
    titleScreen.SetMousePosition(x, y);
    glutPostRedisplay();
}

void OnMouseEnterLeave(int state)
{
    // [11.12] 창 밖에서는 마우스 올림 효과를 남기지 않는다.
    if (state == GLUT_LEFT) {
        titleScreen.SetMousePosition(-1, -1);
        glutPostRedisplay();
    }
}

void make_shaderProgram()
{

	// 먼저 파일에서 읽기 시도
	vertexSource = filetobuf("vertex.glsl");
	fragmentSource = filetobuf("fragment.glsl");

	// 버텍스 셰이더
	vertexShader = glCreateShader(GL_VERTEX_SHADER);
	glShaderSource(vertexShader, 1, (const GLchar**)&vertexSource, 0);
	glCompileShader(vertexShader);

	GLint result;
	GLchar errorLog[512];
	glGetShaderiv(vertexShader, GL_COMPILE_STATUS, &result);
	if (!result)
	{
		glGetShaderInfoLog(vertexShader, 512, NULL, errorLog);
		std::cerr << "ERROR: vertex shader 컴파일 실패\n" << errorLog << std::endl;
		return;
	}

	// 프래그먼트 셰이더
	fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
	glShaderSource(fragmentShader, 1, (const GLchar**)&fragmentSource, 0);
	glCompileShader(fragmentShader);

	glGetShaderiv(fragmentShader, GL_COMPILE_STATUS, &result);
	if (!result)
	{
		glGetShaderInfoLog(fragmentShader, 512, NULL, errorLog);
		std::cerr << "ERROR: fragment shader 컴파일 실패\n" << errorLog << std::endl;
		return;
	}

	// 셰이더 프로그램 생성
	shaderProgramID = glCreateProgram();
	glAttachShader(shaderProgramID, vertexShader);
	glAttachShader(shaderProgramID, fragmentShader);
	glLinkProgram(shaderProgramID);

	// 링크 상태 확인
	glGetProgramiv(shaderProgramID, GL_LINK_STATUS, &result);
	if (!result) {
		glGetProgramInfoLog(shaderProgramID, 512, NULL, errorLog);
		std::cerr << "ERROR: shader program 링크 실패\n" << errorLog << std::endl;
		return;
	}

	glDeleteShader(vertexShader);
	glDeleteShader(fragmentShader);
	vertexShader = fragmentShader = 0; // [종료 변경 3] OnExit에서 이미 삭제한 셰이더를 재삭제하지 않는다.

	glUseProgram(shaderProgramID);
	glUniform1i(glGetUniformLocation(shaderProgramID, "texture1"), 0);
	std::cout << "셰이더 프로그램이 성공적으로 생성되었습니다." << std::endl;
}

GLvoid Drawscene()
{
	if (shaderProgramID == 0) return;
	// [11.14] 로비·로그인 등에서는 이전 대기실 미리보기를 비운다.
	if (gameClient.GetState() != CLIENT_ROOM) titleScreen.ResetCharacterPreview();
	if (gameClient.GetState() != CLIENT_PLAYING && matchSceneInitialized) ClearMatchScene();
	glUseProgram(shaderProgramID);
	glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	switch (gameClient.GetState()) {
	case CLIENT_LOGIN:
		// [11.7] 닉네임 오류가 발생한 프레임에만 버튼 효과를 시작한다.
		if (gameClient.ConsumeNameError()) titleScreen.PlayNameError();
		titleScreen.render(shaderProgramID);
		// [11.7] 닉네임과 버튼은 배경 이미지 위에 매 프레임 따로 그린다.
		titleScreen.renderLoginUI(shaderProgramID, gameClient.GetName(), gameClient.GetErrorMessage(),
			false, gameClient.GetServerAddress(),
			gameClient.IsServerAddressFocused(), gameClient.IsServerAddressSelected());
		break;

	case CLIENT_LOBBY: // [11.10] 로그인 성공 후 로비 이미지를 그린다.
		// [11.11] 현재 선택 위치를 화면에도 전달한다.
		titleScreen.renderLobby(shaderProgramID, gameClient.GetRoom(0), gameClient.GetRoomCount(),
			gameClient.GetSelectedRoomIndex(), gameClient.GetErrorMessage());
		break;

	case CLIENT_ROOM: // [11.11] 서버가 입장을 허용한 뒤 안내를 표시한다.
		// [11.12] 저장된 두 자리 참가자 정보를 화면에 전달한다.
		titleScreen.renderRoomEntry(shaderProgramID, gameClient.GetMyRoomId(), gameClient.GetMySlot(),
			gameClient.GetRoomPlayer(0));
		break;

	case CLIENT_PLAYING: {
        if (!matchSceneInitialized) {
            gameMap.initialize();
            for (int slot = 0; slot < MAX_ROOM_PLAYERS; ++slot) {
                const ClientObject* player = gameClient.GetObject(slot);
                if (player->character == CHAR_STEVE) {
                    matchSteve[slot] = new Steve::Character("steve.png");
                    matchSteve[slot]->pos = player->renderPos;
                }
                else {
                    matchAlex[slot] = new Alex::Character("alex.png");
                    matchAlex[slot]->pos = player->renderPos;
                }
            }
            // [11.23] 그리기에 쓸 눈덩이 모양을 경기 시작 때 한 번 준비한다.
            matchSnowball = new Snowball(glm::vec3(0.0f),
                glm::vec3(0.0f, 0.0f, 1.0f), 0.0f, 0.15f);
            matchSceneInitialized = true;
        }
        const int winW = std::max(1, glutGet(GLUT_WINDOW_WIDTH));
        const int winH = std::max(1, glutGet(GLUT_WINDOW_HEIGHT));
        glViewport(0, 0, winW, winH);
        const int mySlot = gameClient.GetMySlot();
        const ClientObject* me = gameClient.GetObject(mySlot);
        glm::mat4 view(1.0f), projection(1.0f);
        if (mySlot == 0) {
            steveCamera.updateFromCharacterPosition(me->renderPos);
            cameraPos = steveCamera.getPosition();
            view = steveCamera.getViewMatrix();
            projection = steveCamera.getProjectionMatrix(static_cast<float>(winW), static_cast<float>(winH));
        }
        else {
            alexCamera.updateFromCharacterPosition(me->renderPos);
            cameraPos = alexCamera.getPosition();
            view = alexCamera.getViewMatrix();
            projection = alexCamera.getProjectionMatrix(static_cast<float>(winW), static_cast<float>(winH));
        }
        // [11.25] 맞은 직후 3D 화면을 살짝 뒤로 밀고 기울인다.
        const float hitStrength = gameClient.GetHitEffectStrength(mySlot);
        if (hitStrength > 0.0f) {
            glm::mat4 recoil = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 0.0f, -0.10f * hitStrength)); //recoil 뒷걸음질
            recoil = glm::rotate(recoil, glm::radians(-3.0f) * hitStrength, glm::vec3(1.0f, 0.0f, 0.0f));
            recoil = glm::rotate(recoil, glm::radians(4.0f) * hitStrength, glm::vec3(0.0f, 0.0f, 1.0f));
            view = recoil * view;
            cameraPos = glm::vec3(glm::inverse(view)[3]);
        }
        renderWorld(view, projection);
        // [11.25] 맵과 캐릭터를 그린 다음 HUD를 화면 위에 올린다.
        DrawGameUI(me->character, gameClient.GetHP(mySlot), winW, winH, me->powerUpActive);
		break;
	}

	case CLIENT_RESULT:
		break;
	}

	glutSwapBuffers();
}

// 	if (currentGameState == GameState::TITLE_SCREEN) {
// 		titleScreen.render(shaderProgramID);
// 		glutSwapBuffers();
// 		return;
// 	}
// 	if (currentGameState == GameState::FINISH_SCREEN) {
// 		finishScreen.render(shaderProgramID);
// 		glutSwapBuffers();
// 		return;
// 	}

// 	int winW = glutGet(GLUT_WINDOW_WIDTH);
// 	int winH = glutGet(GLUT_WINDOW_HEIGHT);
// 	if (winW <= 0) winW = WinX;
// 	if (winH <= 0) winH = WinY;
// 	if (splitScreenMode && currentGameState == GameState::FIRST_PERSON_MODE) {
// 		if (steve) steveCamera.updateFromCharacterPosition(steve->pos);
// 		if (alex) alexCamera.updateFromCharacterPosition(alex->pos);
// 		// 왼쪽: Steve
// 		glViewport(0, 0, winW / 2, winH);
// 		glm::mat4 steveView = steveCamera.getViewMatrix();
// 		glm::mat4 steveProj = steveCamera.getProjectionMatrix(winW / 2, winH);
// 		renderWorld(steveView, steveProj);
// 		glClear(GL_DEPTH_BUFFER_BIT);
// 		// 오른쪽: Alex
// 		glViewport(winW / 2, 0, winW - winW / 2, winH);
// 		glm::mat4 alexView = alexCamera.getViewMatrix();
// 		glm::mat4 alexProj = alexCamera.getProjectionMatrix(winW - winW / 2, winH);
// 		renderWorld(alexView, alexProj);
// 		drawCharacterFaces(winW, winH);
// 	}
// 	else {
// 		glViewport(0, 0, winW, winH);
// 		glm::mat4 view = camera.getViewMatrix();
// 		glm::mat4 proj = camera.getProjectionMatrix(winW, winH);
// 		renderWorld(view, proj);
// 	}

// GLvoid drawSplitScreen()
// {
// }

struct LandingMarker {
    glm::vec3 position;
    float tilt;
};

// [11.24] 지금 놓으면 눈덩이가 바닥 높이와 뒤쪽 벽 중 어디에 먼저 닿는지 구한다.
static LandingMarker PredictSnowballMarker(const ClientObject& player, int slot, float chargeSeconds)
{
    // 서버 Room::ThrowSnowball과 같은 시작 위치, 방향, 속도를 사용한다.
    constexpr float groundY = BLOCK_SIZE * 0.5f;
    constexpr float chargeLimitSeconds = 1.0f;
    constexpr float startUp = 0.8f;
    constexpr float startForward = 1.0f;
    constexpr float gravity = -9.8f;
    constexpr float step = 0.033f; // 서버의 기본 한 틱은 33ms다.
    constexpr float markerOffset = 0.015f; // 바닥이나 벽과 겹치지 않도록 조금 띄운다.
    const float forward = slot == 0 ? 1.0f : -1.0f;
    const float charge = std::min(chargeSeconds / chargeLimitSeconds, 1.0f);
    const float throwPower = player.character == CHAR_STEVE ? 1.3f : 1.0f;
    const float speed = glm::mix(2.0f, 15.0f, charge) * throwPower;
    glm::vec3 velocity = glm::normalize(glm::vec3(0.0f, 0.3f, forward)) * speed;
    glm::vec3 position(player.renderPos.x, groundY + startUp, player.renderPos.z + startForward * forward);

    const float wallZ = slot == 0 ? MAP_DEPTH * BLOCK_SIZE - groundY : -groundY;
    glm::vec3 previous;
    do {
        previous = position;
        velocity.y += gravity * step;
        position += velocity * step;

        // 벽을 넘은 한 틱 안에서 벽에 닿는 순간의 높이를 구한다.
        if ((previous.z - wallZ) * forward < 0.0f && (position.z - wallZ) * forward >= 0.0f) {
            const float t = (wallZ - previous.z) / (position.z - previous.z);
            const float wallY = glm::mix(previous.y, position.y, t);
            if (wallY >= groundY) {
                return { glm::vec3(position.x, wallY, wallZ - markerOffset * forward),
                    -forward * glm::half_pi<float>() };
            }
        }
    } while (position.y > groundY);

    // 중앙의 빈 공간에서도 양쪽 바닥과 같은 높이에 표시한다.
    const float t = (groundY - previous.y) / (position.y - previous.y);
    const float landingZ = glm::mix(previous.z, position.z, t);
    return { glm::vec3(position.x, groundY + markerOffset, landingZ), 0.0f };
}

// [11.24] 예상 위치에 빨간 원과 가운데 십자가를 함께 그린다.
static void DrawLandingMarker(const LandingMarker& marker)
{
    constexpr int segments = 32;
    constexpr int ringVertexCount = (segments + 1) * 2;
    constexpr int crossVertexCount = 14;
    if (landingMarkerVao == 0) {
        glm::vec3 vertices[ringVertexCount + crossVertexCount];
        for (int i = 0; i <= segments; ++i) {
            const float angle = glm::two_pi<float>() * i / segments;
            const glm::vec3 direction(std::cos(angle), 0.0f, std::sin(angle));
            vertices[2 * i] = direction * 0.38f;
            vertices[2 * i + 1] = direction * 0.29f;
        }
        // 십자가는 가운데 점에서 바깥 윤곽으로 삼각형을 이어 만든다.
        constexpr float arm = 0.18f;
        constexpr float halfWidth = 0.035f;
        const glm::vec3 crossVertices[crossVertexCount] = {
            { 0.0f, 0.0f, 0.0f },
            { -halfWidth, 0.0f, arm }, { halfWidth, 0.0f, arm },
            { halfWidth, 0.0f, halfWidth }, { arm, 0.0f, halfWidth },
            { arm, 0.0f, -halfWidth }, { halfWidth, 0.0f, -halfWidth },
            { halfWidth, 0.0f, -arm }, { -halfWidth, 0.0f, -arm },
            { -halfWidth, 0.0f, -halfWidth }, { -arm, 0.0f, -halfWidth },
            { -arm, 0.0f, halfWidth }, { -halfWidth, 0.0f, halfWidth },
            { -halfWidth, 0.0f, arm }
        };
        std::copy_n(crossVertices, crossVertexCount, vertices + ringVertexCount);

        glGenVertexArrays(1, &landingMarkerVao);
        glGenBuffers(1, &landingMarkerVbo);
        glBindVertexArray(landingMarkerVao);
        glBindBuffer(GL_ARRAY_BUFFER, landingMarkerVbo);
        glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(glm::vec3), nullptr);
        glEnableVertexAttribArray(0);
        glBindBuffer(GL_ARRAY_BUFFER, 0);
    }

    const GLboolean blendEnabled = glIsEnabled(GL_BLEND);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glDepthMask(GL_FALSE);
    const glm::mat4 model = glm::rotate(glm::translate(glm::mat4(1.0f), marker.position),
        marker.tilt, glm::vec3(1.0f, 0.0f, 0.0f));
    const GLint uiModeLoc = glGetUniformLocation(shaderProgramID, "uiMode");
    glUniformMatrix4fv(glGetUniformLocation(shaderProgramID, "model"), 1, GL_FALSE, &model[0][0]);
    glUniform1i(glGetUniformLocation(shaderProgramID, "useTexture"), 0);
    glUniform1i(uiModeLoc, 1);
    glUniform4f(glGetUniformLocation(shaderProgramID, "uiColor"), 1.0f, 0.06f, 0.06f, 0.85f);
    glBindVertexArray(landingMarkerVao);
    glDrawArrays(GL_TRIANGLE_STRIP, 0, ringVertexCount);
    glDrawArrays(GL_TRIANGLE_FAN, ringVertexCount, crossVertexCount);
    glBindVertexArray(0);
    glUniform1i(uiModeLoc, 0);
    glDepthMask(GL_TRUE);
    if (!blendEnabled) glDisable(GL_BLEND);
}

// [12.1] 기존 이미지 사각형을 바닥에 눕혀 강화한 캐릭터의 발밑에 그린다.
static void DrawPowerUpAura(const glm::vec3& position, ULONGLONG nowMs)
{
    if (powerUpAuraTexture == 0) return;

    constexpr float baseRadius = 1.1f * BLOCK_SIZE;
    constexpr float groundY = BLOCK_SIZE * 0.5f + 0.02f;
    constexpr ULONGLONG rotationPeriodMs = 8000;
    // [12.1] 발밑 중심은 고정하고 반지름만 3%씩 커졌다 작아지게 한다.
    const float pulseAngle = PowerUpAnimationAngle(nowMs, powerUpPulsePeriodMs);
    const float radius = baseRadius * (1.0f + 0.03f * std::sin(pulseAngle));
    const float rotationAngle = PowerUpAnimationAngle(nowMs, rotationPeriodMs);

    // [12.1] 바닥 끝을 넘어가도 원 전체를 그리며, 위를 향한 Y축을 중심으로 회전한다.
    glm::mat4 model = glm::translate(glm::mat4(1.0f), glm::vec3(position.x, groundY, position.z));
    model = glm::rotate(model, rotationAngle, glm::vec3(0.0f, 1.0f, 0.0f));
    model = glm::rotate(model, -glm::half_pi<float>(), glm::vec3(1.0f, 0.0f, 0.0f));
    model = glm::scale(model, glm::vec3(radius, radius, 1.0f));

    initializeFaceBuffers();
    const GLboolean blendEnabled = glIsEnabled(GL_BLEND);
    GLboolean depthWrite;
    glGetBooleanv(GL_DEPTH_WRITEMASK, &depthWrite);
    GLint srcRgb, dstRgb, srcAlpha, dstAlpha;
    glGetIntegerv(GL_BLEND_SRC_RGB, &srcRgb);
    glGetIntegerv(GL_BLEND_DST_RGB, &dstRgb);
    glGetIntegerv(GL_BLEND_SRC_ALPHA, &srcAlpha);
    glGetIntegerv(GL_BLEND_DST_ALPHA, &dstAlpha);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glDepthMask(GL_FALSE);

    const GLint uiModeLoc = glGetUniformLocation(shaderProgramID, "uiMode");
    glUniformMatrix4fv(glGetUniformLocation(shaderProgramID, "model"), 1, GL_FALSE, &model[0][0]);
    glUniform1i(uiModeLoc, 1);
    glUniform1i(glGetUniformLocation(shaderProgramID, "useTexture"), 1);
    glUniform4f(glGetUniformLocation(shaderProgramID, "uiColor"), 1.0f, 1.0f, 1.0f, 1.0f);
    glUniform4f(glGetUniformLocation(shaderProgramID, "uiUVRect"), 0.0f, 0.0f, 1.0f, 1.0f);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, powerUpAuraTexture);
    glBindVertexArray(faceVAO);
    glDrawArrays(GL_TRIANGLES, 0, 6);
    glBindVertexArray(0);
    glBindTexture(GL_TEXTURE_2D, 0);
    glUniform1i(uiModeLoc, 0);

    glDepthMask(depthWrite);
    glBlendFuncSeparate(srcRgb, dstRgb, srcAlpha, dstAlpha);
    if (!blendEnabled) glDisable(GL_BLEND);
}

// [12.1] 방패 PNG의 나무 판과 금속 테두리를 육면체의 각 면에 맞춘다.
static void InitializeMatchShield()
{
    // PNG를 256칸으로 보았을 때 오른쪽의 어두운 금속 부분을 옆면에 쓴다.
    constexpr float imageSize = 256.0f;
    constexpr UVRect metalEdge{ 178.0f, 90.0f, 4.0f, 70.0f };
    const UVRect faceUVs[] = { metalEdge, metalEdge, metalEdge, metalEdge, metalEdge, metalEdge };
    const bool flips[6]{};
    std::vector<glm::vec3> positions, normals;
    std::vector<glm::vec2> texCoords;
    Init::makeCube(glm::vec3(0.36f, 0.54f, 0.06f), faceUVs, flips, imageSize, imageSize,
        positions, normals, texCoords);

    // PNG 속 기울어진 나무 판의 네 모서리를 앞뒤의 평평한 면에 맞춘다.
    const auto imageUV = [imageSize](float x, float y) {
        return glm::vec2(x / imageSize, 1.0f - y / imageSize);
    };
    const glm::vec2 lowerLeft = imageUV(58.0f, 203.0f);
    const glm::vec2 lowerRight = imageUV(173.0f, 247.0f);
    const glm::vec2 upperRight = imageUV(173.0f, 57.0f);
    const glm::vec2 upperLeft = imageUV(58.0f, 18.0f);
    const glm::vec2 panelUVs[] = { lowerLeft, lowerRight, upperRight, upperRight, upperLeft, lowerLeft };
    std::copy_n(panelUVs, 6, texCoords.begin() + 24); // +Z 면
    std::copy_n(panelUVs, 6, texCoords.begin() + 30); // -Z 면

    Init::uploadMesh(positions, normals, texCoords,
        matchShield.vao, matchShield.vbo, matchShield.nbo, matchShield.tbo, matchShield.count);
}

// [12.1] 방패와 방어 고리가 같은 위치를 기준으로 시작한다.
static glm::vec3 GetMatchShieldCenter(const glm::vec3& position, float forward)
{
    return position + glm::vec3(0.0f, 0.07f, 0.80f * forward);
}

// [12.1] 방패가 켜진 캐릭터 앞에 텍스처를 입힌 얇은 육면체를 띄운다.
static void DrawMatchShield(const glm::vec3& position, int slot)
{
    if (matchShield.vao == 0) InitializeMatchShield();

    // 0번 자리는 +Z, 1번 자리는 -Z를 바라본다.
    const float forward = slot == 0 ? 1.0f : -1.0f;
    const glm::vec3 center = GetMatchShieldCenter(position, forward);
    const glm::mat4 model = glm::translate(glm::mat4(1.0f), center);

    const GLint cutoutLoc = glGetUniformLocation(shaderProgramID, "textureAlphaCutout");
    glUniformMatrix4fv(glGetUniformLocation(shaderProgramID, "model"), 1, GL_FALSE, &model[0][0]);
    glUniform1i(glGetUniformLocation(shaderProgramID, "useTexture"), 1);
    glUniform1i(cutoutLoc, 1);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, gameUITextures.itemShield);
    glBindVertexArray(matchShield.vao);
    glDrawArrays(GL_TRIANGLES, 0, matchShield.count);
    glBindVertexArray(0);
    glBindTexture(GL_TEXTURE_2D, 0);
    glUniform1i(cutoutLoc, 0);
}

// [12.1] 흰 고리를 양쪽 시점에서 볼 수 있도록 세로로 세운다.
static void InitializeShieldBlockRing()
{
    constexpr int segments = 40;
    constexpr float innerRadius = 0.93f;
    glm::vec3 vertices[2 * (segments + 1)];
    for (int i = 0; i <= segments; ++i) {
        const float angle = glm::two_pi<float>() * i / segments;
        const glm::vec3 direction(std::cos(angle), std::sin(angle), 0.0f);
        vertices[2 * i] = direction;
        vertices[2 * i + 1] = innerRadius * direction;
    }

    glGenVertexArrays(1, &shieldBlockRing.vao);
    glGenBuffers(1, &shieldBlockRing.vbo);
    glBindVertexArray(shieldBlockRing.vao);
    glBindBuffer(GL_ARRAY_BUFFER, shieldBlockRing.vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(glm::vec3), nullptr);
    glEnableVertexAttribArray(0);
    glBindVertexArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    shieldBlockRing.count = 2 * (segments + 1);
}

// [12.1] 방패 앞에서 시작한 고리 두 개가 차례로 캐릭터 뒤쪽으로 지나간다.
static void DrawShieldBlockEffect(const glm::vec3& position, int slot, ULONGLONG elapsedMs)
{
    constexpr ULONGLONG ringDurationMs = 300;
    constexpr ULONGLONG secondRingDelayMs = 120;
    if (elapsedMs >= ringDurationMs + secondRingDelayMs) return;
    if (shieldBlockRing.vao == 0) InitializeShieldBlockRing();

    const float forward = slot == 0 ? 1.0f : -1.0f;
    const glm::vec3 shieldCenter = GetMatchShieldCenter(position, forward);
    const glm::vec3 start = shieldCenter + glm::vec3(0.0f, 0.0f, 0.12f * forward);
    const glm::vec3 end(shieldCenter.x, shieldCenter.y, position.z - 0.60f * forward);

    const GLboolean blendEnabled = glIsEnabled(GL_BLEND);
    const GLboolean cullEnabled = glIsEnabled(GL_CULL_FACE);
    GLboolean depthWrite;
    glGetBooleanv(GL_DEPTH_WRITEMASK, &depthWrite);
    GLint srcRgb, dstRgb, srcAlpha, dstAlpha;
    glGetIntegerv(GL_BLEND_SRC_RGB, &srcRgb);
    glGetIntegerv(GL_BLEND_DST_RGB, &dstRgb);
    glGetIntegerv(GL_BLEND_SRC_ALPHA, &srcAlpha);
    glGetIntegerv(GL_BLEND_DST_ALPHA, &dstAlpha);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glDisable(GL_CULL_FACE);
    glDepthMask(GL_FALSE);

    const GLint modelLoc = glGetUniformLocation(shaderProgramID, "model");
    const GLint uiModeLoc = glGetUniformLocation(shaderProgramID, "uiMode");
    const GLint uiColorLoc = glGetUniformLocation(shaderProgramID, "uiColor");
    glUniform1i(uiModeLoc, 1);
    glUniform1i(glGetUniformLocation(shaderProgramID, "useTexture"), 0);
    glBindVertexArray(shieldBlockRing.vao);

    for (const ULONGLONG delayMs : { 0ULL, secondRingDelayMs }) {
        if (elapsedMs < delayMs || elapsedMs - delayMs >= ringDurationMs) continue;
        const float progress = static_cast<float>(elapsedMs - delayMs) / ringDurationMs;
        const float radius = glm::mix(0.84f, 1.02f, progress);
        const glm::vec3 center = glm::mix(start, end, progress);
        const glm::mat4 model = glm::scale(glm::translate(glm::mat4(1.0f), center),
            glm::vec3(radius, radius, 1.0f));
        glUniformMatrix4fv(modelLoc, 1, GL_FALSE, &model[0][0]);
        glUniform4f(uiColorLoc, 1.0f, 1.0f, 1.0f, 0.90f * (1.0f - progress));
        glDrawArrays(GL_TRIANGLE_STRIP, 0, shieldBlockRing.count);
    }

    glBindVertexArray(0);
    glUniform1i(uiModeLoc, 0);
    glDepthMask(depthWrite);
    if (cullEnabled) glEnable(GL_CULL_FACE);
    glBlendFuncSeparate(srcRgb, dstRgb, srcAlpha, dstAlpha);
    if (!blendEnabled) glDisable(GL_BLEND);
}

GLvoid renderWorld(const glm::mat4& view, const glm::mat4& projection)
{
	glEnable(GL_CULL_FACE);
	glCullFace(GL_BACK);
	glFrontFace(GL_CCW);
	glEnable(GL_DEPTH_TEST);

	unsigned int modelLoc = glGetUniformLocation(shaderProgramID, "model");
	unsigned int viewLoc = glGetUniformLocation(shaderProgramID, "view");
	unsigned int projLoc = glGetUniformLocation(shaderProgramID, "projection");

	GLint lightPosLoc = glGetUniformLocation(shaderProgramID, "lightPos");
	GLint lightColorLoc = glGetUniformLocation(shaderProgramID, "lightColor");
	GLint viewPosLoc = glGetUniformLocation(shaderProgramID, "viewPos");
	GLint lightingEnabledLoc = glGetUniformLocation(shaderProgramID, "lightingEnabled");
	GLint useTextureLoc = glGetUniformLocation(shaderProgramID, "useTexture");

	if (lightingEnabledLoc != -1) glUniform1i(lightingEnabledLoc, 1);

	if (lightPosLoc != -1) {
		glm::vec3 lightPos(5.0f, 8.0f, 7.5f);
		glUniform3fv(lightPosLoc, 1, &lightPos[0]);
	}
	if (lightColorLoc != -1) {
		glm::vec3 lightColor(1.5f, 1.5f, 1.5f);
		glUniform3fv(lightColorLoc, 1, &lightColor[0]);
	}

	if (viewLoc != -1) glUniformMatrix4fv(viewLoc, 1, GL_FALSE, &view[0][0]);
	if (projLoc != -1) glUniformMatrix4fv(projLoc, 1, GL_FALSE, &projection[0][0]);

	if (useTextureLoc != -1) glUniform1i(useTextureLoc, 0);
	gameMap.render();

    if (gameClient.GetState() == CLIENT_PLAYING) {
        if (viewPosLoc != -1) glUniform3fv(viewPosLoc, 1, &cameraPos[0]);
        if (useTextureLoc != -1) glUniform1i(useTextureLoc, 1);
        const int mySlot = gameClient.GetMySlot();
        const GLint hitTintLoc = glGetUniformLocation(shaderProgramID, "hitTint");
        for (int slot = 0; slot < MAX_ROOM_PLAYERS; ++slot) {
            const ClientObject* player = gameClient.GetObject(slot);
            const float facing = slot == 0 ? 0.0f : 180.0f;
            const bool drawPlayer = slot != mySlot;
            // [11.25] 맞은 캐릭터를 그릴 동안에만 붉은색을 섞는다.
            glUniform1f(hitTintLoc, drawPlayer ? 0.65f * gameClient.GetHitEffectStrength(slot) : 0.0f);
            if (matchSteve[slot]) {
                matchSteve[slot]->pos = player->renderPos;
                if (drawPlayer) matchSteve[slot]->draw(modelLoc, facing);
            }
            if (matchAlex[slot]) {
                matchAlex[slot]->pos = player->renderPos;
                if (drawPlayer) matchAlex[slot]->draw(modelLoc, facing - 180.0f);
            }
        }
        glUniform1f(hitTintLoc, 0.0f);

        // [12.1] 방패는 시점과 관계없이 사용하는 캐릭터의 위치를 따라간다.
        for (int slot = 0; slot < MAX_ROOM_PLAYERS; ++slot) {
            const ClientObject* player = gameClient.GetObject(slot);
            if (player && player->shieldActive) {
                DrawMatchShield(player->renderPos, slot);
            }
        }

        std::vector<SnowWallVisual> walls;
        // [11.23] 저장된 눈덩이를 서버가 알려 준 표시 위치에 그린다.
        for (const auto& entry : gameClient.GetObjects()) {
            const ClientObject& object = entry.second;
            if (object.type == OBJ_SNOWBALL) {
                matchSnowball->setPosition(object.renderPos);
                // [12.1] 서버가 강화했다고 알려 준 눈덩이에만 붉은 눈 무늬를 씌운다.
                const GLuint texture = object.powered ? poweredSnowballTexture : 0;
                matchSnowball->render(shaderProgramID, glm::vec3(1.0f), texture);
            } else if (object.type == OBJ_SNOW_WALL) {
                // [11.26] 이번 프레임에 표시할 눈 벽의 중심과 높이를 모은다.
                walls.push_back({ object.renderPos, object.height });
            }
        }
        matchSnowWalls.setWallVisuals(std::move(walls));
        matchSnowWalls.render(shaderProgramID);

        // [12.1] 강화가 켜진 자리마다 해당 진영 바닥에 아우라를 그린다.
        const ULONGLONG nowMs = GetTickCount64();
        for (int slot = 0; slot < MAX_ROOM_PLAYERS; ++slot) {
            const ClientObject* player = gameClient.GetObject(slot);
            if (!player) continue;
            if (player->powerUpActive) {
                DrawPowerUpAura(player->renderPos, nowMs);
            }
            const ULONGLONG blockMs = gameClient.GetLastShieldBlockMs(slot);
            if (blockMs != 0) {
                DrawShieldBlockEffect(player->renderPos, slot, nowMs - blockMs);
            }
        }

        // [11.24] 내 차징 중에만 예상 착지점을 바닥에 그린다.
        float chargeSeconds;
        if (gameClient.TryGetChargeSeconds(chargeSeconds)) {
            DrawLandingMarker(PredictSnowballMarker(*gameClient.GetObject(mySlot), mySlot, chargeSeconds));
        }
        return;
    }
	if (useTextureLoc != -1) glUniform1i(useTextureLoc, 1);
	snowSystem.render(shaderProgramID);

	if (useTextureLoc != -1) glUniform1i(useTextureLoc, 1);

	if (steve) {
		steve->draw(modelLoc);
	}
	if (alex) {
		alex->draw(modelLoc);
	}

	if (useTextureLoc != -1) glUniform1i(useTextureLoc, 0);
	for (const auto& snowball : snowballs) {
		snowball.render(shaderProgramID, glm::vec3(1.0f));
	}
}

GLvoid Reshape(int w, int h)
{
	glViewport(0, 0, w, h);
}

// void updateStevePosition(float deltaTime)
// {
// 	steve->moveDir = glm::vec2(0.0f, 0.0f);
// 	if (splitScreenMode) {
// 		if (input.isKeyDown('w') || input.isKeyDown('W')) steve->moveDir.y += 1.0f;
// 		if (input.isKeyDown('s') || input.isKeyDown('S')) steve->moveDir.y += -1.0f;
// 		if (input.isKeyDown('a') || input.isKeyDown('A')) steve->moveDir.x += 1.0f;
// 		if (input.isKeyDown('d') || input.isKeyDown('D')) steve->moveDir.x += -1.0f;
// 	}
// 	else {
// 		if (input.isKeyDown('w') || input.isKeyDown('W')) steve->moveDir.x += 1.0f;
// 		if (input.isKeyDown('s') || input.isKeyDown('S')) steve->moveDir.x += -1.0f;
// 		if (input.isKeyDown('a') || input.isKeyDown('A')) steve->moveDir.y += -1.0f;
// 		if (input.isKeyDown('d') || input.isKeyDown('D')) steve->moveDir.y += 1.0f;
// 	}
// }
// void updateAlexPosition(float deltaTime)
// {
// 	alex->moveDir = glm::vec2(0.0f, 0.0f);
// 	if (splitScreenMode) {
// 		if (input.isKeyDown('i') || input.isKeyDown('I')) alex->moveDir.y += -1.0f;
// 		if (input.isKeyDown('k') || input.isKeyDown('K')) alex->moveDir.y += 1.0f;
// 		if (input.isKeyDown('j') || input.isKeyDown('J')) alex->moveDir.x += -1.0f;
// 		if (input.isKeyDown('l') || input.isKeyDown('L')) alex->moveDir.x += 1.0f;
// 	}
// 	else {
// 		if (input.isKeyDown('i') || input.isKeyDown('I')) alex->moveDir.x += 1.0f;
// 		if (input.isKeyDown('k') || input.isKeyDown('K')) alex->moveDir.x += -1.0f;
// 		if (input.isKeyDown('j') || input.isKeyDown('J')) alex->moveDir.y += -1.0f;
// 		if (input.isKeyDown('l') || input.isKeyDown('L')) alex->moveDir.y += 1.0f;
// 	}
// }

void checkSteveMoving()
{
	bool moving = input.isKeyDown('w') || input.isKeyDown('W') ||
		input.isKeyDown('s') || input.isKeyDown('S') ||
		input.isKeyDown('a') || input.isKeyDown('A') ||
		input.isKeyDown('d') || input.isKeyDown('D');
	if (!moving && steve) steve->changeState(1, 0);
}


void checkAlexMoving()
{
	bool moving = input.isKeyDown('i') || input.isKeyDown('I') ||
		input.isKeyDown('j') || input.isKeyDown('J') ||
		input.isKeyDown('k') || input.isKeyDown('K') ||
		input.isKeyDown('l') || input.isKeyDown('L');
	if (!moving && alex) alex->changeState(1, 0);
}

template<class CharacterType>
void UpdateMatchCharacter(CharacterType* character, const ClientObject& player, float dt)
{
    if (!character) return;
    const bool isMoving = character->pos.x != player.renderPos.x ||
        character->pos.z != player.renderPos.z;
    character->pos = player.renderPos;
    if (character->armState != player.armState) {
        switch (player.armState) {
        case ARM_CHARGE:
            character->enterCharge();
            break;
        case ARM_THROW:
            if (character->armAngle >= 0.0f) character->armAngle = -glm::radians(90.0f);
            character->enterThrow();
            break;
        default:
            character->changeState(0, player.armState);
            break;
        }
    }
    if (player.armState == ARM_CHARGE) {
        character->armAngle = std::max(-glm::radians(180.0f),
            character->armAngle - glm::radians(180.0f) * dt / maxChargeTime);
    }
    character->changeState(1, isMoving ? 1 : 0);
    character->updateAnimation(dt, isMoving, true);
}

void TimerFunction(int value)
{
	// [11.6] 타이틀 화면에서도 통신을 처리하도록 화면 검사 전에 호출한다.
	gameClient.Tick();

    const int now = glutGet(GLUT_ELAPSED_TIME);
    static int lastAnimationMs = now;
    const float animationDt = std::min(static_cast<float>(now - lastAnimationMs) / 1000.0f, 0.05f);
    lastAnimationMs = now;
    if (gameClient.GetState() == CLIENT_PLAYING && matchSceneInitialized) {
        for (int slot = 0; slot < MAX_ROOM_PLAYERS; ++slot) {
            const ClientObject& player = *gameClient.GetObject(slot);
            UpdateMatchCharacter(matchSteve[slot], player, animationDt);
            UpdateMatchCharacter(matchAlex[slot], player, animationDt);
        }
    }

	if (gameClient.GetState() == CLIENT_PLAYING || currentGameState == GameState::TITLE_SCREEN) {
		glutPostRedisplay();
		glutTimerFunc(16, TimerFunction, 1);
		return;
	}

	float deltaTime = 0.016f;

	snowSystem.updateAnimations(deltaTime);

	for (auto& sb : snowballs) sb.update(deltaTime, snowSystem, gameMap);
	snowballs.erase(std::remove_if(snowballs.begin(), snowballs.end(),
		[](const Snowball& s) { return !s.getIsActive(); }), snowballs.end());

	checkAllSnowballCollisions();

	if (steve && steveCharging && steve->armState == 2) {
		float currentTime = glutGet(GLUT_ELAPSED_TIME) / 1000.0f;
		float chargeTime = currentTime - steveChargeStartTime;
		float chargeRatio = std::min(chargeTime / maxChargeTime, 1.0f);
		steve->armAngle = -glm::radians(180.0f) * chargeRatio;
	}
	if (alex && alexCharging && alex->armState == 2) {
		float currentTime = glutGet(GLUT_ELAPSED_TIME) / 1000.0f;
		float chargeTime = currentTime - alexChargeStartTime;
		float chargeRatio = std::min(chargeTime / maxChargeTime, 1.0f);
		alex->armAngle = -glm::radians(180.0f) * chargeRatio;
	}

	// 캐릭터 업데이트
	// updateStevePosition(deltaTime);
	// updateAlexPosition(deltaTime);

	if (steve) steve->update(gameMap, snowSystem);
	if (alex) alex->update(gameMap, snowSystem);

	checkSteveMoving();
	checkAlexMoving();

	// cameraPos = camera.position;
	// cameraFront = camera.front;
	// cameraUp = camera.up;

	glutPostRedisplay();
	glutTimerFunc(16, TimerFunction, 1);
}
