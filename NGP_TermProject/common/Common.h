#pragma once

constexpr int MAX_ITEMS = 2;
constexpr int INVALID_ID = -1;

enum ITEM_TYPE : unsigned char
{
    ITEM_NONE,
    ITEM_SHIELD,
    ITEM_POWER_UP
};
