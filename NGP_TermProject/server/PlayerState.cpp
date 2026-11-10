#include "PlayerState.h"

void PlayerState::Reset(CHARACTER_TYPE characterType, const glm::vec3& spawn)
{
    character = characterType;
    pos = spawn;
    hp = MAX_HP;
    armState = ARM_IDLE;
    shield = false;
    powerUp = false;
    itemCount = 0;
    ClearInput();
}

void PlayerState::ClearInput()
{
    moveKeys = 0;
    charging = false;
    chargeStartMs = 0;
    throwEndMs = 0;
}
