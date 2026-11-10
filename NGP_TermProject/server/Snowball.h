#pragma once
#include "../common/Common.h"
#include <glm/glm.hpp>

class Snowball {
public:
    bool          active = false;
    int           objectId = INVALID_ID;
    unsigned char ownerSlot = 0;
    bool          powered = false;
    glm::vec3     pos = glm::vec3(0.0f);
    glm::vec3     vel = glm::vec3(0.0f);
    float         lifeTime = 0.0f;

    void Launch(int id, unsigned char owner, bool powered, const glm::vec3& pos, const glm::vec3& vel);
    void Deactivate();
};
