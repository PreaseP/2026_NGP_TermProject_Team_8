#include "GameClient.h"

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
