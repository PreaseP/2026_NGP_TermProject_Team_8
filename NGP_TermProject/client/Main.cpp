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

static Camera camera;
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

static bool steveThrowFlag = false;
static bool alexThrowFlag = false;

static glm::vec3 stevePosition = glm::vec3(3.0f, 0.8f, 5.0f);
static glm::vec3 alexPosition = glm::vec3(7.0f, 0.8f, 5.0f);

static bool splitScreenMode = false;

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
GLvoid drawScene();
GLvoid drawSplitScreen();
GLvoid renderWorld(const glm::mat4& view, const glm::mat4& projection);
GLvoid Reshape(int w, int h);

void initializeGame();

void fireSteveSnowball();
void fireAlexSnowball();

bool checkSnowballCharacterCollision(const Snowball& snowball, const glm::vec3& characterPos, const glm::vec3& boundingBoxSize);
void checkAllSnowballCollisions();

void initializeFaceBuffers();
void loadFaceTextures();
GLuint loadTexture(const char* path);
void renderCharacterFace(GLuint textureID, float x, float y, float size, int screenWidth, int screenHeight);
void drawCharacterFaces(int winW, int winH);

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
	auto releaseCharacter = [](auto*& character) {
		if (!character) return;
		for (Part* part : { &character->gBody, &character->gHead,
			&character->gArmL, &character->gArmR, &character->gLegL,
			&character->gLegR, &character->gBoundingBox }) {
			glDeleteVertexArrays(1, &part->vao);
			glDeleteBuffers(1, &part->vbo);
			glDeleteBuffers(1, &part->nbo);
			glDeleteBuffers(1, &part->tbo);
		}
		glDeleteTextures(1, &character->textureID);
		delete character;
		character = nullptr;
	};
	releaseCharacter(steve);
	releaseCharacter(alex);

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
	if (!gameClient.Init("127.0.0.1")) {
		std::cerr << "클라이언트 네트워크 초기화 실패" << std::endl;
	}
	else if (!gameClient.TryConnect()) {
		std::cerr << "서버 접속 실패" << std::endl;
	}
	else {
		std::cout << "서버 접속 완료" << std::endl;
	}

	std::cout << "\n=== MINECRAFT SNOWBALL FIGHT ===" << std::endl;
	std::cout << "닉네임 입력 후 GAME START 클릭 또는 Enter: 로그인" << std::endl; // [11.7]
	std::cout << "종료: 창의 X 버튼" << std::endl;

	//--- 콜백 등록
	glutDisplayFunc(drawScene);
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

GLuint loadTexture(const char* path)
{
	GLuint textureID;
	glGenTextures(1, &textureID);
	glBindTexture(GL_TEXTURE_2D, textureID);

	// 텍스처 파라미터 설정
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
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

		glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);
		glGenerateMipmap(GL_TEXTURE_2D);

		std::cout << "텍스처 로드 성공: " << path << " (" << width << "x" << height << ", " << channels << " channels)" << std::endl;
	}
	else {
		std::cerr << "텍스처 로드 실패: " << path << std::endl;
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

void drawCharacterFaces(int winW, int winH)
{
	if (!splitScreenMode || currentGameState != GameState::FIRST_PERSON_MODE) return;
	if (!faceTexturesLoaded) return;

	const float faceSize = 40.0f;
	const float margin = 40.0f;

	float steveX = margin;
	float steveY = winH - faceSize - margin;

	float alexX = margin;
	float alexY = winH - faceSize - margin;

	glViewport(0, 0, winW / 2, winH);
	renderCharacterFace(steveFaceTextureID, steveX, steveY, faceSize, winW / 2, winH);

	glViewport(winW / 2, 0, winW - winW / 2, winH);
	renderCharacterFace(alexFaceTextureID, alexX, alexY, faceSize, winW - winW / 2, winH);

	glViewport(0, 0, winW, winH);
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

	if (currentGameState == GameState::FIRST_PERSON_MODE) {
		splitScreenMode = true;
		std::cout << "\n=== 1인칭 모드 조작법 ===" << std::endl;
		std::cout << "V: 분할 화면 모드 토글" << std::endl;
		std::cout << "WASD: Steve 1인칭 시점 회전" << std::endl;
		std::cout << "화살표 키: Alex 1인칭 시점 회전" << std::endl;
		std::cout << "IJKL: 선택된 캐릭터 이동 (앞/왼/뒤/오른)" << std::endl;
		std::cout << "TAB: 캐릭터 선택 (Steve/Alex)" << std::endl;
	}
	else {
		splitScreenMode = false;
		std::cout << "\n=== 3인칭 모드 조작법 ===" << std::endl;
		std::cout << "V: 분할 화면 모드 토글" << std::endl;
		std::cout << "화살표 키: 카메라 회전" << std::endl;
		std::cout << "IJKL: 선택된 캐릭터 이동 (앞/왼/뒤/오른)" << std::endl;
		std::cout << "TAB: 캐릭터 선택 (Steve/Alex)" << std::endl;
	}
	std::cout << "E: Steve 눈덩이 발사" << std::endl;
	std::cout << "O: Alex 눈덩이 발사" << std::endl;
	std::cout << "X: 모든 눈 제거" << std::endl;
	std::cout << "종료: 창의 X 버튼" << std::endl;

	finishScreen.initialize();
}

void Keyboard(unsigned char key, int x, int y)
{
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
	case 'v': case 'V':
		splitScreenMode = !splitScreenMode;
		std::cout << "분할 화면 모드: " << (splitScreenMode ? "ON" : "OFF") << std::endl;
		break;
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

	float deltaTime = 0.016f;
	if (!splitScreenMode || currentGameState == GameState::THIRD_PERSON_MODE) {
		camera.processSpecialKeyboard(key);
		cameraPos = camera.position;
		cameraFront = camera.front;
		cameraUp = camera.up;
	}
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
	// [11.7] 버튼 클릭 로그인 요청
	if (gameClient.GetState() == CLIENT_LOGIN) {
		if (button == GLUT_LEFT_BUTTON && state == GLUT_DOWN &&
			titleScreen.hitsStart(x, y, glutGet(GLUT_WINDOW_WIDTH), glutGet(GLUT_WINDOW_HEIGHT))) {
			gameClient.OnKeyDown('\r'); // 엔터키랑 같은 처리
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
					if (titleScreen.IsMouseOverCharacterArrow(x, y, width, height, true)) {
						titleScreen.ChangeCharacterPreview(-1, me->character);
					}
					else if (titleScreen.IsMouseOverCharacterArrow(x, y, width, height, false)) {
						titleScreen.ChangeCharacterPreview(1, me->character);
					}
					else if (titleScreen.IsMouseOverCharacterConfirm(x, y, width, height)) {
						// [11.14] 현재 표시 중인 캐릭터를 기존 송신 버퍼에 등록한다.
						C2S_SelectCharacter request{};
						request.type = C2S_SELECT_CHARACTER;
						request.character = static_cast<CHARACTER_TYPE>(titleScreen.GetPreviewCharacter(me->character));
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

	// 마우스 휠로 줌 인/아웃
	if (button == 3) { // 휠 위로
		if (splitScreenMode) {
			steveCamera.processMouseScroll(1);
			alexCamera.processMouseScroll(1);
		}
		else {
			camera.processMouseScroll(1);
			cameraPos = camera.position;
		}
		glutPostRedisplay();
	}
	else if (button == 4) { // 휠 아래로
		if (splitScreenMode) {
			steveCamera.processMouseScroll(-1);
			alexCamera.processMouseScroll(-1);
		}
		else {
			camera.processMouseScroll(-1);
			cameraPos = camera.position;
		}
		glutPostRedisplay();
	}
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

GLvoid drawScene()
{
	if (shaderProgramID == 0) return;
	// [11.14] 로비·로그인 등에서는 이전 대기실 미리보기를 비운다.
	if (gameClient.GetState() != CLIENT_ROOM) titleScreen.ResetCharacterPreview();
	glUseProgram(shaderProgramID);
	glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	// [11.7] 닉네임과 버튼은 배경 이미지 위에 매 프레임 따로 그린다.
	if (gameClient.GetState() == CLIENT_LOBBY) { // [11.10] 로그인 성공 후 로비 이미지를 그린다.
		// [11.11] 현재 선택 위치를 화면에도 전달한다.
		titleScreen.renderLobby(shaderProgramID, gameClient.GetRoom(0), gameClient.GetRoomCount(),
			gameClient.GetSelectedRoomIndex(), gameClient.GetErrorMessage());
		glutSwapBuffers();
		return;
	}
	if (gameClient.GetState() == CLIENT_ROOM) { // [11.11] 서버가 입장을 허용한 뒤 안내를 표시한다.
		// [11.12] 저장된 두 자리 참가자 정보를 화면에 전달한다.
		titleScreen.renderRoomEntry(shaderProgramID, gameClient.GetMyRoomId(), gameClient.GetMySlot(),
			gameClient.GetRoomPlayer(0));
		glutSwapBuffers();
		return;
	}
	if (gameClient.GetState() == CLIENT_LOGIN) {
		// [11.7] 닉네임 오류가 발생한 프레임에만 버튼 효과를 시작한다.
		if (gameClient.ConsumeNameError()) titleScreen.PlayNameError();
		titleScreen.render(shaderProgramID);
		titleScreen.renderLoginUI(shaderProgramID, gameClient.GetName(), gameClient.GetErrorMessage(),
			gameClient.GetState() == CLIENT_LOBBY);
		glutSwapBuffers();
		return;
	}

	if (currentGameState == GameState::TITLE_SCREEN) {
		titleScreen.render(shaderProgramID);
		glutSwapBuffers();
		return;
	}

	if (currentGameState == GameState::FINISH_SCREEN) {
		finishScreen.render(shaderProgramID);
		glutSwapBuffers();
		return;
	}

	int winW = glutGet(GLUT_WINDOW_WIDTH);
	int winH = glutGet(GLUT_WINDOW_HEIGHT);
	if (winW <= 0) winW = WinX;
	if (winH <= 0) winH = WinY;

	if (splitScreenMode && currentGameState == GameState::FIRST_PERSON_MODE) {
		if (steve) steveCamera.updateFromCharacterPosition(steve->pos);
		if (alex) alexCamera.updateFromCharacterPosition(alex->pos);

		// 왼쪽: Steve
		glViewport(0, 0, winW / 2, winH);
		glm::mat4 steveView = steveCamera.getViewMatrix();
		glm::mat4 steveProj = steveCamera.getProjectionMatrix(winW / 2, winH);
		renderWorld(steveView, steveProj);

		glClear(GL_DEPTH_BUFFER_BIT);

		// 오른쪽: Alex
		glViewport(winW / 2, 0, winW - winW / 2, winH);
		glm::mat4 alexView = alexCamera.getViewMatrix();
		glm::mat4 alexProj = alexCamera.getProjectionMatrix(winW - winW / 2, winH);
		renderWorld(alexView, alexProj);

		drawCharacterFaces(winW, winH);
	}
	else {
		glViewport(0, 0, winW, winH);
		glm::mat4 view = camera.getViewMatrix();
		glm::mat4 proj = camera.getProjectionMatrix(winW, winH);
		renderWorld(view, proj);
	}

	glutSwapBuffers();
}

GLvoid drawSplitScreen() 
{
	
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

void updateStevePosition(float deltaTime)
{
	steve->moveDir = glm::vec2(0.0f, 0.0f);

	if (splitScreenMode) {
		if (input.isKeyDown('w') || input.isKeyDown('W')) steve->moveDir.y += 1.0f;
		if (input.isKeyDown('s') || input.isKeyDown('S')) steve->moveDir.y += -1.0f;
		if (input.isKeyDown('a') || input.isKeyDown('A')) steve->moveDir.x += 1.0f;
		if (input.isKeyDown('d') || input.isKeyDown('D')) steve->moveDir.x += -1.0f;
	}
	else {
		if (input.isKeyDown('w') || input.isKeyDown('W')) steve->moveDir.x += 1.0f;
		if (input.isKeyDown('s') || input.isKeyDown('S')) steve->moveDir.x += -1.0f;
		if (input.isKeyDown('a') || input.isKeyDown('A')) steve->moveDir.y += -1.0f;
		if (input.isKeyDown('d') || input.isKeyDown('D')) steve->moveDir.y += 1.0f;
	}
}

void updateAlexPosition(float deltaTime)
{
	alex->moveDir = glm::vec2(0.0f, 0.0f);
	if (splitScreenMode) {
		if (input.isKeyDown('i') || input.isKeyDown('I')) alex->moveDir.y += -1.0f;
		if (input.isKeyDown('k') || input.isKeyDown('K')) alex->moveDir.y += 1.0f;
		if (input.isKeyDown('j') || input.isKeyDown('J')) alex->moveDir.x += -1.0f;
		if (input.isKeyDown('l') || input.isKeyDown('L')) alex->moveDir.x += 1.0f;
	}
	else {
		if (input.isKeyDown('i') || input.isKeyDown('I')) alex->moveDir.x += 1.0f;
		if (input.isKeyDown('k') || input.isKeyDown('K')) alex->moveDir.x += -1.0f;
		if (input.isKeyDown('j') || input.isKeyDown('J')) alex->moveDir.y += -1.0f;
		if (input.isKeyDown('l') || input.isKeyDown('L')) alex->moveDir.y += 1.0f;
	}
}

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

void TimerFunction(int value)
{
	// [11.6] 타이틀 화면에서도 통신을 처리하도록 화면 검사 전에 호출한다.
	gameClient.Tick();

	if (currentGameState == GameState::TITLE_SCREEN) {
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
	updateStevePosition(deltaTime);
	updateAlexPosition(deltaTime);

	if (steve) steve->update(gameMap, snowSystem);
	if (alex) alex->update(gameMap, snowSystem);

	checkSteveMoving();
	checkAlexMoving();

	cameraPos = camera.position;
	cameraFront = camera.front;
	cameraUp = camera.up;

	glutPostRedisplay();
	glutTimerFunc(16, TimerFunction, 1);
}
