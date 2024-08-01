#pragma once

#include <unordered_map>
#include <vector>
#include <box2d/box2d.h>
#include <iostream>

class HashedPosition {
public:
    double searchInstance = 0;

    HashedPosition();
    virtual b2Vec2 getHashingPosition();
    virtual float getHashSize();
};

class HashResult {
public:
    std::vector<HashedPosition*> nearby;
    std::vector<float> distances;
};

class SpatialHash {
private:
    double searchInstance = 0;
    float cellSize;
    std::vector<std::vector<HashedPosition*>> cells;
    float worldWidth;
    float worldHeight;
    float halfWidth;
    float halfHeight;
    float invCellSize;

public:
    float nextCellSize;
    SpatialHash(float width, float height, float cellSize);

    void insert(HashedPosition* entity);
    int toKey(float x, float y); 
    void clear();

    void query(HashResult& result, float x, float y, float range, HashedPosition* ignore);
};