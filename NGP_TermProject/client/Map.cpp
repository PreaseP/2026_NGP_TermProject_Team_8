#include "Map.h"
#include <gl/glew.h>

Map::Map() : frontGround(MAP_WIDTH, 5, BLOCK_SIZE), backGround(MAP_WIDTH, 5, BLOCK_SIZE)
{

}

void Map::initialize()
{
    frontGround.initializeBlocks(0.0f, 0.0f, 0.0f, BLOCK_SIZE);
    
    backGround.initializeBlocks(0.0f, 0.0f, 10.0f * BLOCK_SIZE, BLOCK_SIZE);
    
    surroundingWall.clear();
    
    const int WALL_HEIGHT = 5;
    
    for (int height = 0; height < WALL_HEIGHT; ++height) {
        float wallY = (height + 1) * BLOCK_SIZE;
        
        surroundingWall.createHorizontalWall(-BLOCK_SIZE, wallY, -BLOCK_SIZE, MAP_WIDTH + 2, BLOCK_SIZE);
        
        surroundingWall.createHorizontalWall(-BLOCK_SIZE, wallY, MAP_DEPTH * BLOCK_SIZE, MAP_WIDTH + 2, BLOCK_SIZE);
        
        surroundingWall.createVerticalWall(-BLOCK_SIZE, wallY, 0.0f, MAP_DEPTH, BLOCK_SIZE);
        
        surroundingWall.createVerticalWall(MAP_WIDTH * BLOCK_SIZE, wallY, 0.0f, MAP_DEPTH, BLOCK_SIZE);
    }
}

void Map::render() const
{
    frontGround.render();
    backGround.render(); 
    surroundingWall.render();
}
