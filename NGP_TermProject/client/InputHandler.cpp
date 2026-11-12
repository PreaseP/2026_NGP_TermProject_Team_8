#include "InputHandler.h"

#include "GameClient.h" // [11.6] 현재 상태 확인과 로그인 요청에 사용한다.

// [11.6] 글자는 추가하고, Backspace는 삭제하고, Enter는 로그인 요청을 보낸다.
void InputHandler::OnKeyDown(unsigned char key)
{
    if (game == nullptr) return;

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

    if (key == '\r') { // Enter
        // 실패해도 닉네임을 유지한다. 고친 뒤 Enter를 누르면 다시 요청할 수 있다.
        if (!game->RequestLogin(nameBuffer)) return;
        // 요청 등록 성공은 로그인 성공이 아니다. 상태 변경은 서버 응답에서 한다.
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

// [11.6] 이전 입력이 다음까지 남지 않게 한다. 닉네임과 요청 대상은 유지한다.
void InputHandler::Reset()
{
    previousKeys = 0;
    attackPressed = false;
}
