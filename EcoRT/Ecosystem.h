#pragma once


#include <iostream>
#include <fstream>
#include <vector>
#include <random>
#include <chrono>
#include <filesystem>

#include <omp.h>
#include "torch/torch.h"
#include <SFML/Graphics.hpp>
#include <box2d/box2d.h>

#include "SpatialHash.h"
#include "Entity.h"
#include "Render.h"
#include "BorderData.h"
#include "SmoothNoise.h"
#include "MyRandom.h"
#include "WritingTracker.h"


const b2Vec2 world_size = b2Vec2(752, 423);
const int starting_entities = 100;

class GLFWCallbacks;

class Ecosystem {
private:
    TreeOfLife treeOfLife;
    // pressure test: Ensures a large number of entities and plants, prevents reproduction.
    bool pressureTest = false;
    bool shouldUseOpenMP = false;


    int lastBranchSize = -1;
    float seasonLength = 10000.0f;
    float plantDensityPer1sq = 0.0009f; // plant density per square pixel

    bool wantsToReloadShaders = false;

    float offsetX = 0;
    float offsetY = 0;
    float scaleStep = 0;
    int total_steps = 0;
    float timeStep = 1.0f / 60.0f;
    int velIter = 2;
    int posIter = 2;
    float speedUpRate = 10;
    int targetFps = 60;
    int entityUpdateRate = 4;
    float skillFactor = 1.0;
    int sleepingPlants = 0;

    bool showWorldPanel = false;
    bool isDragging = false;
    b2Vec2 dragStart;
    b2Vec2 originalWorldCoords;
    b2Vec2 originalOffset;
    SpatialHash spatialHash = SpatialHash(world_size.x, world_size.y, 25);
    std::vector<TextData> textData;
    glm::vec4 screenDataFullscreen = glm::vec4(50, 50, 800, 800); // used for storing window data
    bool isFullscreen = false;

    bool shouldRun = true;
    int randomAccesses = 0;

    BRandom rndEngine;
    SmoothNoise smoothNoise;

    Render renderer = Render(world_size);
    void addEdgeToBorder(b2EdgeShape& edge);
    void syncEntitiesBox2d();
    void removeEntities();
    EnvironmentBody* getClosestTo(float x, float y, float range, EnvironmentBody* ignore);
    void updateText();
    std::string getPrecisionString(double number, int precision);
    b2Vec2 calculatePlantSpawnPoint();
    int calculateAmountOfPlants();

public:
    int worldSeed;
    int livingEntities = 0;

    std::vector<EnvironmentBody*> allEntities;
    std::vector<Entity*> entitiesList;
    std::vector<PlantEntity*> plantsList;
    EnvironmentBody* focusedEntity = nullptr;
    double totalStep = 0; // how many world.Step() increments have been taken. MUST be a double due to float limits 2^19
    bool shouldSave = false;
    std::string worldFilePath = "saves";

    b2World world = b2World(b2Vec2(0.0f, 0.0f));
    Border border;
    Ecosystem();
    void run();
    void updateEntities();
    void createBorder();
    double random();
    b2Vec2 pixelToWorldCoordinates(float x, float y);
    b2Vec2 worldCoordinatesToPixel(float x, float y);
    float getScale();
    void monitorEcosystem();
    void addEntity(EnvironmentBody* body);
    void removeEntity(EnvironmentBody* body);
    void saveToFile();
    bool loadFromFile();
    b2Vec2 getRandomSpawnPoint();
    bool isInWorldBounds(float x, float y);
    bool isInBorderBounds(float x, float y);
    void setFocusedEntity(EnvironmentBody* entity);
    void updateGui();
    float getSeasonTime();
    float getSeasonalGrowthFactor();
    std::string getSeasonName();
    void writeTrunk(ByteWriter& outputFile, TreeBranch* branch);
    void readTrunk(std::ifstream& inputFile, std::map<int, TreeBranch*>& allBranches, TreeBranch* branch);
    void setFullscreen(bool enable);


    void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods);
    void cursor_position_callback(GLFWwindow* window, double xpos, double ypos);
    void framebuffer_size_callback(GLFWwindow* window, int width, int height);
    void mouse_button_callback(GLFWwindow* window, int button, int action, int mods);
    void scroll_callback(GLFWwindow* window, double xoffset, double yoffset);
    void close_window_callback(GLFWwindow* window);
};

class GLFWCallbacks {
public:

    static void close_window_callback(GLFWwindow* window);
    static void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods);
    static void cursor_position_callback(GLFWwindow* window, double xpos, double ypos);
    static void framebuffer_size_callback(GLFWwindow* window, int width, int height);
    static void mouse_button_callback(GLFWwindow* window, int button, int action, int mods);
    static void scroll_callback(GLFWwindow* window, double xoffset, double yoffset);
    static void glfw_error_callback(int error, const char* description);
};