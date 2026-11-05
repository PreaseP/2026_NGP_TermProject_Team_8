#pragma once

constexpr int MAX_ITEMS = 2;
constexpr int MAX_NAME_LEN = 16;
constexpr int BUF_SIZE = 256;
constexpr int MAX_HP = 100;
constexpr int INVALID_ID = -1;

enum ITEM_TYPE : unsigned char
{
    ITEM_NONE,
    ITEM_SHIELD,
    ITEM_POWER_UP
};

enum CHARACTER_TYPE : unsigned char
{
    CHARACTER_STEVE,
    CHARACTER_ALEX
};

enum ARM_STATE : unsigned char
{
    ARM_IDLE,
    ARM_CHARGING,
    ARM_THROWING
};
