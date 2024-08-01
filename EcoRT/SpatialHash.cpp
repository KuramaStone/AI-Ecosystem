#include "SpatialHash.h"

HashedPosition::HashedPosition() {

}

b2Vec2 HashedPosition::getHashingPosition() {
    return b2Vec2(0, 0);
}

float HashedPosition::getHashSize() {
    return 0;
}

SpatialHash::SpatialHash(float width, float height, float cellSize) : cellSize(cellSize), worldWidth(width), worldHeight(height) {
    // Initialize cells based on screen dimensions and cell size
    int numCols = ceil(worldWidth / cellSize);
    int numRows = ceil(worldHeight / cellSize);
    cells.resize(numCols * numRows, std::vector<HashedPosition*>(numRows, nullptr));
    nextCellSize = cellSize;
    cellSize = cellSize;


    invCellSize = 1.0 / cellSize;
    halfWidth = worldWidth / 2;
    halfHeight = worldHeight / 2;
}

int SpatialHash::toKey(float x, float y) {

    int x1 = int((x + halfWidth) * invCellSize);
    int y1 = int((y + halfHeight) * invCellSize);

    return x1 + y1 * (worldWidth * invCellSize);
}

// inserts entity into hash. A size of zero forces a single hash box each. Larger sizes adds to nearby boxes.
void SpatialHash::insert(HashedPosition* entity) {
    // Calculate the cell coordinates based on the entity's position
    b2Vec2 hashedPosition = entity->getHashingPosition();
    float x = hashedPosition.x;
    float y = hashedPosition.y;


    if (x < -worldWidth / 2) {
        x = -worldWidth / 2;
    }
    if (y < -worldHeight / 2) {
        y = -worldHeight / 2;
    }
    if (x >= worldWidth / 2 - 0.001) {
        x = worldWidth / 2 - 1;
    }
    if (y >= worldHeight / 2 - 0.001) {
        y = worldHeight / 2 - 1;
    }

    if (x >= worldWidth / 2 || y >= worldHeight / 2 || x < -worldWidth / 2 || y < -worldHeight / 2) {
        std::cerr << "X: " << x << ", Y: " << y << std::endl;
        throw std::runtime_error("Can't insert to hash due to entity's OOB x/y.");
    }

    // Store the entity in the corresponding cell
    float hashSize = entity->getHashSize();
    
    if (hashSize <= 0) {
        int cellKey = toKey(x, y);
        if (cellKey > cells.size() || cellKey < 0) {
            std::cerr << "X: " << x << ", Y: " << y << ", CellKey: " << cellKey << std::endl;
            throw std::runtime_error("Cellkey is out of range.");
        }



        //std::cout << "key: " << cellKey << " " << x << "," << y << std::endl;
        cells[cellKey].push_back(entity);
    }
    else {
        // if size isn't zero, add to all nearby cells as well.

        float ceilRange = ceil(hashSize * invCellSize);

        // Retrieve the entities from the nearby cells
        for (int x1 = -ceilRange; x1 <= ceilRange; x1++) {
            for (int y1 = -ceilRange; y1 <= ceilRange; y1++) {
                float wx = x1 * cellSize + x;
                float wy = y1 * cellSize + y;

                if (wx < -worldWidth / 2 || wy < -worldHeight / 2 || wx >= worldWidth / 2 - 0.001 || wy >= worldHeight / 2 - 0.001) {
                    // outside of range, just ignore.
                    continue;
                }
                int cellKey = toKey(wx, wy);

                cells[cellKey].push_back(entity);
            }
        }
    }
}

void SpatialHash::clear() {

    int numCols = ceil(worldWidth / cellSize);
    int numRows = ceil(worldHeight / cellSize);
    for (int x1 = 0; x1 < numCols; x1++) {
        for (int y1 = 0; y1 < numRows; y1++) {
            std::vector<HashedPosition*>& vec = cells[x1 + y1 * numCols];
            vec.clear();
        }
    }

    if (nextCellSize != cellSize) {
        cellSize = nextCellSize;
        numCols = ceil(worldWidth / cellSize);
        numRows = ceil(worldHeight / cellSize);
        cells.resize(numCols * numRows, std::vector<HashedPosition*>(numRows, nullptr));
    }
}

// gets nearby entities
void SpatialHash::query(HashResult& result, float x, float y, float range, HashedPosition* ignore) {
    searchInstance++;

    float ceilRange = ceil(range * invCellSize);
    float range2 = range * range;

    // Retrieve the entities from the nearby cells
    result.nearby.clear();
    result.distances.clear();

    int x1;
    int y1;
    float wx;
    float wy;
    int cellKey;
    std::vector<HashedPosition*>& inCell = cells[0];
    float dx;
    float dy;
    float dist2;


    for (x1 = -ceilRange; x1 <= ceilRange; ++x1) {
        for (y1 = -ceilRange; y1 <= ceilRange; ++y1) {
            wx = x1 * cellSize + x;
            wy = y1 * cellSize + y;
            if (wx < -worldWidth / 2 || wy < -worldHeight / 2 || wx >= worldWidth / 2 - 0.001 || wy >= worldHeight/2 - 0.001) {
                // outside of range, just ignore.
                continue;
            }

            cellKey = toKey(wx, wy);

            inCell = cells[cellKey];

            for (HashedPosition* entity : inCell) {
                // ignore entities that have the same search instance.
                // same search instance means it has already been added to the list.
                if (entity != nullptr && entity->searchInstance != searchInstance && entity != ignore) {
                    b2Vec2 epos = entity->getHashingPosition();
                    dx = epos.x - x;
                    dy = epos.y - y;
                    dist2 = (dx*dx+dy*dy);

                    if (dist2 <= range2) {
                        entity->searchInstance = searchInstance;
                        result.nearby.push_back(entity);
                        result.distances.push_back(sqrt(dist2));
                    }
                }
            }

        }
    }

}