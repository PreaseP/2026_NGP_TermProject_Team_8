#include "GameClient.h"

#include <cstddef>
#include <cmath>
#include <cstring> // [11.6] 닉네임을 패킷에 복사한다.
#include <Windows.h>

// [11.20] 서버의 발 높이를 모델과 카메라의 기준 높이로 맞춘다.
constexpr float CHARACTER_RENDER_OFFSET_Y = 1.0f;

// [11.30] 서버와 주고받는 아이템 종류인지 확인한다.
static bool IsValidItemType(ITEM_TYPE item)
{
    return item == ITEM_SHIELD || item == ITEM_POWER_UP;
}

// [11.4] 게임 클라이언트의 시작 상태와 네트워크 사용을 준비한다.
bool GameClient::Init(const char* ip)
{
    // 이미 접속해서 사용 중인 상태를 실수로 초기화하지 않도록 한다.
    if (network.IsConnected()) {
        errorMessage = "Cannot initialize while connected.";
        return false;
    }

    // 주소가 없거나 빈 문자열이면 접속할 곳을 저장할 수 없다.
    if (ip == nullptr || ip[0] == '\0') { // 문자열 주소 자체가 없다 || 내용이없는 빈 문자열
        errorMessage = "Server IP is empty.";
        return false;
    }

    // 문자열 내용을 복사한다. 호출자가 전달한 원래 문자열이 사라져도 주소가 남는다.
    serverIp = ip;
    if (!network.InitNetwork()) {
        errorMessage = "Network initialization failed.";
        return false;
    }

    inputHandler.SetServerAddress(serverIp.c_str());
    lastTickMs = GetTickCount64();
    currentState = CLIENT_LOGIN;
    mySessionId = -1;
    myRoomId = -1;
    mySlot = -1;
    ClearWorld();
    // [11.12] 시작할 때 참가자와 캐릭터 선택을 비운다.
    for (RoomPlayerInfo& player : roomPlayers) {
        player = {};
        player.character = CHAR_NONE;
    }
    // [11.10] 시작할 때 방 목록을 비움
    for (RoomSummary& room : rooms) {
        room = {};
    }
    roomCount = 0;
    selectedRoomIndex = 0; // [11.11] 첫 번째 방부터 선택한다.
    inputHandler.Reset(); // [11.6] 시작할 때 이전 이동·공격 입력 기록을 비운다.
    nameErrorEffect = false; // [11.7] 이전 닉네임 오류 효과를 비운다.
    errorMessage.clear(); // 초기화에 성공했으므로 이전 오류 메시지를 지운다.
    return true;
}

// [11.9] 창을 닫을 때 소켓과 송수신 버퍼를 비우고 Winsock 사용을 마친다.
void GameClient::Shutdown()
{
    network.Shutdown();
}

bool GameClient::SetServerAddress(const char* address)
{
    if (currentState != CLIENT_LOGIN) {
        errorMessage = "Server address can only be changed before login.";
        return false;
    }
    in_addr parsed{};
    if (address == nullptr || inet_pton(AF_INET, address, &parsed) != 1) {
        nameErrorEffect = false;
        errorMessage = "Enter a valid server IPv4 address.";
        return false;
    }
    if (serverIp != address) {
        network.Disconnect();
        serverIp = address;
    }
    inputHandler.SetServerAddress(serverIp.c_str());
    errorMessage.clear();
    return true;
}

// [11.4] 서버 IP로 실제 접속을 시도하는 함수
bool GameClient::TryConnect()
{
    // 이미 연결되어 있다면 새 소켓을 만들지 않고 성공으로 처리한다.
    if (network.IsConnected()) {
        errorMessage.clear();
        return true;
    }
    if (serverIp.empty()) {
        errorMessage = "Call Init before connecting.";
        return false;
    }
    if (!network.ConnectToServer(serverIp.c_str())) {
        errorMessage = "Failed to connect to server: " + serverIp;
        return false;
    }

    // 성공하면 로그인 화면 상태로 설정
    currentState = CLIENT_LOGIN;
    errorMessage.clear();
    return true;
}

// [11.9] 연결이 끊기면 이전 접속 정보를 버리고 새 로그인을 준비한다.
void GameClient::HandleDisconnect()
{
    network.Disconnect(); // [11.9] 소켓과 송수신 버퍼를 비운다.
    ClearWorld();
    currentState = CLIENT_LOGIN;
    mySessionId = -1;
    myRoomId = -1;
    mySlot = -1;
    // [11.12] 연결이 끊기면 이전 방의 참가자 정보를 비운다.
    for (RoomPlayerInfo& player : roomPlayers) {
        player = {};
        player.character = CHAR_NONE;
    }
    // [11.10] 연결이 끊기면 이전 서버의 방 목록을 비운다.
    for (RoomSummary& room : rooms) {
        room = {};
    }
    roomCount = 0;
    selectedRoomIndex = 0; // [11.11] 이전 방 선택도 비운다.
    inputHandler = InputHandler{this}; // [11.9] 닉네임과 이동·공격 입력을 모두 비운다.
    inputHandler.SetServerAddress(serverIp.c_str());
    nameErrorEffect = false;
    errorMessage = "Connection lost. Enter your nickname and press GAME START.";
}

// [11.6] 지금은 기존 통신 처리를 한 번 진행한다.
void GameClient::Tick()
{
    const ULONGLONG now = GetTickCount64();
    const float dt = static_cast<float>(now - lastTickMs) / 1000.0f;
    lastTickMs = now;

    inputHandler.ProcessInput();
    PollNetwork();
    UpdateVisuals(dt);
}

// [11.6] 로그인 성공 여부는 이후 서버 응답으로 확인한다.
bool GameClient::RequestLogin(const char* name)
{
    nameErrorEffect = false; // [11.7] 이번 요청의 닉네임 검사 결과를 새로 기록한다.
    if (currentState != CLIENT_LOGIN) {
        errorMessage = "Login is only available on the login screen.";
        return false;
    }
    if (name == nullptr || name[0] == '\0') {
        errorMessage = "Enter a nickname.";
        nameErrorEffect = true; // [11.7] 빈 닉네임도 버튼 오류 효과로 알린다.
        return false;
    }

    // 마지막 한 칸은 문자열 끝을 나타내는 널 문자 자리다.
    std::size_t nameLength = 0;
    while (nameLength < MAX_NAME_LEN && name[nameLength] != '\0') {
        const char ch = name[nameLength];
        if (!((ch >= 'A' && ch <= 'Z') ||
              (ch >= 'a' && ch <= 'z') ||
              (ch >= '0' && ch <= '9'))) {
            errorMessage = "Use only English letters and numbers.";
            nameErrorEffect = true; // [11.7] 허용되지 않는 글자
            return false;
        }
        ++nameLength;
    }
    if (nameLength >= MAX_NAME_LEN) {
        errorMessage = "Nickname is too long.";
        nameErrorEffect = true; // [11.7] 닉네임 길이 초과
        return false;
    }

    // 연결이 없으면 기존 접속 함수를 사용한다. 실패 이유는 그대로 남긴다.
    if (!network.IsConnected() && !TryConnect()) return false;

    C2S_Login packet{}; // 모두 0으로 시작하므로 닉네임 뒤에 널 문자가 남는다.
    packet.size = static_cast<unsigned char>(sizeof(packet));
    packet.type = C2S_LOGIN;
    std::memcpy(packet.name, name, nameLength);

    // 실제 송신은 다음 PollNetwork 호출에서 진행한다.
    if (!network.QueuePacket(&packet)) {
        errorMessage = "Failed to queue login request.";
        return false;
    }
    errorMessage.clear();
    return true;
}

// [11.11] 방향키로 선택 위치만 바꾼다. 끝에 도달하면 현재 선택을 유지한다.
void GameClient::MoveRoomSelection(int direction)
{
    if (currentState != CLIENT_LOBBY) return;
    if (direction != -1 && direction != 1) return;

    const int nextIndex = selectedRoomIndex + direction;
    if (nextIndex >= 0 && nextIndex < roomCount) {
        selectedRoomIndex = nextIndex;
    }
}

// [11.11] 호출자는 요청 종류(type)와 내용을 채운다. 실제 송신은 PollNetwork에서 한다.
bool GameClient::QueueRequest(const void* packet, int packetSize)
{
    if (currentState == CLIENT_LOGIN) {
        errorMessage = "Log in before sending requests.";
        return false;
    }
    // [11.11] size와 type으로 최소 2바이트가 필요하고, size는 최대 255다.
    if (packet == nullptr || packetSize < 2 || packetSize > 255) {
        errorMessage = "Invalid request size.";
        return false;
    }

    unsigned char request[BUF_SIZE]{};
    std::memcpy(request, packet, packetSize);
    if (request[1] < C2S_JOIN_ROOM || request[1] > C2S_USE_ITEM) {
        errorMessage = "Invalid request type.";
        return false;
    }

    switch (request[1]) {
    case C2S_LEAVE_ROOM:
        // [11.12] 퇴장은 대기실에서 정해진 크기의 패킷으로만 요청한다.
        if (currentState != CLIENT_ROOM || packetSize != sizeof(C2S_LeaveRoom)) {
            errorMessage = "Invalid leave room request.";
            return false;
        }
        break;

    case C2S_SELECT_CHARACTER:
        // [11.14] 캐릭터 선택은 대기실에서 정해진 패킷과 캐릭터로만 요청한다.
        if (currentState != CLIENT_ROOM || packetSize != sizeof(C2S_SelectCharacter)) {
            errorMessage = "Invalid character selection request.";
            return false;
        }
        if (request[2] != CHAR_STEVE && request[2] != CHAR_ALEX) {
            errorMessage = "Invalid character selection.";
            return false;
        }
        if (IsCharacterConfirmed()) {
            errorMessage = "Character selection is already confirmed.";
            return false;
        }
        break;

    case C2S_GAME_START:
        if (currentState != CLIENT_ROOM || packetSize != sizeof(C2S_GameStart)) {
            errorMessage = "Invalid game start request.";
            return false;
        }
        break;

    case C2S_MOVE: {
        const unsigned char allowedKeys = KEY_FORWARD | KEY_BACK | KEY_LEFT | KEY_RIGHT;
        if (currentState != CLIENT_PLAYING || packetSize != sizeof(C2S_Move) ||
            (request[2] & ~allowedKeys) != 0) {
            errorMessage = "Invalid movement request.";
            return false;
        }
        break;
    }

    // [11.20] 경기 중에는 정해진 크기의 차징 시작 요청만 받는다.
    case C2S_CHARGE_START:
        if (currentState != CLIENT_PLAYING || packetSize != sizeof(C2S_ChargeStart)) {
            errorMessage = "Invalid charge start request.";
            return false;
        }
        break;

    // [11.20] 던지기 요청도 경기 상태와 패킷 크기를 확인한다.
    case C2S_THROW:
        if (currentState != CLIENT_PLAYING || packetSize != sizeof(C2S_Throw)) {
            errorMessage = "Invalid throw request.";
            return false;
        }
        break;

    // [11.27] 경기 중 실제로 갖고 있는 아이템만 사용 요청에 넣는다.
    case C2S_USE_ITEM: {
        if (currentState != CLIENT_PLAYING || packetSize != sizeof(C2S_UseItem)) {
            errorMessage = "Invalid use item request.";
            return false;
        }

        const ITEM_TYPE item = static_cast<ITEM_TYPE>(request[offsetof(C2S_UseItem, item)]);
        if (!IsValidItemType(item)) {
            errorMessage = "Invalid item type.";
            return false;
        }
        if (!inventory.Has(item)) {
            errorMessage = "Item is not in inventory.";
            return false;
        }
        break;
    }

    default:
        break;
    }

    request[0] = static_cast<unsigned char>(packetSize); // [11.11] 첫 바이트에 전체 크기를 넣는다.
    if (!network.QueuePacket(request)) {
        HandleDisconnect(); // [11.11] 연결이 없으면 기존 로그인 복귀 처리를 사용한다.
        return false;
    }
    // [11.12] 퇴장 응답 패킷이 없으므로 등록 성공 후 로비로 돌아간다.
    if (request[1] == C2S_LEAVE_ROOM) {
        ClearWorld();
        myRoomId = -1;
        mySlot = -1;
        for (RoomPlayerInfo& player : roomPlayers) {
            player = {};
            player.character = CHAR_NONE;
        }
        inputHandler.Reset(); // 닉네임은 유지하고 이전 이동·공격 입력만 비운다.
        currentState = CLIENT_LOBBY;
    }
    errorMessage.clear();
    return true; // [11.11] 송신 버퍼에 등록한 상태다.
}

// [11.5]
void GameClient::PollNetwork()
{
    // 연결 전이거나 이미 연결을 정리했다면 할 일이 없다.
    if (!network.IsConnected()) return;

    // QueuePacket으로 모아 둔 데이터를 실제로 보낸다.
    const NET_RESULT sendResult = network.FlushSendBuffer();
    if (sendResult == NET_CLOSED || sendResult == NET_ERROR) {
        HandleDisconnect(); // [11.9] 오류( 데이터를 보내다가 오류 or 연결종료 발생)
        return;
    }
    // 송신이 잠시 막혀도 수신은 시도한다. 못 보낸 데이터는 다음 호출에서 다시 보낸다.
    const NET_RESULT recvResult = network.ReceiveData();

    // [11.5] 서버가 연결을 끊었어도 그 전에 받은 완성된 패킷은 먼저 처리한다.
    unsigned char packet[BUF_SIZE];
    while (true) {
        const NET_RESULT packetResult = network.ExtractPacket(packet);
        if (packetResult == NET_WOULD_BLOCK) break; // 아직 완성된 패킷이 없다.
        if (packetResult != NET_OK) {
            HandleDisconnect(); // [11.9] 오류( 받은 데이터에서 패킷을 꺼내는데 길이가 잘못됨 )
            return;
        }

        ProcessPacket(packet);
        // 패킷 검사에서 연결을 끊었다면 그 오류 메시지를 유지하고 끝낸다.
        if (!network.IsConnected()) return;
    }

    if (recvResult == NET_CLOSED || recvResult == NET_ERROR) {
        HandleDisconnect(); // [11.9] 오류( 서버가 연결을 닫았거나 수신 오류 발생 )
        return;
    }
}

void GameClient::ClearWorld()
{
    objects.clear();
    inventory = {}; // [11.30] 이전 경기의 아이템을 비운다.
    itemGrantCycleStartMs = 0; // [11.27] 이전 경기의 지급 시간도 비운다.
    for (int slot = 0; slot < MAX_ROOM_PLAYERS; ++slot) {
        hp[slot] = 0;
        // [11.25] 이전 경기의 피격 표시를 지운다.
        lastHitMs[slot] = 0;
        lastShieldBlockMs[slot] = 0; // [12.1] 이전 경기에 막았던 기록을 지운다.
    }
    gameStartResult = {};
    gameOverResult = {};
    for (CHARACTER_TYPE& character : gameStartResult.character) {
        character = CHAR_NONE;
    }
}

// [11.27] 경기 시작 또는 마지막 지급 알림부터 20초 주기를 계산한다.
int GameClient::GetItemGrantRemainingMs() const
{
    if (currentState != CLIENT_PLAYING) return 0;
    const ULONGLONG elapsed = GetTickCount64() - itemGrantCycleStartMs;
    return ITEM_INTERVAL_MS - static_cast<int>(elapsed % ITEM_INTERVAL_MS);
}

bool GameClient::ShouldFlashHearts() const // 이번 프레임에 하트의 흰 테두리를 켤지 검사
{
    if (mySlot < 0 || mySlot >= MAX_ROOM_PLAYERS || lastHitMs[mySlot] == 0) return false;

    // [11.25] 140ms마다 일반 하트와 흰 테두리 하트를 세 번 번갈아 표시한다.
    constexpr ULONGLONG phaseMs = 140;
    constexpr ULONGLONG phaseCount = 6;
    const ULONGLONG phase = (GetTickCount64() - lastHitMs[mySlot]) / phaseMs;
    return phase < phaseCount && phase % 2 == 1;
}

float GameClient::GetHitEffectStrength(int slot) const //카메라와 캐릭터 색 효과를 얼마나 강하게 줄지 (피격시)
{
    if (slot < 0 || slot >= MAX_ROOM_PLAYERS || lastHitMs[slot] == 0) return 0.0f;

    // [11.25] 피격 직후 가장 강하고 150ms 안에 부드럽게 사라진다.
    constexpr ULONGLONG durationMs = 150;
    const ULONGLONG elapsedMs = GetTickCount64() - lastHitMs[slot];
    if (elapsedMs >= durationMs) return 0.0f;

    const float remaining = 1.0f - static_cast<float>(elapsedMs) / durationMs;
    return remaining * remaining * (3.0f - 2.0f * remaining);
}

void GameClient::HandleGameStartResult(const S2C_GameStartResult& result)
{
    if (!result.success) {
        errorMessage = "Game could not start. Check players and character selections.";
        return;
    }
    for (int slot = 0; slot < MAX_ROOM_PLAYERS; ++slot) {
        if ((result.character[slot] != CHAR_STEVE && result.character[slot] != CHAR_ALEX) ||
            !std::isfinite(result.x[slot]) || !std::isfinite(result.y[slot]) ||
            !std::isfinite(result.z[slot])) {
            HandleDisconnect();
            return;
        }
    }
    ClearWorld();
    gameStartResult = result;
    itemGrantCycleStartMs = GetTickCount64(); // [11.27] 서버 시각을 받기 전까지 쓸 표시 기준
    for (int slot = 0; slot < MAX_ROOM_PLAYERS; ++slot) {
        ClientObject player{};
        player.objectId = slot;
        player.type = OBJ_CHARACTER;
        player.character = result.character[slot];
        player.renderPos = glm::vec3(result.x[slot], result.y[slot] + CHARACTER_RENDER_OFFSET_Y, result.z[slot]);
        player.prevPos = player.renderPos;
        player.targetPos = player.renderPos;
        player.lerpT = 1.0f;
        player.armState = ARM_IDLE;
        objects[slot] = player;
        hp[slot] = MAX_HP;
    }
    inputHandler.Reset();
    errorMessage.clear();
    currentState = CLIENT_PLAYING;
}


void GameClient::ApplyWorldPacket(const unsigned char* packet)
{
    // [11.23] 생성 패킷을 읽고 눈덩이에 필요한 값을 확인한다.
    if (packet[1] == S2C_ADD_OBJECT) {
        S2C_AddObject added{};
        std::memcpy(&added, packet, sizeof(added));
        // [11.26] 서버가 알려 준 눈 벽의 번호, 위치, 높이를 저장한다.
        if (added.obj_type == OBJ_SNOW_WALL) {
            if (added.object_id < OBJ_ID_SNOW_WALL_START ||
                !std::isfinite(added.x) || !std::isfinite(added.y) || !std::isfinite(added.z) ||
                !std::isfinite(added.height) || added.height <= 0.0f) {
                HandleDisconnect();
                return;
            }

            ClientObject wall{};
            wall.objectId = added.object_id;
            wall.type = OBJ_SNOW_WALL;
            wall.height = added.height;
            wall.targetHeight = added.height; // [11.26] 처음에는 현재 높이와 목표 높이가 같다.
            wall.startHeight = added.height;
            wall.wallBaseY = added.y - added.height * 0.5f; // [11.26] 중심에서 높이의 절반을 빼면 밑면이다.
            wall.renderPos = glm::vec3(added.x, added.y, added.z);
            wall.prevPos = wall.renderPos;
            wall.targetPos = wall.renderPos;
            objects[wall.objectId] = wall;
            return;
        }

        // [11.23] 강화 여부는 bool로 사용하기 전에 0 또는 1인지 확인한다.
        const unsigned char poweredValue = packet[offsetof(S2C_AddObject, powered)];
        if (added.obj_type != OBJ_SNOWBALL ||
            added.object_id < OBJ_ID_SNOWBALL_START ||
            added.object_id >= OBJ_ID_SNOW_WALL_START ||
            added.owner_slot >= MAX_ROOM_PLAYERS ||
            !std::isfinite(added.x) || !std::isfinite(added.y) || !std::isfinite(added.z) ||
            poweredValue > 1) {
            HandleDisconnect();
            return;
        }
        // [11.23] 서버가 알려 준 눈덩이 정보를 저장한다.
        ClientObject snowball{};
        snowball.objectId = added.object_id;
        snowball.type = OBJ_SNOWBALL;
        snowball.ownerSlot = added.owner_slot;
        snowball.powered = added.powered;
        // [11.23] 처음에는 세 위치를 생성 위치로 맞춰 바로 놓는다.
        snowball.renderPos = glm::vec3(added.x, added.y, added.z);
        snowball.prevPos = snowball.renderPos;
        snowball.targetPos = snowball.renderPos;
        snowball.lerpT = 1.0f;
        objects[snowball.objectId] = snowball;
        return;
    }

    // [11.24] 서버가 지우라고 한 눈덩이를 목록에서 뺀다.
    if (packet[1] == S2C_REMOVE_OBJECT) {
        S2C_RemoveObject removed{};
        std::memcpy(&removed, packet, sizeof(removed));
        const auto found = objects.find(removed.object_id);
        if (found != objects.end() && found->second.type == OBJ_SNOWBALL) {
            objects.erase(found);
        }
        return;
    }

    // [11.25] 맞은 자리와 남은 체력을 확인하고 저장한다.
    if (packet[1] == S2C_HIT) {
        S2C_Hit hit{};
        std::memcpy(&hit, packet, sizeof(hit));
        const unsigned char blockedValue = packet[offsetof(S2C_Hit, blocked)];
        if (hit.target_slot >= MAX_ROOM_PLAYERS || hit.hp < 0 || hit.hp > MAX_HP || blockedValue > 1) {
            HandleDisconnect();
            return;
        }
        // [12.1] 방패로 막았다는 응답이 오면 흰 고리를 시작할 시각을 저장한다.
        if (hit.blocked) {
            lastShieldBlockMs[hit.target_slot] = GetTickCount64();
        }
        int& targetHp = hp[hit.target_slot];
        // [11.25] 체력이 줄어든 자리의 피격 시각을 기록한다.
        if (hit.hp < targetHp) {
            lastHitMs[hit.target_slot] = GetTickCount64();
        }
        targetHp = hit.hp;
        return;
    }

    // [11.30] 서버가 내게 지급한 아이템을 빈 칸에 넣는다.
    if (packet[1] == S2C_ITEM_GET) {
        S2C_ItemGet received{};
        std::memcpy(&received, packet, sizeof(received));
        if (!IsValidItemType(received.item)) {
            HandleDisconnect();
            return;
        }
        // [11.27] 실제 지급 알림을 받으면 다음 20초 주기를 여기서 다시 맞춘다.
        itemGrantCycleStartMs = GetTickCount64();
        if (!inventory.hasItem) {
            inventory = { received.item, true };
        }
        return;
    }

    // [11.30] 내가 보유한 아이템의 사용이 확인되면 칸을 비운다.
    if (packet[1] == S2C_ITEM_USED) {
        S2C_ItemUsed used{};
        std::memcpy(&used, packet, sizeof(used));
        if (used.user_slot >= MAX_ROOM_PLAYERS || !IsValidItemType(used.item)) {
            HandleDisconnect();
            return;
        }
        if (used.user_slot == mySlot && inventory.Has(used.item)) {
            inventory = {};
        }
        return;
    }

    if (packet[1] == S2C_STATUS_CHANGE) {
        S2C_StatusChange state{};
        std::memcpy(&state, packet, sizeof(state));

        // [11.26] 같은 번호의 눈 벽을 찾아 서버가 알려 준 목표 높이를 저장한다.
        if (state.status == STATUS_WALL_HEIGHT) {
            if (!std::isfinite(state.value) || state.value <= 0.0f) {
                HandleDisconnect();
                return;
            }
            const auto found = objects.find(state.object_id);
            if (found == objects.end() || found->second.type != OBJ_SNOW_WALL) return;

            ClientObject& wall = found->second;
            if (wall.targetHeight == state.value) return;

            // [11.26] 지금 보이는 높이에서 새 목표 높이까지 이어서 바꾼다.
            wall.startHeight = wall.height;
            wall.targetHeight = state.value;
            wall.lerpT = 0.0f;
            /*
            벽이 0.5 → 1.0으로 자라는 중에 0.8까지 왔고, 서버가 새 목표 1.5를 보내면 
            startHeight는 0.8, targetHeight는 1.5가 된다. 
            그래서 0.8부터 자연스럽게 이어서 자람.
            */
            return;
        }

        // [12.1] 서버가 알려 준 자리의 방패 또는 공격 강화를 켜거나 끈다.
        if (state.status == STATUS_SHIELD || state.status == STATUS_POWER_UP) {
            if (state.object_id < 0 || state.object_id >= MAX_ROOM_PLAYERS ||
                (state.value != 0.0f && state.value != 1.0f)) {
                HandleDisconnect();
                return;
            }

            const auto found = objects.find(state.object_id);
            if (found == objects.end() || found->second.type != OBJ_CHARACTER) return;

            ClientObject& player = found->second;
            const bool active = (state.value == 1.0f);
            if (state.status == STATUS_SHIELD) {
                player.shieldActive = active;
            } else {
                player.powerUpActive = active;
            }
            return;
        }

        if (state.status != STATUS_ARM_STATE) return;
        if (state.object_id < 0 ||
            (state.value != ARM_IDLE && state.value != ARM_RUN &&
             state.value != ARM_CHARGE && state.value != ARM_THROW)) {
            HandleDisconnect();
            return;
        }
        const auto found = objects.find(state.object_id);
        if (found == objects.end() || found->second.type != OBJ_CHARACTER) return;
        found->second.armState = static_cast<ARM_STATE>(state.value);
        return;
    }
    if (packet[1] != S2C_MOVE_OBJECT) return;
    S2C_MoveObject movement{};
    std::memcpy(&movement, packet, sizeof(movement));
    if (movement.object_id < 0 || !std::isfinite(movement.x) ||
        !std::isfinite(movement.y) || !std::isfinite(movement.z)) {
        HandleDisconnect();
        return;
    }
    const auto found = objects.find(movement.object_id);
    if (found == objects.end()) return;

    // [11.24] 캐릭터와 눈덩이의 이동만 반영한다.
    ClientObject& object = found->second;
    if (object.type != OBJ_CHARACTER && object.type != OBJ_SNOWBALL) return;

    // [11.23] 현재 이동 보정은 캐릭터에만 적용한다.
    float targetY = movement.y;
    if (object.type == OBJ_CHARACTER) {
        targetY += CHARACTER_RENDER_OFFSET_Y;
    }

    // [11.24] 지금 보이는 위치에서 새 목표 위치로 이동을 시작한다.
    object.prevPos = object.renderPos;
    object.targetPos = glm::vec3(movement.x, targetY, movement.z);
    object.lerpT = 0.0f;
}

void GameClient::UpdateVisuals(float dt)
{
    if (currentState != CLIENT_PLAYING) return;
    constexpr float moveDuration = 0.033f;
    constexpr float wallHeightDuration = 1.5f; // [11.26] 기존 게임의 눈 벽 성장 시간이다.
    for (auto& entry : objects) {
        ClientObject& object = entry.second;
        if (object.lerpT >= 1.0f) continue;

        if (object.type == OBJ_SNOW_WALL) {
            object.lerpT = glm::min(object.lerpT + dt / wallHeightDuration, 1.0f);
            // [11.26] 처음에 빠르게 자라고 목표 높이에 가까워지면 천천히 자란다.
            const float progress = 1.0f - std::pow(1.0f - object.lerpT, 4.0f);
            object.height = glm::mix(object.startHeight, object.targetHeight, progress);
            object.renderPos.y = object.wallBaseY + object.height * 0.5f;
            continue;
        }

        object.lerpT = glm::min(object.lerpT + dt / moveDuration, 1.0f);
        object.renderPos = glm::mix(object.prevPos, object.targetPos, object.lerpT); //glm::mix(출발점, 목표점, 진행도)는 두 위치 사이에서 진행도에 해당하는 위치를 계산
    }
}

// [11.5]패킷 검사와 종류 구분
void GameClient::ProcessPacket(const unsigned char* packet)
{
    // 이 함수에는 ExtractPacket으로 끝까지 꺼낸 패킷을 전달한다.
    if (packet == nullptr || packet[0] < sizeof(PacketHeader)) {
        HandleDisconnect(); // [11.9] 오류( 패킷이 없거나 기본 헤더보다 길이가 짧을때 )
        return;
    }

    // type은 두 번째 바이트다. 종류마다 필요한 전체 길이를 확인한다.
    std::size_t expectedSize = 0;
    switch (packet[1]) {
    case S2C_LOGIN_RESULT: // 로그인 결과
        expectedSize = sizeof(S2C_LoginResult);
        break;
    case S2C_ROOM_LIST: // 방 목록
        expectedSize = sizeof(S2C_RoomList);
        break;
    case S2C_JOIN_ROOM_RESULT: // 방 입장 결과
        expectedSize = sizeof(S2C_JoinRoomResult);
        break;
    case S2C_ROOM_INFO: // 대기실 정보
        expectedSize = sizeof(S2C_RoomInfo);
        break;
    case S2C_GAME_START_RESULT: // 게임 시작 결과
        expectedSize = sizeof(S2C_GameStartResult);
        break;
    case S2C_ADD_OBJECT: // 오브젝트 생성
        expectedSize = sizeof(S2C_AddObject);
        break;
    case S2C_MOVE_OBJECT: // 오브젝트 위치 변경
        expectedSize = sizeof(S2C_MoveObject);
        break;
    case S2C_REMOVE_OBJECT: // 오브젝트 제거
        expectedSize = sizeof(S2C_RemoveObject);
        break;
    case S2C_STATUS_CHANGE: // 오브젝트 상태 변경
        expectedSize = sizeof(S2C_StatusChange);
        break;
    case S2C_HIT: // 피격 결과
        expectedSize = sizeof(S2C_Hit);
        break;
    case S2C_ITEM_GET: // 아이템 획득
        expectedSize = sizeof(S2C_ItemGet);
        break;
    case S2C_ITEM_USED: // 아이템 사용
        expectedSize = sizeof(S2C_ItemUsed);
        break;
    case S2C_GAME_OVER: // 게임 종료
        expectedSize = sizeof(S2C_GameOver);
        break;
    default: // 서버가 보내기로 정하지 않은 종류
        HandleDisconnect(); // [11.9] 오류( 정의하지 않은 패킷 종류 )
        return;
    }

    if (packet[0] != expectedSize) {
        HandleDisconnect(); // [11.9] 오류 ( 패킷 종류에 맞는 길이가 아님 )
        return;
    }

    // 종류와 길이를 확인했다.
    if (packet[1] == S2C_GAME_START_RESULT && currentState == CLIENT_ROOM) {
        const unsigned char success = packet[offsetof(S2C_GameStartResult, success)];
        if (success > 1) {
            HandleDisconnect();
            return;
        }
        S2C_GameStartResult result{};
        std::memcpy(&result, packet, sizeof(result));
        HandleGameStartResult(result);
        return;
    }
    // [11.28] 경기 중 받은 서버 결과를 저장하고 결과 화면으로 바꾼다.
    if (packet[1] == S2C_GAME_OVER && currentState == CLIENT_PLAYING) {
        S2C_GameOver result{};
        std::memcpy(&result, packet, sizeof(result));
        if (result.winner_slot >= MAX_ROOM_PLAYERS ||
            (result.reason != END_HP_ZERO && result.reason != END_OPPONENT_LEFT)) {
            HandleDisconnect();
            return;
        }

        gameOverResult = result;
        inputHandler.Reset();
        currentState = CLIENT_RESULT;
        return;
    }
    // [11.23] 경기 중 생성 패킷도 월드 처리로 넘긴다.
    // [11.30] 아이템 지급과 사용 알림도 월드 처리에 맡긴다.
    if (currentState == CLIENT_PLAYING &&
        (packet[1] == S2C_ADD_OBJECT || packet[1] == S2C_MOVE_OBJECT ||
         packet[1] == S2C_REMOVE_OBJECT || packet[1] == S2C_STATUS_CHANGE ||
         packet[1] == S2C_HIT || packet[1] == S2C_ITEM_GET ||
         packet[1] == S2C_ITEM_USED)) {
        ApplyWorldPacket(packet);
        return;
    }
    // [11.7] 로그인 성공 응답을 받은 뒤에만 로비로 이동한다.
    if (packet[1] == S2C_LOGIN_RESULT && currentState == CLIENT_LOGIN) {
        S2C_LoginResult result{};
        std::memcpy(&result, packet, sizeof(result));
        if (result.result == LOGIN_OK && result.session_id >= 0) {
            mySessionId = result.session_id;
            currentState = CLIENT_LOBBY;
            selectedRoomIndex = 0; // [11.11] 로그인 후 첫 번째 방을 선택한다.
            inputHandler.Reset();
            errorMessage.clear();
        }
        else if (result.result == LOGIN_INVALID_NAME) {
            errorMessage = "Nickname rejected. Use 1-15 letters or numbers.";
            nameErrorEffect = true; // [11.7] 서버가 닉네임을 거절한 경우도 표시한다.
        }
        else if (result.result == LOGIN_SERVER_FULL) {
            errorMessage = "Server is full. Please try again later.";
        }
        else {
            HandleDisconnect(); // [11.9] 오류 ( 로그인 응답 값이 잘못됨 )
        }
    }

    // [11.11] 방 입장 결과를 읽고 성공한 경우에만 대기실 상태로 바꾼다.
    if (packet[1] == S2C_JOIN_ROOM_RESULT && currentState == CLIENT_LOBBY) {
        S2C_JoinRoomResult result{};
        std::memcpy(&result, packet, sizeof(result));
        switch (result.result) {
        case JOIN_OK:
            if (result.room_id >= MAX_ROOMS || result.my_slot >= MAX_ROOM_PLAYERS) {
                HandleDisconnect(); // [11.11] 성공 응답의 방·자리 번호가 잘못됐다.
                return;
            }
            myRoomId = result.room_id;
            mySlot = result.my_slot;
            ClearWorld();
            // [11.12] 새 방의 참가자 정보를 받을 준비를 한다.
            for (RoomPlayerInfo& player : roomPlayers) {
                player = {};
                player.character = CHAR_NONE;
            }
            currentState = CLIENT_ROOM;
            inputHandler.Reset();
            errorMessage.clear();
            break;
        case JOIN_ROOM_FULL:
            errorMessage = "Room is full. Choose another room.";
            break;
        case JOIN_ROOM_PLAYING:
            errorMessage = "Room is already playing. Choose another room.";
            break;
        case JOIN_INVALID_ROOM:
            errorMessage = "Room does not exist. Choose another room.";
            break;
        default:
            HandleDisconnect(); // [11.11] 정의된 입장 결과가 아니다.
            return;
        }
    }

    // [11.28] 정상 종료와 상대 이탈 모두 결과 화면에서 방 복귀를 기다린다.
    const bool returningAfterMatch = currentState == CLIENT_RESULT;
    // [11.12] 현재 방의 참가자 정보를 검사하고 저장한다.
    if (packet[1] == S2C_ROOM_INFO && (currentState == CLIENT_ROOM || returningAfterMatch)) {
        S2C_RoomInfo roomInfo{};
        std::memcpy(&roomInfo, packet, sizeof(roomInfo));
        if (roomInfo.room_id >= MAX_ROOMS || roomInfo.room_id != myRoomId) {
            HandleDisconnect(); // [11.12] 현재 들어간 방의 정보가 아니다.
            return;
        }

        for (RoomPlayerInfo& player : roomInfo.players) {
            // [11.12] 접속 여부는 0 또는 1이어야 한다.
            unsigned char connected = 0;
            std::memcpy(&connected, &player.connected, sizeof(connected));
            if (connected > 1) {
                HandleDisconnect();
                return;
            }
            if (connected == 0) {
                player = {}; // [11.12] 빈 자리의 이전 이름과 캐릭터를 지운다.
                player.character = CHAR_NONE;
                continue;
            }
            if (player.character != CHAR_STEVE && player.character != CHAR_ALEX &&
                player.character != CHAR_NONE) {
                HandleDisconnect(); // [11.12] 정해진 캐릭터 값이 아니다.
                return;
            }

            // [11.12] 이름 규칙은 다시 검사하지 않고 문자열 끝만 보장한다.
            player.name[MAX_NAME_LEN - 1] = '\0';
        }

        if (returningAfterMatch) {
            if (mySlot < 0 || mySlot >= MAX_ROOM_PLAYERS ||
                !roomInfo.players[mySlot].connected ||
                roomInfo.players[mySlot].character != CHAR_NONE) {
                return;
            }

            // [11.28] 경기 데이터를 비우고 같은 방의 대기실로 돌아간다.
            ClearWorld();
            inputHandler.Reset();
            errorMessage.clear();
            currentState = CLIENT_ROOM;
        }

        // [11.12] 두 자리 모두 검사가 끝나면 저장된 정보를 한 번에 바꾼다.
        std::memcpy(roomPlayers, roomInfo.players, sizeof(roomPlayers));
    }

    // [11.10] 로비에서 받은 방 목록을 검사하고 저장한다.
    if (packet[1] == S2C_ROOM_LIST && currentState == CLIENT_LOBBY) {
        S2C_RoomList roomList{};
        std::memcpy(&roomList, packet, sizeof(roomList));
        if (roomList.room_count > MAX_ROOMS) {
            HandleDisconnect(); // [11.10] 저장할 수 있는 방 수를 넘었다.
            return;
        }

        RoomSummary newRooms[MAX_ROOMS]{};
        const RoomSummary* selectedRoom = GetRoom(selectedRoomIndex); // [11.11] 갱신 전 선택한 방
        int newSelectedIndex = 0; // [11.11] 선택한 방이 사라지면 첫 번째 방으로 돌아간다.
        bool seenRooms[MAX_ROOMS]{}; // [11.10] 같은 방 번호가 두 번 오는지 확인한다.
        for (int i = 0; i < roomList.room_count; ++i) {
            const RoomSummary& room = roomList.rooms[i];
            if (room.room_id >= MAX_ROOMS || seenRooms[room.room_id] ||
                room.player_count > MAX_ROOM_PLAYERS ||
                (room.state != ROOM_WAITING && room.state != ROOM_PLAYING && room.state != ROOM_RESULT)) {
                HandleDisconnect(); // [11.10] 방 번호·인원·상태가 잘못됐다.
                return;
            }
            seenRooms[room.room_id] = true;
            newRooms[i] = room;
            if (selectedRoom != nullptr && selectedRoom->room_id == room.room_id) {
                newSelectedIndex = i; // [11.11] 목록 순서가 바뀌어도 같은 방을 선택한다.
            }
        }

        // [11.10] 모든 검사가 끝나면 이전 목록을 한 번에 바꾼다.
        std::memcpy(rooms, newRooms, sizeof(rooms));
        roomCount = roomList.room_count;
        selectedRoomIndex = newSelectedIndex;
    }
}
