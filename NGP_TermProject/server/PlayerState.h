#pragma once
#include "../common/Common.h"
#include <windows.h>
#include <glm/glm.hpp>

class PlayerState {
public:
    CHARACTER_TYPE character = CHAR_NONE;
    glm::vec3      pos = glm::vec3(0.0f);
    int            hp = MAX_HP;
    ARM_STATE      armState = ARM_IDLE;
    bool           charging = false;
    ULONGLONG      chargeStartMs = 0;
    ULONGLONG      throwEndMs = 0;
    bool           shield = false;
    bool           powerUp = false;
    ITEM_TYPE      items[MAX_ITEMS] = {};
    int            itemCount = 0;
    unsigned char  moveKeys = 0;

    void Reset(CHARACTER_TYPE character, const glm::vec3& spawn);
    void ClearInput();
};
