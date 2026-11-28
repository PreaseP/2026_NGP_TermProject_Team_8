#pragma once

#include "NetworkClient.h"
#include "InputHandler.h" // [11.6]
#include <string>
#include <unordered_map>
#include <gl/glm/glm.hpp>

enum CLIENT_STATE { CLIENT_LOGIN, CLIENT_LOBBY, CLIENT_ROOM, CLIENT_PLAYING, CLIENT_RESULT };

struct ClientObject {
    int objectId = -1;
    OBJECT_TYPE type = OBJ_CHARACTER;
    CHARACTER_TYPE character = CHAR_NONE;
    unsigned char ownerSlot = 0xFF; // [11.23] 눈덩이를 던진 자리. 아직 없으면 0xFF다.
    bool powered = false; // [11.23] 강화된 눈덩이인지 저장한다.
    bool powerUpActive = false; // [12.1] 플레이어가 다음 눈덩이에 쓸 공격 강화를 가지고 있는지 저장한다.
    bool shieldActive = false; // [12.1] 플레이어의 방패가 켜져 있는지 저장한다.
    float height = 0.0f; // [11.26] 눈 벽의 현재 높이
    float targetHeight = 0.0f; // [11.26] 눈 벽의 목표 높이
    float startHeight = 0.0f; // [11.26] 높이가 바뀌기 시작할 때 보이던 높이
    float wallBaseY = 0.0f; // [11.26] 고정할 눈 벽의 밑면 위치
    glm::vec3 renderPos{0.0f};
    glm::vec3 prevPos{0.0f};
    glm::vec3 targetPos{0.0f};
    float lerpT = 1.0f;
    ARM_STATE armState = ARM_IDLE;
};

// [11.27] 내 아이템 한 개를 보관한다.
struct ClientInventory {
    ITEM_TYPE item = ITEM_SHIELD;
    bool hasItem = false; // 아이템 종류와 별개로 칸이 비었는지 기억한다.

    bool Has(ITEM_TYPE requestedItem) const {
        return hasItem && item == requestedItem;
    }
};

// [11.4] 네트워크 기능을 사용하면서 클라이언트의 게임 상태를 관리한다.
class GameClient {
public:
    const char* GetName() const { return inputHandler.GetName(); } // [11.7] 로그인 화면 표시
    const char* GetServerAddress() const { return inputHandler.GetServerAddress(); }
    bool IsServerAddressFocused() const { return inputHandler.IsServerAddressFocused(); }
    bool IsServerAddressSelected() const { return inputHandler.IsServerAddressSelected(); }
    void FocusServerAddress(bool focus) { inputHandler.FocusServerAddress(focus); }
    bool SetServerAddress(const char* address);
    const std::string& GetErrorMessage() const { return errorMessage; } // [11.7] 실패 이유 표시
    int GetMyRoomId() const { return myRoomId; } // [11.11] 입장한 방 번호
    int GetMySlot() const { return mySlot; } // [11.11] 서버가 지정한 자리 번호
    const S2C_GameStartResult* GetGameStartResult() const {
        return gameStartResult.success ? &gameStartResult : nullptr;
    }
    const ClientObject* GetObject(int objectId) const {
        const auto found = objects.find(objectId);
        return found != objects.end() ? &found->second : nullptr;
    }
    // [11.23] 화면에서 물체 목록을 읽기만 할 수 있게 한다.
    const std::unordered_map<int, ClientObject>& GetObjects() const { return objects; }
    int GetHP(int slot) const {
        if (slot < 0 || slot >= MAX_ROOM_PLAYERS) return 0;
        return hp[slot];
    }
    // [11.27] 아이템을 갖고 있으면 그 종류를 알려 준다.
    bool TryGetHeldItem(ITEM_TYPE& item) const {
        if (!inventory.hasItem) return false;
        item = inventory.item;
        return true;
    }
    int GetItemGrantRemainingMs() const; // [11.27] 다음 지급까지의 예상 남은 시간
    bool ShouldFlashHearts() const; // [11.25] 하트의 흰 테두리를 그릴 때인지 알려 준다.
    float GetHitEffectStrength(int slot) const; // [11.25] 해당 자리의 피격 효과 세기를 알려 준다.
    // [12.1] 이 자리의 방패가 마지막으로 공격을 막은 시각이다.
    ULONGLONG GetLastShieldBlockMs(int slot) const {
        if (slot < 0 || slot >= MAX_ROOM_PLAYERS) return 0;
        return lastShieldBlockMs[slot];
    }
    // [11.12] 화면에서 읽을 자리의 참가자 정보를 돌려준다.
    const RoomPlayerInfo* GetRoomPlayer(int slot) const {
        if (slot < 0 || slot >= MAX_ROOM_PLAYERS) return nullptr;
        return &roomPlayers[slot];
    }
    bool IsCharacterConfirmed() const {
        const RoomPlayerInfo* me = GetRoomPlayer(mySlot);
        return me != nullptr && me->connected &&
            (me->character == CHAR_STEVE || me->character == CHAR_ALEX);
    }
    int GetRoomCount() const { return roomCount; } // [11.10] 저장된 방 개수
    int GetSelectedRoomIndex() const { return selectedRoomIndex; } // [11.11] 현재 선택한 목록 위치
    void MoveRoomSelection(int direction); // [11.11] -1은 위, 1은 아래로 방 선택 이동
    // [11.10] 목록 순서로 방을 조회한다. 범위를 벗어나면 nullptr을 반환한다.
    const RoomSummary* GetRoom(int index) const {
        if (index < 0 || index >= roomCount) return nullptr;
        return &rooms[index];
    }
    // [11.7] 닉네임 오류가 나면 화면에 효과를 한 번 요청한다.
    bool ConsumeNameError() {
        const bool occurred = nameErrorEffect;
        nameErrorEffect = false;
        return occurred;
    }
    bool Init(const char* ip);// 서버 IP를 저장하고 네트워크 사용을 준비한다.
    void Shutdown(); // [11.9] 프로그램 종료 시 네트워크 사용을 마무리한다.
    bool TryConnect(); //서버 IP로 연결을 시도하고 성공 여부를 알려 준다.
    void Tick(); // [11.6] 주기적으로 클라이언트 처리를 진행한다.
    bool RequestLogin(const char* name); // [11.6] 닉네임을 검사하고 로그인 요청을 버퍼에 넣는다.
    // [11.11] 요청 데이터의 주소와 크기를 받아 송신 버퍼에 등록한다.
    bool QueueRequest(const void* packet, int packetSize);
    void PollNetwork(); // [11.5] 쌓인 데이터를 보내고 도착한 데이터를 받는다.
    CLIENT_STATE GetState() const { return currentState; } // [11.6] 입력 처리기가 현재 상태를 확인한다.
    void OnKeyUp(unsigned char key) { inputHandler.OnKeyUp(key); }
    void OnKeyDown(unsigned char key) { inputHandler.OnKeyDown(key); } // [11.6] 콜백에서 받은 키를 입력 처리기에 전달한다.
    void OnMouse(int button, int state) { inputHandler.OnMouse(button, state); } // [11.20] 마우스 입력을 전달한다.
    bool TryGetChargeSeconds(float& seconds) const { return inputHandler.TryGetChargeSeconds(seconds); } // [11.24]

private:
    void HandleDisconnect(); // [11.9] 이전 접속 정보를 비우고 첫 로그인 화면으로 돌아간다.
    void HandleGameStartResult(const S2C_GameStartResult& result);
    void ClearWorld();

    // [11.5] ExtractPacket(NetworkClient안에 있음)으로 꺼낸 패킷의 종류와 길이를 확인한다.
    void ProcessPacket(const unsigned char* packet);
    void ApplyWorldPacket(const unsigned char* packet);
    void UpdateVisuals(float dt); // [11.18] 보간

    CLIENT_STATE currentState = CLIENT_LOGIN;
    ULONGLONG lastTickMs = 0;
    ULONGLONG itemGrantCycleStartMs = 0; // [11.27] 경기 시작 또는 마지막 지급 알림을 받은 시각
    int mySessionId = -1; //서버가 구분하기위한 번호
    int myRoomId = -1; // 내가 들어간 방 번호
    int mySlot = -1; // 방 안에서 내가 차지한 자리 번호
    RoomPlayerInfo roomPlayers[MAX_ROOM_PLAYERS]{}; // [11.12] 현재 방의 두 자리 참가자 정보
    S2C_GameStartResult gameStartResult{};
    std::unordered_map<int, ClientObject> objects;
    int hp[MAX_ROOM_PLAYERS]{};
    ULONGLONG lastHitMs[MAX_ROOM_PLAYERS]{};
    ULONGLONG lastShieldBlockMs[MAX_ROOM_PLAYERS]{}; // [12.1] 방패가 공격을 막은 시각
    ClientInventory inventory{}; // [11.27] 내 아이템 보유 상태
    RoomSummary rooms[MAX_ROOMS]{}; // [11.10] 서버에서 받은 방 목록
    int roomCount = 0; // [11.10]
    int selectedRoomIndex = 0; // [11.11] 로비에서는 첫 번째 방부터 선택한다.
    std::string serverIp; // 접속할 서버의 IP 주소
    std::string errorMessage; //실패한 이유를 적어 둘 문자열
    bool nameErrorEffect = false; // [11.7] 아직 화면에 알리지 않은 닉네임 오류
    NetworkClient network; // 실제 소켓 연결과 송수신을 담당
    InputHandler inputHandler{this}; // [11.6] 이 GameClient로 입력 요청을 전달한다.
};
