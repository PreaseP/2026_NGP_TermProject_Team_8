#pragma once
#include "Block.h"
#include <vector>

class Wall
{
private:
    std::vector<Block> blocks;
    
public:
    Wall();

    void addBlock(const Block& block);
    void addBlock(float x, float y, float z, float size = 1.0f);

    void createHorizontalWall(float startX, float y, float z, int length, float blockSize);
    void createVerticalWall(float x, float y, float startZ, int length, float blockSize);

    size_t getBlockCount() const { return blocks.size(); }
    const Block& getBlock(size_t index) const { return blocks[index]; }

    void render() const;

    void clear() { blocks.clear(); }
};

