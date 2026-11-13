#include "GameClient.h"

#include <cstddef>
#include <cstring> // [11.6] 닉네임을 패킷에 복사한다.

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

    currentState = CLIENT_LOGIN;
    mySessionId = -1;
    myRoomId = -1;
    mySlot = -1;
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
    nameErrorEffect = false;
    errorMessage = "Connection lost. Enter your nickname and press GAME START.";
}

// [11.6] 지금은 기존 통신 처리를 한 번 진행한다.
void GameClient::Tick()
{
    PollNetwork();
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

    request[0] = static_cast<unsigned char>(packetSize); // [11.11] 첫 바이트에 전체 크기를 넣는다.
    if (!network.QueuePacket(request)) {
        HandleDisconnect(); // [11.11] 연결이 없으면 기존 로그인 복귀 처리를 사용한다.
        return false;
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

    // [11.12] 현재 방의 참가자 정보를 검사하고 저장한다.
    if (packet[1] == S2C_ROOM_INFO && currentState == CLIENT_ROOM) {
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
