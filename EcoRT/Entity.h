#pragma once

#include <iostream>
#include <box2d/box2d.h>
#include <box2d/b2_math.h>
#include <vector>
#include <math.h>
#include "Eigen/Dense"
#include "Genetics.h"
#include <random>
#include "Stomach.h"
#include "MyRandom.h"
#include "SpatialHash.h"
#include "BorderData.h"
#include "BMath.h"

const uint16_t WALL_BITS = 0x0001;
const uint16_t CELL_BITS = 0x0002;
const uint16_t SPAWN_CIRCLE_BITS = 0x0004;

class EnvironmentBody : public HashedPosition {
public:
    Border* borderData = nullptr;
    BRandom rndEngine;
    b2World* world;
    b2Body* body;
    b2Vec2 position;
    b2Vec2 screenPosition;
    b2Vec3 displayColor = b2Vec3(1.0f, 0.0f, 1.0f);
    HashResult hashResult;
    std::vector<EnvironmentBody*> addToWorld = std::vector<EnvironmentBody*>();
    b2Vec2 gradualVelocity = b2Vec2(0.0, 0.0); // slowly updates to match current velocity
    b2Vec2 lastForceUsed = b2Vec2(0.0, 0.0);
    bool focused = false;

    bool allLiveForever = false;
    bool disableMutations = false;
    bool requestsRemoval = false;
    bool isAlive = true;
    std::string origin = "none";
    int uid = -1;

    int generation = 0;
    int age = 0;
    float attackedAmount = 0.0f;
    float lastAttackAngle = 0.0f;
    float proteinLevels = 4.0f;
    float sugarLevels = 4.0f;
    float angle = 0.0f;
    bool hasBody = false;
    bool isPlant = false;
    float radius = 1;
    float sightRange = 64;
    b2Vec3 scentValue = b2Vec3(0.0f, 0.0f, 0.0f);


    void syncBox2d();
    virtual b2Vec2 getHashingPosition();
    virtual void update();
    virtual void preupdate();
    virtual void postupdate();
    virtual void buildBody();
    virtual void destroy();
    void destroyBody();
    float distanceTo(EnvironmentBody* entity);
    float distanceTo(float x, float y);
    bool isInFrontOf(EnvironmentBody* entity);
    static EnvironmentBody* getClosestIntersectingEntity(b2Vec2& origin, float angle, float length, HashResult& result);
    static bool doesIntersect(b2Vec2& origin, float angle, float length, EnvironmentBody* circle);

};

class PlantEntity : public EnvironmentBody {
public:

    PlantEntity(b2World& world, float x, float y);
    virtual void update();
    virtual void preupdate();
    virtual void postupdate();
    virtual void buildBody();
    virtual void destroy();
    void applyForces();
};


class Entity : public EnvironmentBody {
private:
    float geneSpeedModifier = 1.0f;
    float geneMaxSize = 1.0f;
    float geneAsexuality = 0.5f;
    int ticksSinceDeath = 0; 

    void useResources();
    void growth();
    void aggression();
    void digestion();
    void processes();
    void updateBrain();
    void useBrain();
    void reproduction();
    float getAgeMod();
    Eigen::VectorXf getReceptorInputs();
    Eigen::VectorXf getWallSightInputs();
public:
    CellBrain brain;
    Genetics genetics;
    int branchIndex = -1;
    int parentBranch = -1;
    std::string speciesName = "unknown";
    bool isOnTreeOfLife = false;

    float growthRadius = 0.0f;
    float attackValue = 0.0f;
    float bindValue = 0.0f;
    float holdValue = 0.0f;
    float attackingAmount = 0.0f;
    float reproductionValue = 0.0f;
    float willToGrow = 0.0f;
    std::vector<Entity*> bindedTo;
    Eigen::VectorXf inputVector = Eigen::VectorXf::Zero(50);
    Eigen::VectorXf outputs = Eigen::VectorXf::Zero(4);
    Stomach stomach;
    bool initDeath = false;

    float proteinIncome = 0.0f;
    float sugarIncome = 0.0f;
    float proteinExpense = 0.0f;
    float sugarExpense = 0.0f;
    float storedProtein = 0.0f;
    float storedSugar = 0.0f;
  

    Entity(b2World& world, float x, float y);
    virtual void preupdate();
    virtual void update();
    virtual void postupdate();
    virtual void buildBody();
    virtual void destroy();

    Eigen::VectorXf getNearbyEntityInputs();
    b2Vec3 getAverageNearbyScents();
    bool isBindedTo(Entity* entity);
    void mutate();
    Entity* createChild(float x, float y);
    void updateGenetics();
    void death();
};