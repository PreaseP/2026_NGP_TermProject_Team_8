#include "Inventory.h"

bool Inventory::Add(ITEM_TYPE item)
{
    if (item == ITEM_NONE || itemCount >= MAX_ITEMS)
        return false;

    items[itemCount++] = item;
    return true;
}

bool Inventory::Remove(ITEM_TYPE item)
{
    for (int i = 0; i < itemCount; ++i) {
        if (items[i] == item) {
            for (int j = i; j < itemCount - 1; ++j)
                items[j] = items[j + 1];
            items[--itemCount] = ITEM_NONE;
            return true;
        }
    }
    return false;
}

bool Inventory::Has(ITEM_TYPE item) const
{
    for (int i = 0; i < itemCount; ++i) {
        if (items[i] == item)
            return true;
    }
    return false;
}

void Inventory::Clear()
{
    for (int i = 0; i < MAX_ITEMS; ++i)
        items[i] = ITEM_NONE;
    itemCount = 0;
}
