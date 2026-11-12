#pragma once

#include "../common/protocol.h"

class GameClient;

// [11.6] 입력 상태와 로그인에 사용할 닉네임을 보관한다.
class InputHandler {
public:
    const char* GetName() const { return nameBuffer; } // [11.7] 화면에 입력 중인 닉네임을 보여 준다.
    // [11.6] 입력으로 만든 요청을 전달할 GameClient를 기억한다.
    explicit InputHandler(GameClient* target) : game(target) {}
    // [11.11] 로비에서는 Enter로 현재 선택한 방의 입장을 요청한다.
    void OnKeyDown(unsigned char key); // [11.6] 로그인 화면의 닉네임 입력을 처리한다.
    void Reset(); // [11.6] 이전 이동 기록과 공격 버튼 상태를 비운다.

private:
    GameClient* game = nullptr; // [11.6] 요청을 전달할 대상.
    unsigned char previousKeys = 0; // [11.6] 마지막으로 보낸 이동 방향 기록
    bool attackPressed = false; // [11.6] 공격 버튼을 누르고 있는지 기록
    char nameBuffer[MAX_NAME_LEN]{}; // [11.6] 입력할 닉네임.
    int nameLength = 0; // [11.6] 입력한 닉네임의 글자 수
};

/*
GameClient가 InputHandler를 보유
InputHandler는 요청을 전달할 GameClient 주소를 기억
*/
