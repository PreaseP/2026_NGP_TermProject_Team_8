#pragma once
#include "Ground.h"
#include "Wall.h"

constexpr int MAP_WIDTH = 10;
constexpr int MAP_DEPTH = 15;
constexpr float BLOCK_SIZE = 1.0f;
class Map
{
private:
    Ground frontGround;
    Ground backGround;
    Wall surroundingWall;
    
public:
    Map();
    
    void initialize();
    
    Ground& getFrontGround() { return frontGround; }
    Ground& getBackGround() { return backGround; }
    Wall& getWall() { return surroundingWall; }
    
    const Ground& getFrontGround() const { return frontGround; }
    const Ground& getBackGround() const { return backGround; }
    const Wall& getWall() const { return surroundingWall; }
    
    void render() const;
};

