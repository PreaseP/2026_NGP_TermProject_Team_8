#pragma once
#include <unordered_set>

class KeyManager {
private:
    std::unordered_set<int> downKeys;
    std::unordered_set<int> downSpecial;

public:
    KeyManager() = default;

    void pressKey(int key) { downKeys.insert(key); }
    void releaseKey(int key) { downKeys.erase(key); }
    bool isKeyDown(int key) const { return downKeys.find(key) != downKeys.end(); }

    void pressSpecial(int key) { downSpecial.insert(key); }
    void releaseSpecial(int key) { downSpecial.erase(key); }
    bool isSpecialDown(int key) const { return downSpecial.find(key) != downSpecial.end(); }

    void clearAll() { downKeys.clear(); downSpecial.clear(); }
};

