#pragma once

#include "NetworkClient.h"
#include <string>

enum CLIENT_STATE { CLIENT_LOGIN, CLIENT_LOBBY, CLIENT_ROOM, CLIENT_PLAYING, CLIENT_RESULT };

// [11.4] 네트워크 기능을 사용하면서 클라이언트의 게임 상태를 관리한다.
class GameClient {
public:
    bool Init(const char* ip);// 서버 IP를 저장하고 네트워크 사용을 준비한다.
    bool TryConnect(); //서버 IP로 연결을 시도하고 성공 여부를 알려 준다.
    void PollNetwork(); // [11.5] 쌓인 데이터를 보내고 도착한 데이터를 받는다.

private:
    // [11.5] ExtractPacket(NetworkClient안에 있음)으로 꺼낸 패킷의 종류와 길이를 확인한다.
    void ProcessPacket(const unsigned char* packet);

    CLIENT_STATE currentState = CLIENT_LOGIN;
    int mySessionId = -1; //서버가 구분하기위한 번호
    int myRoomId = -1; // 내가 들어간 방 번호
    int mySlot = -1; // 방 안에서 내가 차지한 자리 번호
    std::string serverIp; // 접속할 서버의 IP 주소
    std::string errorMessage; //실패한 이유를 적어 둘 문자열
    NetworkClient network; // 실제 소켓 연결과 송수신을 담당
};
