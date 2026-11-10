#include "Snowball.h"

void Snowball::Launch(int id, unsigned char owner, bool isPowered, const glm::vec3& startPos, const glm::vec3& startVel)
{
    active = true;
    objectId = id;
    ownerSlot = owner;
    powered = isPowered;
    pos = startPos;
    vel = startVel;
    lifeTime = 0.0f;
}

void Snowball::Deactivate()
{
    active = false;
    objectId = INVALID_ID;
    powered = false;
    vel = glm::vec3(0.0f);
    lifeTime = 0.0f;
}
