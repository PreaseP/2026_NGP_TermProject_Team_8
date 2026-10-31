#pragma once
#include "Common.h"

class Inventory {
public:
    ITEM_TYPE items[MAX_ITEMS] = {};
    int       itemCount = 0;

    bool Add(ITEM_TYPE item);
    bool Remove(ITEM_TYPE item);
    bool Has(ITEM_TYPE item) const;
    void Clear();
};
