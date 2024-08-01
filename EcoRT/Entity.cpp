#include "Entity.h"

void EnvironmentBody::syncBox2d() {


    if (!hasBody) {
        throw std::runtime_error("Entity has no body.");
    }

    b2Vec2 oldPos = position;
    position = body->GetPosition();
    angle = MyMath::normalizeAngle(body->GetAngle());

    if (!position.IsValid()) {
        std::cerr << "Position:" << position.x << "," << position.y << std::endl;
        std::cerr << "Age:" << age << std::endl;
        std::cerr << "Living:" << isAlive << std::endl;
        std::cerr << "Origin:" << origin << std::endl;
        std::cerr << "Isplant:" << isPlant << std::endl;
        std::cerr << "oldPos:" << oldPos.x << "," << oldPos.y << std::endl;
        std::cerr << "angle:" << angle << std::endl;
        b2Vec2 vel = body->GetLinearVelocity();
        std::cerr << "vel:" << vel.x << ", " << vel.y << std::endl;
        throw std::runtime_error("Position isn't valid.");

        // set back to previous
        //position = oldPos;
        //body->SetTransform(position, angle);
    }

    // update gradualVelocity
    b2Vec2 velocity = body->GetLinearVelocity();
    // slowly speeds up, quickly stops.
    float catchUpSpeed = (velocity.Length() - gradualVelocity.Length()) > 0 ? 0.005 : 0.005;
    gradualVelocity.x = velocity.x * catchUpSpeed + gradualVelocity.x * (1 - catchUpSpeed);
    gradualVelocity.y = velocity.y * catchUpSpeed + gradualVelocity.y * (1 - catchUpSpeed);

}

b2Vec2 EnvironmentBody::getHashingPosition() {
    return position;
}

// stuff before the main update. single thread
void EnvironmentBody::preupdate() {
}

// the main update. multithreaded, so avoid certain box2d functions
void EnvironmentBody::update() {
}

// stuff after the main update. single thread
void EnvironmentBody::postupdate() {
}

void EnvironmentBody::buildBody() {
}

float EnvironmentBody::distanceTo(EnvironmentBody* entity) {
    return MyMath::distance(position, entity->position);
}
float EnvironmentBody::distanceTo(float x, float y) {
    return MyMath::distance(position, b2Vec2(x, y));
}

void EnvironmentBody::destroyBody() {
    world->DestroyBody(body);
    hasBody = false;
    body = nullptr;
}

void EnvironmentBody::destroy() {
    destroyBody();
    hashResult.nearby.clear();
    hashResult.distances.clear();
    addToWorld.clear();
}

bool EnvironmentBody::isInFrontOf(EnvironmentBody* entity) {

    float angleTo = MyMath::approxAtan2(entity->position.y - position.y, entity->position.x - position.x);
    float dist = distanceTo(entity) - radius - entity->radius;

    // slightly decrease distance to make biting easier
    if (fabs(dist) < 0.1 && fabs(angleTo - angle) < (M_PI * 2 * 0.1)) {
        return true;
    }

    return false;
}

PlantEntity::PlantEntity(b2World& bworld, float x, float y) {
    uid = rand();
    world = &bworld;
    isPlant = true;
    sightRange = 16;
    position.x = x;
    position.y = y;

    displayColor.x = 0.3;
    displayColor.y = 0.8;
    displayColor.z = 0.05;
    scentValue.x = 0.3;
    scentValue.y = 0.8;
    scentValue.z = 0.05;

    proteinLevels = 0.01f;
    sugarLevels = 0.01f;
}

void PlantEntity::destroy() {
    EnvironmentBody::destroy();
}

void PlantEntity::buildBody() {
    hasBody = true;
    b2CircleShape shape = b2CircleShape();
    shape.m_radius = radius;
    b2BodyDef def = b2BodyDef();
    def.position.Set(position.x, position.y);
    def.angle = angle;
    def.type = b2_dynamicBody;
    def.linearDamping = 4.0f;
    def.angularDamping = 0.8f;
    body = world->CreateBody(&def);
    body->SetBullet(false);

    b2Filter filter;
    filter.categoryBits = CELL_BITS;
    filter.maskBits = CELL_BITS | WALL_BITS;

    b2FixtureDef fixtureDef = b2FixtureDef();
    fixtureDef.shape = &shape;
    fixtureDef.density = 3.0f;
    fixtureDef.friction = 0.5f;
    fixtureDef.filter = filter;
    body->CreateFixture(&fixtureDef);
}

void PlantEntity::applyForces() {

    // rotate slowly
    float turnForce = 10 * (uid % 2 == 0 ? 1 : -1);
    //body->ApplyTorque(turnForce, true);

    int nearbyPlants = 0;
    for (HashedPosition* hent : hashResult.nearby) {
        EnvironmentBody* ent = static_cast<EnvironmentBody*>(hent);
        if (ent->isPlant) {
            nearbyPlants++;
        }
    }
    lastForceUsed = b2Vec2(0, 0);
    // apply force to each plant. If facing, attract. if not, avoid.
    for (HashedPosition* hentity : hashResult.nearby) {
        EnvironmentBody* entity = static_cast<EnvironmentBody*>(hentity);
        if (entity->isPlant && entity != this) {
            // apply force
            float dist = distanceTo(entity);
            if (dist > 0) {

                b2Vec2 bodyAPosition = entity->position;
                b2Vec2 bodyBPosition = position;
                b2Vec2 forceDirection = (bodyAPosition - bodyBPosition);
                forceDirection.Normalize();
                b2Vec2 force = forceDirection;

                float relSize = entity->radius / radius;
                // value between -1 and 1. 1 for facing each other, -1 for facing away.
                float angleFactor = -1; //cos(MyMath::normalizeAngle(angle - entity->angle + M_PI));
                float r2 = forceDirection.Length() * forceDirection.Length();

                float forceMagnitude = 75.0f * relSize;
                force.x *= (1.0f / r2) * forceMagnitude * angleFactor;
                force.y *= (1.0f / r2) * forceMagnitude * angleFactor;

                float kTorqueConstant = 10.0f;
                float torque = -kTorqueConstant * MyMath::normalizeAngle(angle - MyMath::approxAtan2(forceDirection.y, forceDirection.x));

                lastForceUsed += force;
                body->ApplyForceToCenter(force, true);
                body->ApplyTorque(torque, true);
            }
        }
    }
}

void PlantEntity::preupdate() {
}

void PlantEntity::update() {



    if (proteinLevels <= 0 && sugarLevels <= 0) {
        // die; something probably ate it
        requestsRemoval = true;
        isAlive = false;
        return;
    }

    // decay if in mountain 
    int noiseIndex = borderData->caveWalls->toStepIndex(position.x, position.y);
    if (borderData->caveWalls->originalTextureData[noiseIndex] > borderData->caveWalls->threshold) {
        proteinLevels = std::max(proteinLevels - 0.01f, 0.0f);
        sugarLevels = std::max(sugarLevels - 0.01f, 0.0f);
    }

    bool isEntityNear = false;

    for (HashedPosition* hentity : hashResult.nearby) {
        EnvironmentBody* entity = static_cast<EnvironmentBody*>(hentity);
        if (!entity->isPlant) {
            isEntityNear = true;
            break;
        }
    }


    if (isEntityNear) {
        // encourage to plant to sleep for most of its life after growing
        applyForces();
    }

    // decreases proportionally to the amount of nearby plants and how close those plants are.
    float nearbyValue = 1.0; // 1 for self
    for (HashedPosition* hentity : hashResult.nearby) {
        EnvironmentBody* entity = static_cast<EnvironmentBody*>(hentity);
        if(entity->isPlant) {
            float distance = distanceTo(entity);
            nearbyValue += 1.0 - distance / sightRange;
        }
    }
    float nearbyDebuff = 1.0 / (nearbyValue);

    // plants grow overtime decreased by nearby plants to prevent infinite growth
    // add a little random factor to make sure they grow at different rates
    sugarLevels += 0.15f * nearbyDebuff * (1.0 + (rndEngine.randomFloat() * 0.3 - 0.15));
    proteinLevels += 0.15f * nearbyDebuff * (1.0 + (rndEngine.randomFloat() * 0.3 - 0.15));

    age++;
}
void PlantEntity::postupdate() {

    int nearbyPlants = 0;
    for (HashedPosition* hent : hashResult.nearby) {
        EnvironmentBody* ent = static_cast<EnvironmentBody*>(hent);
        if (ent->isPlant) {
            nearbyPlants++;
        }
    }

    if (radius < 4) {
        // calculate radius based on "area" of two resources
        // reach size 3 at 1000~ sum
        float newRadius = std::max(1.0, sqrt((sugarLevels + proteinLevels) * 0.025) * 0.6);

        // update whenever it gets 0.X units bigger
        if (abs(newRadius - radius) >= 0.1) {
            radius = newRadius;
            destroyBody();
            buildBody();
        }
    }

    if (radius >= 3 && !allLiveForever) {
        // randomly split after reaching max size
        float chanceToSplit = 0.001;
        if (rndEngine.randomFloat() < chanceToSplit) {

            float rnd = rndEngine.randomFloat();
            // unstable replication, all populations eventually die out.
            if (rnd < 0.2) {
                float childX = position.x + (rndEngine.randomFloat() * 2 - 1) * radius * 0.9;
                float childY = position.y + (rndEngine.randomFloat() * 2 - 1) * radius * 0.9;
                PlantEntity* child = new PlantEntity(*world, childX, childY);
                child->generation = generation + 1;
                child->origin = "child";
                child->proteinLevels = proteinLevels / 20;
                child->sugarLevels = sugarLevels / 20;
                addToWorld.push_back(child);

                childX = position.x + (rndEngine.randomFloat() * 2 - 1) * radius * 0.9;
                childY = position.y + (rndEngine.randomFloat() * 2 - 1) * radius * 0.9;
                child = new PlantEntity(*world, childX, childY);
                child->generation = generation + 1;
                child->origin = "child";
                child->proteinLevels = proteinLevels / 20;
                child->sugarLevels = sugarLevels / 20;
                addToWorld.push_back(child);
            }
            else if (rnd < 0.7) {
                float childX = position.x + (rndEngine.randomFloat() * 2 - 1) * radius * 0.9;
                float childY = position.y + (rndEngine.randomFloat() * 2 - 1) * radius * 0.9;
                PlantEntity* child = new PlantEntity(*world, childX, childY);
                child->generation = generation + 1;
                child->origin = "child";
                child->proteinLevels = proteinLevels / 20;
                child->sugarLevels = sugarLevels / 20;
                addToWorld.push_back(child);
            }
            else {
                // no replacement
            }

            // kill this one
            requestsRemoval = true;
            isAlive = false;
        }
    }
}


/// <summary>
/// ///
/// </summary>
/// <param name="world"></param>

Entity::Entity(b2World& bworld, float x, float y) {
    uid = rand();
    world = &bworld;
    sightRange = 32;
    radius = 1;

    position.x = x;
    position.y = y;

    displayColor.x = 1;
    displayColor.y = 0;
    displayColor.z = 1;
    updateGenetics();
}
void Entity::buildBody() {

    hasBody = true;
    b2CircleShape shape = b2CircleShape();
    shape.m_radius = radius;
    b2BodyDef def = b2BodyDef();
    def.position.Set(position.x, position.y);
    def.angle = angle;
    def.type = b2_dynamicBody;
    def.linearDamping = 40.0f;
    def.angularDamping = 40.0f;
    body = world->CreateBody(&def);
    body->SetBullet(false);

    b2Filter filter;
    filter.categoryBits = CELL_BITS;
    filter.maskBits = CELL_BITS | WALL_BITS;

    b2FixtureDef fixtureDef =b2FixtureDef();
    fixtureDef.shape = &shape;
    fixtureDef.density = 0.1f;
    fixtureDef.friction = 1.0f;
    fixtureDef.filter = filter;
    body->CreateFixture(&fixtureDef);
}

void Entity::preupdate() {
    if (sugarLevels <= 0) {
        isAlive = false;
    }

    proteinIncome = 0;
    sugarIncome = 0;
    sugarExpense = 0;
    proteinExpense = 0;

    if (!isAlive) {
        return;
    }

    // calculate aggression here to use calculated value as input
    aggression();
}
void Entity::postupdate() {

    if (!isAlive) {
        return;
    }

    // these arent thread safe
    growth();
    reproduction();


    storedSugar += sugarExpense * 0.2;
    storedProtein += proteinExpense * 0.5;

    if (proteinIncome < 0) {
        std::cerr << "Protein income is negative. Critical error." << std::endl;
    }
    if (sugarIncome < 0) {
        std::cerr << "Sugar income is negative. Critical error." << std::endl;
    }
    if (proteinExpense < 0) {
        std::cerr << "protein expense is negative. Critical error." << std::endl;
    }
    if (sugarExpense < 0) {
        std::cerr << "Sugar expense is negative. Critical error." << std::endl;
    }

    // set attackedAmount to zero to calculate for next update
    attackedAmount = 0;
}

void Entity::destroy() {
    EnvironmentBody::destroy();

    bindedTo.clear();
}

void Entity::update() {

    if (!isAlive) {

        if (!initDeath) {
            death();
        }


        // protein is converted to sugar until protein reaches zero at which point it collapses.
        // decay exponentially faster (slow start, but becomes inevitable)
        float decay = 0.001f * powf(1.0003f, 1 + float(ticksSinceDeath) / 1.1);
        proteinLevels -= decay;
        sugarLevels += decay;

        if (proteinLevels <= 0) {
            proteinLevels = 0;
            requestsRemoval = true;
        }

        ticksSinceDeath++;
        return;
    }

    processes();
    age++;
}

void Entity::updateGenetics() {
    displayColor.x = genetics.getValue("scentA");
    displayColor.y = genetics.getValue("scentB");
    displayColor.z = genetics.getValue("scentC");

    scentValue.x = genetics.getValue("scentA");
    scentValue.y = genetics.getValue("scentB");
    scentValue.z = genetics.getValue("scentC");

    geneMaxSize = genetics.getValue("maxSize");
    geneSpeedModifier = genetics.getValue("geneSpeedModifier");

    stomach.acidity = genetics.getValue("digestionAcidity");
    stomach.maxSize = 100;
    growthRadius = radius; // growth radius should 

    geneAsexuality = genetics.getValue("asexuality");

    brain.build();

    if (!isAlive) {
        displayColor.x = 0.3;
        displayColor.y = 0.3;
        displayColor.z = 0.3;

        scentValue.x = 0.3;
        scentValue.y = 0.3;
        scentValue.z = 0.3;
    }

}

void Entity::processes() {
    // calculate nearest entities

    // use outputs
    updateBrain();
    useBrain();
    
    // digestion
    digestion();
    useResources();
    
    
    // bindings

}

void Entity::reproduction() {
    if (allLiveForever) {
        return; // dont breed
    }


    if (reproductionValue > 0.0) {


        float proteinForChild = 5.0f;
        float sugarForChild = 5.0f;
        float myProteinUsage = proteinForChild;
        float mySugarUsage = sugarForChild;
        float otherProteinUsage = 0.0;
        float otherSugarUsage = 0.0;
        float perc = 0.0f;
        bool allowCrossover = false;
        Entity* other = nullptr;

        // chance to reproduce with others based on gene.
        if (rndEngine.randomFloat() >= geneAsexuality) {

            // if both want to reproduce, each can contribute a portion to help
            float closestDist = sightRange;
            for (HashedPosition* heb : hashResult.nearby) {
                EnvironmentBody* eb = static_cast<EnvironmentBody*>(heb);
                if (!eb->isPlant) {
                    float distance = distanceTo(eb);
                    if (distance < (radius + eb->radius) * 1.25f) {
                        if (distance < closestDist) {
                            Entity* ent = static_cast<Entity*>(eb);
                            if (ent->reproductionValue > 0.0) {
                                other = ent;
                                closestDist = distance;
                            }
                        }
                    }
                }
            }

            if (other != nullptr) {
                // get the sum willingness to fuck. the crossover percentage is their percentage of the sum.
                float sumEagerness = reproductionValue + other->reproductionValue;
                perc = (other->reproductionValue) / sumEagerness; // will never divide by zero here since both must be above 0

                // split resource usage based on percentage contribution
                otherProteinUsage = proteinForChild * perc;
                otherSugarUsage = sugarForChild * perc;

                if (other->proteinLevels > otherProteinUsage &&
                    other->sugarLevels > otherSugarUsage) {
                    // if it has the resources to do its part, allow crossover

                    myProteinUsage = proteinForChild * (1 - perc);
                    mySugarUsage = sugarForChild * (1 - perc);

                    allowCrossover = true;
                }

            }
        }

        if (proteinLevels > myProteinUsage && sugarLevels > mySugarUsage) {
            float ang = rndEngine.randomFloat() * M_PI * 2;
            float childX = position.x + cos(ang) * radius * 0.9;
            float childY = position.y + sin(ang) * radius * 0.9;

            Entity* child = createChild(childX, childY);
            if (allowCrossover) {
                // allow crossover if both can pay their part.
                child->brain.crossover(other->brain, perc);
                child->genetics.crossover(other->genetics, perc);
            }
            child->mutate();
            child->angle = (rndEngine.randomFloat() * 2 - 1) * M_PI;

            child->proteinLevels = proteinForChild;
            child->sugarLevels = sugarForChild;
            child->generation = generation + 1;

            // use resources

            proteinLevels -= myProteinUsage;
            sugarLevels -= mySugarUsage;

            proteinExpense += myProteinUsage;
            sugarExpense += mySugarUsage;

            if (allowCrossover) {
                // if crossover is allowed, remove resources from other.
                other->proteinLevels -= otherProteinUsage;
                other->sugarLevels -= otherSugarUsage;

                other->proteinExpense += otherProteinUsage;
                other->sugarExpense += otherSugarUsage;
            }

            addToWorld.push_back(child);
        }

    }

}

Entity* Entity::createChild(float x, float y) {
    Entity* child = new Entity(*world, x, y);
    child->genetics.copy(genetics);
    child->brain.setValuesFrom(brain);
    child->origin = "child";
    child->speciesName = speciesName;
    //child->parentBranch = branchIndex; parentBranch is set when the child is being added to world.

    return child;
}

/*
    
    Mutation works as follows:
    Each weight has a chance to mutate.
    When toggling a weight, only one weight can be toggled on and off per mutation

*/
void Entity::mutate() {
    if (disableMutations) {
        return;
    }

    genetics.mutateAll(0.1);
    int totalWeights = brain.totalWeights;

    // major update. This is a major change in their brain
    // This serves as a way to get a new interaction
    float shiftChance = 0.05;
    float shiftScale = 2.0f;
    float enableWeightChance = 0.05;
    float disableWeightChance = 0.1;
    float mutateNeuronChance = 0.01;

    // major changes. Higher chance to remove weight than to add one. This means weights will tend not to exist if not needed.
    brain.mutate(shiftChance, shiftScale, enableWeightChance, disableWeightChance, mutateNeuronChance);

    // minor update
    // Enforces a slight variation on the population to let natural selection have more variation
    brain.mutate(10000.0f, 0.02f, 0.0, 0.0, 0.0);

    updateGenetics();
}

void Entity::useResources() {
    if (allLiveForever) {
        return; // dont use resources
    }

    float sugarUsage = 0;

    float globalMod = 0.0007;
    float forceUsage = (lastForceUsed.Length() / 150.0f) * 0.1;
    float upkeepEnergy = 0.05 * radius * radius;
    float brainUsage = 0.17 * brain.activeWeights; // linear at first but exponentially becomes more expensive
    float ageModifier = powf(1.0001, age);

    sugarUsage += forceUsage;
    sugarUsage += upkeepEnergy;
    sugarUsage += brainUsage;
    sugarUsage *= globalMod * ageModifier;

    if (sugarUsage < 0) {
        std::cerr << "Sugar usage is a negative value. Likely a serious error." << std::endl;
    }

    sugarExpense += sugarUsage;
    sugarLevels -= sugarUsage;

    if (sugarLevels < 0) {
        sugarLevels = 0;
    }

    if (sugarLevels <= 0) {
        isAlive = false;
    }
}

void Entity::death() {
    if (initDeath)
        return;
    attackingAmount = 0; // set to zero to stop it from rendering on corpses

    initDeath = true;
    // add body to protein store. add a bit of sugar
    // 100% of body protein is gained, 50% of body sugar is gained

    proteinLevels += storedProtein;
    sugarLevels += storedSugar;

    displayColor.x = 0.3;
    displayColor.y = 0.3;
    displayColor.z = 0.3;
    scentValue.x = 0.3;
    scentValue.y = 0.3;
    scentValue.z = 0.3;
}

void Entity::growth() {
    if (willToGrow <= 0.0) {
        return;
    }

    if (growthRadius < geneMaxSize) {
        // try to grow
        // growing uses proteins to contribute to the body
        float growthRate = 0.01f;
        float growthAmount = growthRate * willToGrow;

        float currentArea = growthRadius * growthRadius;
        float nextArea = pow(std::min(growthRadius + growthAmount, geneMaxSize), 2);

        float areaToGrow = nextArea - currentArea;
        if (areaToGrow > 0) {
            float proteinNeeded = areaToGrow * 1;
            float sugarNeeded = areaToGrow * 0.1;
            if (proteinLevels > proteinNeeded && sugarLevels > sugarNeeded) {
                growthRadius = std::min(geneMaxSize, growthRadius + growthAmount);

                proteinLevels -= proteinNeeded;
                sugarLevels -= sugarNeeded;

                proteinExpense += proteinNeeded;
                sugarExpense += sugarNeeded;
            }
        }


    }

    // check if growthRadius should update current radius
    if (geneMaxSize == growthRadius || abs(growthRadius - radius) > 0.1) {
        radius = growthRadius;
        destroyBody();
        buildBody();
    }
}

void Entity::digestion() {
    stomach.maxSize = 10 * radius * radius;

    // digest food in stomach
    float multiplier = 4.0;
    b2Vec2 income = stomach.digestion(multiplier);

    if (income.x < 0 || income.y < 0) {
        std::cerr << "Cell income is negative. Likely a serious error. income: " << income.x << ", " << income.y << std::endl;
    }

    // decrease digestion income by age
    proteinIncome += income.x * getAgeMod();
    sugarIncome += income.y * getAgeMod();

    if (proteinIncome > 1.0e-6)
        proteinLevels += proteinIncome * getAgeMod();
    if (sugarIncome > 1.0e-6)
        sugarLevels += sugarIncome * getAgeMod();

}

void Entity::aggression() {
    if (allLiveForever) {
        return; // dont attack
    }

    attackingAmount = std::max(0.0f, attackingAmount - 0.05f);
    // if wants to attack
    if (attackValue > 0.0) {
        
        EnvironmentBody* toAttack = nullptr;

        //std::cout << "My angle: " << angle << std::endl;
        for (HashedPosition* hentity : hashResult.nearby) {
            EnvironmentBody* entity = static_cast<EnvironmentBody*>(hentity);

            //float angleTo = atan2(entity->position.y - position.y, entity->position.x - position.x);
            //float dist = distanceTo(entity) - radius - entity->radius;
            //std::cout << "Angle to: " << angleTo << ", Dist: " << dist << ", Is attacking: " << isInFrontOf(entity) << std::endl;
            if (isInFrontOf(entity)) {
                toAttack = entity;
            }
        }

        // check if target is in front of entity
        if (toAttack != nullptr  && toAttack != this) {
            // attack this cell now.

            float sizeRatio = radius / toAttack->radius;
            if (toAttack->isPlant) {
                sizeRatio = 1.0f;
            }

            float attackPower = std::max(0.0f, attackValue * sizeRatio * (1 * radius) * getAgeMod());

            float pAmount = std::min(toAttack->proteinLevels, attackPower);
            float sAmount = std::min(toAttack->sugarLevels, attackPower);

            if (pAmount + sAmount > 0.001 && stomach.canFit(pAmount + sAmount)) {
                float pComplex;
                float sComplex;
                if (toAttack->isPlant) {
                    // give bite properties of plant
                    pComplex = 2.2;
                    sComplex = 0.4;
                }
                else {
                    // give bite properties of meat
                    pComplex = 0.4;
                    sComplex = 1.0;
                }

                if (pAmount < 0) {
                    pAmount = 0;
                }
                if (sAmount < 0) {
                    sAmount = 0;
                }


                float angleTo = MyMath::approxAtan2(toAttack->position.y - position.y, toAttack->position.x - position.x);
                stomach.addBite(pComplex, sComplex, pAmount, sAmount);
                float angleToAttack = MyMath::normalizeAngle(angleTo);

                toAttack->proteinLevels -= pAmount;
                toAttack->sugarLevels -= sAmount;
                toAttack->attackedAmount += attackPower;
                toAttack->lastAttackAngle = angleToAttack;
                attackingAmount = 1.0f;

                if (toAttack->sugarLevels <= 0.0) {
                    // kill it if it runs out of sugar.
                    toAttack->isAlive = false;
                }
            }

        }

    }
}

b2Vec3 Entity::getAverageNearbyScents() {
    b2Vec3 avgScents(0, 0, 0);
    for (int i = 0; i < hashResult.nearby.size(); i++) {
        EnvironmentBody* entity = static_cast<EnvironmentBody*>(hashResult.nearby[i]);
        b2Vec3 vec = b2Vec3(entity->scentValue);
        float d = hashResult.distances[i];
        vec.x /= (1.0f + d);
        vec.y /= (1.0f + d);
        vec.z /= (1.0f + d);
        avgScents += vec;
    }
    
    int nearSize = hashResult.nearby.size();
    if (nearSize == 0) {
        nearSize = 1;
    }
    avgScents.x /= nearSize;
    avgScents.y /= nearSize;
    avgScents.z /= nearSize;

    return avgScents;
}

bool Entity::isBindedTo(Entity* entity) {
    for (Entity* bind : bindedTo) {
        if (bind == entity) {
            return true;
        }
    }

    return false;
}

Eigen::VectorXf Entity::getReceptorInputs() {
    int amount = 3;
    Eigen::VectorXf vector = Eigen::VectorXf(9 * amount);
    vector.setZero();

    float angleEach = M_PI / (amount - 1);
    for (int i = 0; i <= amount - 1; i++) {
        float eyeAngle = angle + angleEach * (i)-M_PI / 2;
        // eyeAngle is the angle from the entity to look from

        b2Vec3 color(0, 0, 0);
        float distance = sightRange;

        // see if any entity is on this path
        EnvironmentBody* entity = getClosestIntersectingEntity(position, eyeAngle, sightRange, hashResult);
        if (entity == nullptr) {

            bool canSeeWall = false;

            if (canSeeWall) {

            }
            else {
                vector[i * 6 + 0] = 0;
                vector[i * 6 + 1] = 0;
                vector[i * 6 + 2] = 1;
                vector[i * 6 + 3] = sightRange / sightRange;
                vector[i * 6 + 4] = 0;
                vector[i * 6 + 5] = 0;
                vector[i * 6 + 6] = 0;
                vector[i * 6 + 7] = 0;
                vector[i * 6 + 8] = 0;
            }

        }
        else {

            double theta1 = entity->angle;
            double theta2 = angle;
            double delta_theta = MyMath::normalizeAngle(theta1 - theta2);
            double cos_delta_theta = MyMath::approxCos(delta_theta);
            double sin_delta_theta = MyMath::approxSin(delta_theta);

            vector[i * 6 + 0] = entity->scentValue.x;
            vector[i * 6 + 1] = entity->scentValue.y;
            vector[i * 6 + 2] = entity->scentValue.z;
            vector[i * 6 + 3] = distanceTo(entity) / sightRange;
            vector[i * 6 + 4] = entity->radius;
            vector[i * 6 + 5] = cos_delta_theta / M_PI; // the relative angle it is facing based from this
            vector[i * 6 + 6] = sin_delta_theta / M_PI; // the relative angle it is facing based from this
            vector[i * 6 + 7] = entity->proteinLevels / 10;
            vector[i * 6 + 8] = entity->sugarLevels / 10;
        }

    }


    return vector;
}

Eigen::VectorXf Entity::getNearbyEntityInputs() {
    int amount = 3;
    Eigen::VectorXf vector = Eigen::VectorXf(11 * amount);
    vector.setZero();


    int indexesUsed[3];

    std::vector<int> alreadySeen;
    bool cancelSearch = false; // skip other views if not enough entities nearby.
    for (int i = 0; i < amount; i++) {
        if (cancelSearch) {
            continue;
        }

         
        float lowest = 0;
        int index = -1;

        for (int j = 0; j < hashResult.distances.size(); j++) {
            if (std::find(alreadySeen.begin(), alreadySeen.end(), j) != alreadySeen.end()) {
                // already saw this one.
                continue;
            }
            float dist = hashResult.distances[j];
            if (index == -1 || lowest > dist) {
                lowest = dist;
                index = j;
            }
        }

        indexesUsed[i] = index;
        if (index != -1) {

            EnvironmentBody* entity = static_cast<EnvironmentBody*>(hashResult.nearby[index]);

            double theta1 = entity->angle;
            double theta2 = angle;
            double entity_angle_delta_theta = MyMath::normalizeAngle(theta1 - theta2);
            double entity_angle_cos_delta_theta = MyMath::approxCos(entity_angle_delta_theta);
            double entity_angle_sin_delta_theta = MyMath::approxSin(entity_angle_delta_theta);

            double delta_theta = MyMath::normalizeAngle(angle - entity->angle);
            double cos_delta_theta = MyMath::approxCos(delta_theta);
            double sin_delta_theta = MyMath::approxSin(delta_theta);

            vector[i * 11 + 0] = entity->scentValue.x;
            vector[i * 11 + 1] = entity->scentValue.y;
            vector[i * 11 + 2] = entity->scentValue.z;
            vector[i * 11 + 3] = distanceTo(entity) / sightRange;
            vector[i * 11 + 4] = entity->radius;
            vector[i * 11 + 5] = cos_delta_theta; // the relative angle to the entity
            vector[i * 11 + 6] = sin_delta_theta; // the relative angle to the entity
            vector[i * 11 + 7] = entity_angle_cos_delta_theta; // the relative angle it is facing based from this
            vector[i * 11 + 8] = entity_angle_sin_delta_theta; // the relative angle it is facing based from this
            vector[i * 11 + 9] = entity->proteinLevels / 10.0f;
            vector[i * 11 + 10] = entity->sugarLevels / 10.0f;

            alreadySeen.push_back(index);
        }
        else {
            // add blank values since nothing to see.
            cancelSearch = true;

            vector[i * 11 + 0] = 0;
            vector[i * 11 + 1] = 0;
            vector[i * 11 + 2] = 0;
            vector[i * 11 + 3] = 1; // default to max distance
            vector[i * 11 + 4] = 0;
            vector[i * 11 + 5] = 0; // the relative angle to the entity
            vector[i * 11 + 6] = 0; // the relative angle to the entity
            vector[i * 11 + 7] = 0; // the relative angle it is facing based from this
            vector[i * 11 + 8] = 0; // the relative angle it is facing based from this
            vector[i * 11 + 9] = 0;
            vector[i * 11 + 10] = 0;
        }

    }

    if (focused) {

        float angle = 0;
        if (indexesUsed[0] != -1) {

            EnvironmentBody* entity = static_cast<EnvironmentBody*>(hashResult.nearby[indexesUsed[0]]);

            double theta1 = entity->angle;
            double theta2 = angle;
            double entity_angle_delta_theta = MyMath::normalizeAngle(theta1 - theta2);
            angle = entity_angle_delta_theta;
        }

        //std::cout << "Relative angle: " << angle << std::endl;
        //std::cout << "Cos: " << vector[5] << " Sin: " << vector[6] << std::endl << std::endl;
    }


    return vector;
}

Eigen::VectorXf Entity::getWallSightInputs() {
    Eigen::VectorXf vector = Eigen::VectorXf(4);

    float step = sightRange / vector.size();
    b2Vec2 direction(cos(angle), sin(angle));

    for (int i = 0; i < vector.size(); i++) {
        b2Vec2 pos;
        pos.x = direction.x * (i) * step;
        pos.y = direction.y * (i) * step;

        float val = borderData->caveWalls->getNoiseLevel(pos.x, pos.y);

        vector[i] = val < borderData->caveWalls->threshold ? 0 : 1;
    }


    return vector;
}

void Entity::updateBrain() {

    float ageValue = age / 5000.0f;
    float stomachProtein = 0;
    float stomachSugar = 0;
    float attackedByValue = attackedAmount / (radius * radius);
    for (EntityBite* bite : stomach.contents) {
        stomachProtein += bite->remainingProtein;
        stomachSugar += bite->remainingSugar;
    }
    stomachProtein = stomachProtein / 10.0f;
    stomachSugar = stomachSugar / 10.0f;

    float heartbeat = sin(age * M_PI / 16.0); // provides a smooth shift from -1 to 1 for a heartbeart-like function
    float proteinLevel = proteinLevels / 10.0f;
    float sugarLevel = sugarLevels / 10.0f;
    float bindCount = static_cast<float>(bindedTo.size()) / 8.0f;

    Eigen::VectorXf nearbyInputs = getNearbyEntityInputs();
    Eigen::VectorXf wallSightInputs = getWallSightInputs();

    b2Vec3 avgScents = getAverageNearbyScents();
    inputVector << 
        ageValue, heartbeat,
        proteinLevel, sugarLevel,
        stomachProtein, stomachSugar, 
        attackedByValue, cos(lastAttackAngle), sin(lastAttackAngle),
        attackingAmount,
        avgScents.x, avgScents.y, avgScents.z, 
        nearbyInputs, wallSightInputs;




    if (int(inputVector.size()) != brain.input_size) {
        std::cerr << "Input is bigger than brain" << std::endl;
        std::cerr << "Brain inputs: " << brain.input_size << std::endl;
        std::cerr << "Actual inputs: " << inputVector.size() << std::endl;
        std::cerr << "nearbyInputs: " << nearbyInputs.size() << std::endl;
    }



}

void Entity::useBrain() {

    // output calculations

    outputs = brain.forward(inputVector);
    if (!outputs.allFinite()) {
        std::cerr << "inputVector:" << std::endl;
        for (int i = 0; i < inputVector.size(); i++) {
            std::cerr << i << ": " << inputVector(i) << std::endl;
        }

        std::cerr << "outputs:" << std::endl;
        for (int i = 0; i < outputs.size(); i++) {
            std::cerr << i << ": " << outputs(i) << std::endl;
        }

        throw std::runtime_error("Position isn't valid.");
    }
    //std::cerr << "outputs:" << std::endl;
    //for (int i = 0; i < outputs.size(); i++) {
    //    std::cerr << outputs(i);
    //}
    //std::cerr << std::endl;

    float forward = outputs(0);
    float turn = outputs(1);
    float attack = outputs(2);
    float split = outputs(3);
    float grow = outputs(4);

    // allow a slow reverse
    if (forward < 0) {
        forward = forward * 0.1;
    }

    attackValue = attack;
    reproductionValue = split;
    willToGrow = grow;

    // handle movement
    b2Vec2 force(forward * 150 * geneSpeedModifier, 0.0f);
    b2Rot rotMatrix(angle);
    force = b2Mul(rotMatrix, force);
    lastForceUsed = force;

    if (!force.IsValid()) {
        std::cerr << "force:" << force.x << "," << force.y << std::endl;
        std::cerr << "forward:" << forward << std::endl;
        std::cerr << "angle:" << body->GetAngle() << std::endl;
        std::cerr << "rotation:" << force.x << "," << force.y << std::endl;

        std::cerr << "inputVector:" << std::endl;
        for (int i = 0; i < inputVector.size(); i++) {
            std::cerr << i << ": " << inputVector(i) << std::endl;
        }

        std::cerr << "outputs:" << std::endl;
        for (int i = 0; i < outputs.size(); i++) {
            std::cerr << i << ": " << outputs(i) << std::endl;
        }
        throw std::runtime_error("force isn't valid.");
    }

    // aging slows them

    float agingMod = getAgeMod();
    force.x *= agingMod;
    force.y *= agingMod;
    body->ApplyTorque(turn * 100 * agingMod, true);
    body->ApplyForceToCenter(force, true);
}


float Entity::getAgeMod() {
    if (allLiveForever) {
        return 1.0f;
    }

    float max_age = 3000 * sqrt(radius); // age at which it reaches 0~ (is influenced by steepness)
    float steepness = 1.4f; // how fast they start aging (values below 1.0 unsafe)

    float agingMod = -std::tanh((4 * steepness * age) / (radius * max_age) - 2 * steepness) * 0.5 + 0.5;
    return std::max(0.0f, agingMod);
}

EnvironmentBody* EnvironmentBody::getClosestIntersectingEntity(b2Vec2& origin, float angle, float length, HashResult& result) {
    angle = MyMath::normalizeAngle(angle);
    EnvironmentBody* closestEntity = nullptr;
    float minDistance = std::numeric_limits<float>::max();

    for (int i = 0; i < result.nearby.size(); i++) {
        EnvironmentBody* entity = static_cast<EnvironmentBody*>(result.nearby[i]);

        // intersects if it is within range.
        float dist = std::abs(result.distances[i] - entity->radius);
        if (dist < minDistance && dist < length) {

            b2Vec2 relPos = entity->position - origin;
            float toEntity = MyMath::approxAtan2(relPos.y, relPos.x);
            toEntity = MyMath::normalizeAngle(toEntity);

            bool intersects = abs(angle - toEntity) < (4 * M_PI / 180);

            if (intersects) {
                minDistance = dist;
                closestEntity = entity;
            }
        }
    }

    return closestEntity;
}

// Function to check if a line intersects a circle
bool EnvironmentBody::doesIntersect(b2Vec2& origin, float angle, float length, EnvironmentBody* entity) {
    // Calculate the line end point from the angle (assuming a unit length line)
    b2Vec2 lineEnd(origin.x + MyMath::approxCos(angle) * length, origin.y + MyMath::approxSin(angle) * length);

    // Calculate the distance between the center of the circle and the line
    float dist = MyMath::pointToLineDistance(origin, lineEnd, entity->position);

    // If the distance is less than or equal to the circle's radius, there is an intersection
    return dist <= entity->radius;
}