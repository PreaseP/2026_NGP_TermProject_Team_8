#pragma once

// =====================================================================
//  Minecraft Snowball Fight - 네트워크 프로토콜
//  - TCP, 리틀 엔디언(Windows x86/x64), #pragma pack(1)
//  - 모든 패킷은 [size(1B)][type(1B)][본문] 구조, size는 헤더 포함 전체 길이
//  - 화면은 정면 고정: 캐릭터는 항상 상대 진영을 보고, 눈덩이는 정면으로 던짐
// =====================================================================

constexpr unsigned short SERVER_PORT = 9000;

constexpr int MAX_ROOMS = 3;     // 서버 시작 시 만들어 두는 방 수
constexpr int MAX_ROOM_PLAYERS = 2;     // 방 하나의 정원 (1:1)
constexpr int MAX_NAME_LEN = 16;    // 닉네임 (널 문자 포함, 영문·숫자)
constexpr int MAX_HP = 100;
constexpr int MAX_ITEMS = 2;     // 한 번에 보유할 수 있는 아이템 수
constexpr int ITEM_INTERVAL_MS = 20000; // 랜덤 아이템 지급 주기
constexpr int BUF_SIZE = 256;   // 패킷 최대 크기 (size가 1바이트)

// 오브젝트 ID 규칙
//  0 ~ 1       : 캐릭터 (방 안의 자리 번호와 같음, S2C_GameStartResult로 생성)
//  100 ~ 999   : 눈덩이 (서버가 순서대로 발급)
//  1000 ~      : 눈 벽  (1000 + gx * 100 + gz)
constexpr int OBJ_ID_SNOWBALL_START = 100;
constexpr int OBJ_ID_SNOW_WALL_START = 1000;

enum PACKET_TYPE : unsigned char {
	C2S_LOGIN,				// 닉네임을 보내 로그인 요청
	C2S_JOIN_ROOM,			// 방 번호를 보내 입장 요청
	C2S_LEAVE_ROOM,			// 방에서 나가 로비로 복귀 요청
	C2S_SELECT_CHARACTER,	// 대기실에서 캐릭터 선택
	C2S_GAME_START,			// 대기실에서 GameStart를 누르면 (조건이 맞으면) 바로 게임 시작
	C2S_MOVE,				// 눌린 이동 키
	C2S_CHARGE_START,		// 던지기 키 누름 (차징 시작) - 차징 시간은 서버가 계산
	C2S_THROW,				// 던지기 키 뗌
	C2S_USE_ITEM,			// 아이템 사용

	S2C_LOGIN_RESULT,		// 로그인 결과
	S2C_ROOM_LIST,			// 로비에 보여줄 방 목록
	S2C_JOIN_ROOM_RESULT,	// 방 입장 결과
	S2C_ROOM_INFO,			// 대기실 정보 (닉네임, 캐릭터)
	S2C_GAME_START_RESULT,	// 게임 시작 결과 (성공 시 캐릭터 정보)
	S2C_ADD_OBJECT,			// 눈덩이, 눈 벽 생성
	S2C_MOVE_OBJECT,		// 캐릭터, 눈덩이 위치 갱신
	S2C_REMOVE_OBJECT,		// 눈덩이, 눈 벽 제거
	S2C_STATUS_CHANGE,		// 오브젝트 상태 변화
	S2C_HIT,				// 눈덩이 적중
	S2C_ITEM_GET,			// 아이템 지급
	S2C_ITEM_USED,			// 아이템 사용 알림
	S2C_GAME_OVER			// 게임 종료
};

enum CHARACTER_TYPE : unsigned char {
	CHAR_STEVE = 0,			// 던지기 강함, 이동 느림
	CHAR_ALEX = 1,			// 이동 빠름, 던지기 약함
	CHAR_NONE = 0xFF		// 아직 선택 안 함
};

enum OBJECT_TYPE : unsigned char {
	OBJ_CHARACTER,			// S2C_AddObject로는 보내지 않음 (ID 0~1, GameStartResult로 생성)
	OBJ_SNOWBALL,
	OBJ_SNOW_WALL
};

enum ITEM_TYPE : unsigned char {
	ITEM_SHIELD,			// 방어막: 다음 1회 피격 방어
	ITEM_POWER_UP			// 다음 눈덩이 강화: 데미지 2배
};

enum ROOM_STATE : unsigned char {
	ROOM_WAITING,			// 대기 중
	ROOM_PLAYING,			// 게임 중
	ROOM_RESULT				// 결과 표시 중
};

enum STATUS_TYPE : unsigned char {
	STATUS_ARM_STATE,		// value: ARM_STATE
	STATUS_WALL_HEIGHT,		// value: 눈 벽 높이 (0.5 단위)
	STATUS_SHIELD,			// value: 1 켜짐 / 0 꺼짐
	STATUS_POWER_UP			// value: 1 켜짐 / 0 꺼짐
};

enum ARM_STATE : unsigned char {
	ARM_IDLE,
	ARM_RUN,
	ARM_CHARGE,
	ARM_THROW
};

enum LOGIN_RESULT : unsigned char {
	LOGIN_OK,
	LOGIN_INVALID_NAME,		// 빈 닉네임, 허용되지 않는 문자
	LOGIN_SERVER_FULL
};

enum JOIN_RESULT : unsigned char {
	JOIN_OK,
	JOIN_ROOM_FULL,
	JOIN_ROOM_PLAYING,		// 게임 중이라 입장 불가
	JOIN_INVALID_ROOM
};

enum END_REASON : unsigned char {
	END_HP_ZERO,			// 상대 체력 0
	END_OPPONENT_LEFT		// 상대 접속 끊김, 이탈
};

// C2S_Move의 move_keys 비트 (여러 개를 OR로 조합, 0이면 정지)
enum MOVE_KEY : unsigned char {
	KEY_FORWARD = 1 << 0,
	KEY_BACK = 1 << 1,
	KEY_LEFT = 1 << 2,
	KEY_RIGHT = 1 << 3
};

#pragma pack(push, 1)

// ───────────────────────── 공통 ─────────────────────────

struct PacketHeader {						// 받은 버퍼의 앞 2바이트를 읽을 때 사용
	unsigned char			size;
	PACKET_TYPE				type;
};

struct RoomSummary {
	unsigned char			room_id;
	unsigned char			player_count;
	ROOM_STATE				state;
};

struct RoomPlayerInfo {
	bool					connected;
	char					name[MAX_NAME_LEN];
	CHARACTER_TYPE			character;
};

// ───────────────────────── C2S ─────────────────────────

struct C2S_Login {
	unsigned char			size;
	PACKET_TYPE				type;
	char					name[MAX_NAME_LEN];
};

struct C2S_JoinRoom {
	unsigned char			size;
	PACKET_TYPE				type;
	unsigned char			room_id;		// 요청일 뿐, 서버가 검사 후 Join_Room_Result로 응답
};

struct C2S_LeaveRoom {
	unsigned char			size;
	PACKET_TYPE				type;
};

struct C2S_SelectCharacter {
	unsigned char			size;
	PACKET_TYPE				type;
	CHARACTER_TYPE			character;
};

struct C2S_GameStart {
	unsigned char			size;
	PACKET_TYPE				type;
};

struct C2S_Move {
	unsigned char			size;
	PACKET_TYPE				type;
	unsigned char			move_keys;		// MOVE_KEY 비트 조합
};

struct C2S_ChargeStart {
	unsigned char			size;
	PACKET_TYPE				type;
};

struct C2S_Throw {
	unsigned char			size;
	PACKET_TYPE				type;
};

struct C2S_UseItem {
	unsigned char			size;
	PACKET_TYPE				type;
	ITEM_TYPE				item;
};

// ───────────────────────── S2C ─────────────────────────

struct S2C_LoginResult {
	unsigned char			size;
	PACKET_TYPE				type;
	LOGIN_RESULT			result;
	int						session_id;
};

struct S2C_RoomList {
	unsigned char			size;
	PACKET_TYPE				type;
	unsigned char			room_count;
	RoomSummary				rooms[MAX_ROOMS];
};

struct S2C_JoinRoomResult {
	unsigned char			size;
	PACKET_TYPE				type;
	JOIN_RESULT				result;
	unsigned char			room_id;
	unsigned char			my_slot;		// 0 또는 1 = 내 캐릭터의 오브젝트 ID
};

struct S2C_RoomInfo {
	unsigned char			size;
	PACKET_TYPE				type;
	unsigned char			room_id;
	RoomPlayerInfo			players[MAX_ROOM_PLAYERS];	// 자리 번호 순서
};

struct S2C_GameStartResult {
	unsigned char			size;
	PACKET_TYPE				type;
	bool					success;		// false면 누른 사람에게만 전송, 아래 필드는 의미 없음
	CHARACTER_TYPE			character[MAX_ROOM_PLAYERS];	// 인덱스 = 자리 번호 = 캐릭터 오브젝트 ID
	float					x[MAX_ROOM_PLAYERS];			// 시작 위치
	float					y[MAX_ROOM_PLAYERS];
	float					z[MAX_ROOM_PLAYERS];
};

struct S2C_AddObject {
	unsigned char			size;
	PACKET_TYPE				type;
	int						object_id;
	OBJECT_TYPE				obj_type;		// OBJ_SNOWBALL 또는 OBJ_SNOW_WALL
	unsigned char			owner_slot;		// 눈덩이: 던진 사람, 눈 벽: 0xFF
	float					x, y, z;
	float					height;			// 눈 벽일 때만
	bool					powered;		// 눈덩이일 때만 (강화 여부)
};

struct S2C_MoveObject {
	unsigned char			size;
	PACKET_TYPE				type;
	int						object_id;		// 캐릭터(0~1) 또는 눈덩이
	float					x, y, z;
};

struct S2C_RemoveObject {
	unsigned char			size;
	PACKET_TYPE				type;
	int						object_id;
};

struct S2C_StatusChange {
	unsigned char			size;
	PACKET_TYPE				type;
	int						object_id;		// 캐릭터(0~1) 또는 눈 벽
	STATUS_TYPE				status;
	float					value;			// STATUS_TYPE 주석 참고
};

struct S2C_Hit {
	unsigned char			size;
	PACKET_TYPE				type;
	unsigned char			target_slot;	// 맞은 사람
	bool					blocked;		// 방어막으로 막았는지
	int						hp;				// 맞은 뒤 남은 체력
};

struct S2C_ItemGet {
	unsigned char			size;
	PACKET_TYPE				type;
	ITEM_TYPE				item;			// 본인에게만 전송
};

struct S2C_ItemUsed {
	unsigned char			size;
	PACKET_TYPE				type;
	unsigned char			user_slot;
	ITEM_TYPE				item;
};

struct S2C_GameOver {
	unsigned char			size;
	PACKET_TYPE				type;
	unsigned char			winner_slot;
	END_REASON				reason;
};

#pragma pack(pop)
