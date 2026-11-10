#pragma once
#include "Block.h"
#include <vector>

class Ground
{
private:
    std::vector<std::vector<Block>> blocks;
    int width, depth;

public:
    Ground(int width, int depth, float blockSize = 1.0f);
    
    int getWidth() const;
    int getDepth() const;
    Block& getBlock(int x, int z);
    const Block& getBlock(int x, int z) const;
    
    void initializeBlocks(float startX, float startY, float startZ, float blockSize);
    
    void render() const;
};


