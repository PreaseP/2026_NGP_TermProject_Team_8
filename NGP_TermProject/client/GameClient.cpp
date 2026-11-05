#include "GameClient.h"

#include <cstddef>

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
    errorMessage.clear(); // 초기화에 성공했으므로 이전 오류 메시지를 지운다.
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

// [11.5]
void GameClient::PollNetwork()
{
    // 연결 전이거나 이미 연결을 정리했다면 할 일이 없다.
    if (!network.IsConnected()) return;

    // QueuePacket으로 모아 둔 데이터를 실제로 보낸다.
    const NET_RESULT sendResult = network.FlushSendBuffer();
    if (sendResult == NET_CLOSED || sendResult == NET_ERROR) {
        if (sendResult == NET_CLOSED) { // 연결이 종료됨
            errorMessage = "Connection closed while sending.";
        }
        else { // NET_ERROR: 데이터를 보내다가 오류 발생
            errorMessage = "Failed to send data.";
        }
        network.Disconnect();
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
            errorMessage = "Invalid packet length.";
            network.Disconnect();
            return;
        }

        ProcessPacket(packet);
        // 패킷 검사에서 연결을 끊었다면 그 오류 메시지를 유지하고 끝낸다.
        if (!network.IsConnected()) return;
    }

    if (recvResult == NET_CLOSED || recvResult == NET_ERROR) {
        if (recvResult == NET_CLOSED) { // 서버가 정상적으로 연결을 종료함
            errorMessage = "Server closed the connection.";
        }
        else { // NET_ERROR: 데이터를 받다가 오류 발생
            errorMessage = "Failed to receive data.";
        }
        network.Disconnect();
        return;
    }

}

// [11.5] 이번 단계는 패킷 검사와 종류 구분까지다. 게임 상태 변경은 이후에 연결한다.
void GameClient::ProcessPacket(const unsigned char* packet)
{
    // 이 함수에는 ExtractPacket으로 끝까지 꺼낸 패킷을 전달한다.
    if (packet == nullptr || packet[0] < sizeof(PacketHeader)) {
        errorMessage = "Invalid packet header.";
        network.Disconnect();
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
        errorMessage = "Unknown server packet type.";
        network.Disconnect();
        return;
    }

    if (packet[0] != expectedSize) {
        errorMessage = "Packet size does not match its type.";
        network.Disconnect();
        return;
    }

    // 종류와 길이를 확인했다. 각 Handle 함수 호출은 해당 기능을 만들 때 추가한다.
}
