#include "InputHandler.h"

#include "GameClient.h" // [11.6] 현재 상태 확인과 로그인 요청에 사용한다.
#include <gl/freeglut.h>
#include <cstring>

namespace {
    unsigned char MovementBit(unsigned char key) {
        switch (key) {
        case 'w': case 'W': return KEY_FORWARD;
        case 's': case 'S': return KEY_BACK;
        case 'a': case 'A': return KEY_LEFT;
        case 'd': case 'D': return KEY_RIGHT;
        default: return 0;
        }
    }
}
// [11.6] 글자는 추가하고, Backspace는 삭제하고, Enter는 로그인 요청을 보낸다.
void InputHandler::OnKeyDown(unsigned char key)
{
    if (game == nullptr) return;
    if (game->GetState() == CLIENT_PLAYING) {
        moveKeys |= MovementBit(key);
        return;
    }

    // [11.12] 대기실에서 Esc를 누르면 퇴장 요청을 등록한다.
    if (game->GetState() == CLIENT_ROOM) {
        if (key != 27) return; // Esc
        C2S_LeaveRoom request{};
        request.type = C2S_LEAVE_ROOM;
        game->QueueRequest(&request, sizeof(request));
        return;
    }

    // [11.11] 로비에서는 Enter로 현재 선택한 방의 입장을 요청한다.
    if (game->GetState() == CLIENT_LOBBY) {
        if (key != '\r') return;
        const int roomIndex = game->GetSelectedRoomIndex();
        const RoomSummary* room = game->GetRoom(roomIndex);
        if (room == nullptr) return;

        C2S_JoinRoom request{};
        request.type = C2S_JOIN_ROOM;
        request.room_id = room->room_id; // [11.11] 서버가 보낸 실제 방 번호를 사용한다.
        game->QueueRequest(&request, sizeof(request));
        return;
    }
    if (game->GetState() != CLIENT_LOGIN) return;

    if (key == '\t') {
        FocusServerAddress(!serverAddressFocused);
        return;
    }
    if (key == 1 && serverAddressFocused) {
        serverAddressSelected = true;
        return;
    }
    if (key == '\r') { // Enter
        if (!game->SetServerAddress(serverAddress)) return;
        // 실패해도 닉네임을 유지한다. 고친 뒤 Enter를 누르면 다시 요청할 수 있다.
        if (!game->RequestLogin(nameBuffer)) return;
        // 요청 등록 성공은 로그인 성공이 아니다. 상태 변경은 서버 응답에서 한다.
        return;
    }

    if (serverAddressFocused) {
        if (key == '\b') {
            if (serverAddressSelected) serverAddressLength = 0;
            else if (serverAddressLength > 0) --serverAddressLength;
            serverAddress[serverAddressLength] = '\0';
            serverAddressSelected = false;
        }
        else if ((key >= '0' && key <= '9') || key == '.') {
            if (serverAddressSelected) {
                serverAddressLength = 0;
                serverAddressSelected = false;
            }
            if (serverAddressLength < static_cast<int>(sizeof(serverAddress)) - 1) {
                serverAddress[serverAddressLength++] = static_cast<char>(key);
                serverAddress[serverAddressLength] = '\0';
            }
        }
        return;
    }

    if (key == '\b') { // Backspace
        if (nameLength > 0) {
            --nameLength;
            nameBuffer[nameLength] = '\0';
        }
        return;
    }

    // 영문과 숫자만 받으며, 마지막 한 칸은 널 문자용으로 남긴다.
    if ((key >= 'A' && key <= 'Z') ||
        (key >= 'a' && key <= 'z') ||
        (key >= '0' && key <= '9')) {
        if (nameLength < MAX_NAME_LEN - 1) {
            nameBuffer[nameLength] = static_cast<char>(key);
            ++nameLength;
            nameBuffer[nameLength] = '\0';
        }
    }
}

void InputHandler::OnKeyUp(unsigned char key)
{
    if (game == nullptr || game->GetState() != CLIENT_PLAYING) return;
    moveKeys = static_cast<unsigned char>(moveKeys & ~MovementBit(key));
}

// [11.20] 왼쪽 버튼을 누르면 차징하고, 떼면 던지기를 요청한다.
void InputHandler::OnMouse(int button, int state)
{
    if (game == nullptr || game->GetState() != CLIENT_PLAYING || button != GLUT_LEFT_BUTTON) return;

    if (state == GLUT_DOWN) {
        if (attackPressed) return; // 이미 차징을 등록했으면 같은 요청을 보내지 않는다.

        C2S_ChargeStart request{};
        request.type = C2S_CHARGE_START;
        if (game->QueueRequest(&request, sizeof(request))) {
            attackPressed = true;
            chargeStartTime = std::chrono::steady_clock::now(); // [11.24] 조준점에 쓸 시작 시간
        }
        return;
    }

    if (state != GLUT_UP || !attackPressed) return;

    attackPressed = false; // 버튼을 뗐으므로 다음 클릭을 받을 수 있게 한다.
    C2S_Throw request{};
    request.type = C2S_THROW;
    game->QueueRequest(&request, sizeof(request));
}

bool InputHandler::TryGetChargeSeconds(float& seconds) const
{
    if (!attackPressed) return false;
    seconds = std::chrono::duration<float>(std::chrono::steady_clock::now() - chargeStartTime).count();
    return true;
}

void InputHandler::ProcessInput()
{
    if (game == nullptr || game->GetState() != CLIENT_PLAYING || moveKeys == previousKeys) return;
    C2S_Move request{};
    request.type = C2S_MOVE;
    request.move_keys = moveKeys;
    if (game->QueueRequest(&request, sizeof(request))) previousKeys = moveKeys;
}
void InputHandler::SetServerAddress(const char* address)
{
    std::size_t length = 0;
    while (length < sizeof(serverAddress) - 1 && address[length] != '\0') ++length;
    std::memmove(serverAddress, address, length);
    serverAddress[length] = '\0';
    serverAddressLength = static_cast<int>(length);
    serverAddressSelected = false;
}

// [11.6] 이전 입력이 다음까지 남지 않게 한다. 닉네임과 요청 대상은 유지한다.
void InputHandler::Reset()
{
    moveKeys = 0;
    previousKeys = 0;
    attackPressed = false;
}
