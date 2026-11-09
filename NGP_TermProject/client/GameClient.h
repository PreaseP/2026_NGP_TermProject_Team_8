#pragma once

#include "NetworkClient.h"
#include "InputHandler.h" // [11.6]
#include <string>

enum CLIENT_STATE { CLIENT_LOGIN, CLIENT_LOBBY, CLIENT_ROOM, CLIENT_PLAYING, CLIENT_RESULT };

// [11.4] 네트워크 기능을 사용하면서 클라이언트의 게임 상태를 관리한다.
class GameClient {
public:
    const char* GetName() const { return inputHandler.GetName(); } // [11.7] 로그인 화면 표시
    const std::string& GetErrorMessage() const { return errorMessage; } // [11.7] 실패 이유 표시
    // [11.7] 닉네임 오류가 나면 화면에 효과를 한 번 요청한다.
    bool ConsumeNameError() {
        const bool occurred = nameErrorEffect;
        nameErrorEffect = false;
        return occurred;
    }
    bool Init(const char* ip);// 서버 IP를 저장하고 네트워크 사용을 준비한다.
    bool TryConnect(); //서버 IP로 연결을 시도하고 성공 여부를 알려 준다.
    void Tick(); // [11.6] 주기적으로 클라이언트 처리를 진행한다.
    bool RequestLogin(const char* name); // [11.6] 닉네임을 검사하고 로그인 요청을 버퍼에 넣는다.
    void PollNetwork(); // [11.5] 쌓인 데이터를 보내고 도착한 데이터를 받는다.
    CLIENT_STATE GetState() const { return currentState; } // [11.6] 입력 처리기가 현재 상태를 확인한다.
    void OnKeyDown(unsigned char key) { inputHandler.OnKeyDown(key); } // [11.6] 콜백에서 받은 키를 입력 처리기에 전달한다.

private:
    void HandleDisconnect(); // [11.9] 이전 접속 정보를 비우고 첫 로그인 화면으로 돌아간다.

    // [11.5] ExtractPacket(NetworkClient안에 있음)으로 꺼낸 패킷의 종류와 길이를 확인한다.
    void ProcessPacket(const unsigned char* packet);

    CLIENT_STATE currentState = CLIENT_LOGIN;
    int mySessionId = -1; //서버가 구분하기위한 번호
    int myRoomId = -1; // 내가 들어간 방 번호
    int mySlot = -1; // 방 안에서 내가 차지한 자리 번호
    std::string serverIp; // 접속할 서버의 IP 주소
    std::string errorMessage; //실패한 이유를 적어 둘 문자열
    bool nameErrorEffect = false; // [11.7] 아직 화면에 알리지 않은 닉네임 오류
    NetworkClient network; // 실제 소켓 연결과 송수신을 담당
    InputHandler inputHandler{this}; // [11.6] 이 GameClient로 입력 요청을 전달한다.
};
