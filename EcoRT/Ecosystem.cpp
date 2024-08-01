#include "Ecosystem.h"

// Definitions of class methods

Ecosystem::Ecosystem() {
    // Initialize your members here
    worldSeed = rndEngine.randomInt(0, 100000000);


    b2Vec2 bottomLeft = border.bounds[0];
    float width = world_size.x;
    float height = world_size.y;

    shouldSave = true;
    bool loadSave = true;

    bool loaded = false;
    if (loadSave) {
        loaded = loadFromFile();
    }
    renderer.init();

    // create border
    createBorder();

    if (!loaded) {
        // create starting entities
        allEntities = std::vector<EnvironmentBody*>();
        entitiesList = std::vector<Entity*>();
        plantsList = std::vector<PlantEntity*>();
        for (int i = 0; i < starting_entities; i++) {
            b2Vec2 spawn = getRandomSpawnPoint();
            Entity* entity = new Entity(world, spawn.x, spawn.y);
            for (int j = 0; j < 10; j++)
                entity->mutate();
            entity->angle = random() * M_PI * 2;
            entity->buildBody();
            entity->origin = "spawned";
            addEntity(entity);
        }

        int amountOfPlants = calculateAmountOfPlants();
        for (int i = 0; i < amountOfPlants; i++) {
            float x = bottomLeft.x + random() * width;
            float y = bottomLeft.y + random() * height;
            b2Vec2 spawn = getRandomSpawnPoint();
            PlantEntity* entity = new PlantEntity(world, spawn.x, spawn.y);
            entity->angle = random() * M_PI * 2;
            entity->buildBody();
            entity->origin = "spawned";
            addEntity(entity);
        }

        Entity* first = entitiesList[0];
        TreeEntry te = TreeEntry(first->brain, first->genetics, totalStep, first->displayColor, 0);
        treeOfLife.setFirstSpecies(te);
    }
    
    // reset tree of life
    //CellBrain cb = CellBrain();
    //cb.build();
    //TreeEntry te = TreeEntry(cb, Genetics(), totalStep, b2Vec3(1, 1, 1), 0);
    //treeOfLife.setFirstSpecies(te);



    GLFWwindow* window = renderer.getWindow();
    glfwSetErrorCallback(GLFWCallbacks::glfw_error_callback);
    glfwSetWindowUserPointer(window, this);
    glfwSetKeyCallback(window, GLFWCallbacks::key_callback);
    glfwSetCursorPosCallback(window, GLFWCallbacks::cursor_position_callback);
    glfwSetFramebufferSizeCallback(window, GLFWCallbacks::framebuffer_size_callback);
    glfwSetScrollCallback(window, GLFWCallbacks::scroll_callback);
    glfwSetMouseButtonCallback(window, GLFWCallbacks::mouse_button_callback);
    glfwSetWindowCloseCallback(window, GLFWCallbacks::close_window_callback);
    renderer.buildGUI();

    int frameWidth;
    int frameHeight;
    glfwGetFramebufferSize(window, &frameWidth, &frameHeight);
    renderer.viewportSize.x = frameWidth;
    renderer.viewportSize.y = frameHeight;
}

void Ecosystem::setFullscreen(bool enable) {

    GLFWwindow* window = renderer.getWindow();
    // true fullscreen
    if (enable) {
        isFullscreen = true;

        int windowPosX, windowPosY;
        int windowWidth, windowHeight;
        glfwGetWindowPos(window, &windowPosX, &windowPosY);
        glfwGetWindowSize(window, &windowWidth, &windowHeight);
        screenDataFullscreen.w = windowPosX;
        screenDataFullscreen.x = windowPosY;
        screenDataFullscreen.y = windowWidth;
        screenDataFullscreen.z = windowHeight;

        GLFWmonitor* primaryMonitor = glfwGetPrimaryMonitor();
        const GLFWvidmode* mode = glfwGetVideoMode(primaryMonitor);

        glfwSetWindowMonitor(window, primaryMonitor, 0, 0, mode->width, mode->height, GLFW_DONT_CARE);
    }
    // regular bordered
    else {
        isFullscreen = false;

        int windowPosX = screenDataFullscreen.w;
        int windowPosY = screenDataFullscreen.x;
        int windowWidth = screenDataFullscreen.y;
        int windowHeight = screenDataFullscreen.z;
        glfwWindowHint(GLFW_DECORATED, GLFW_TRUE);
        glfwSetWindowMonitor(window, nullptr, windowPosX, windowPosY, windowWidth, windowHeight, GLFW_DONT_CARE);
    }

}

// detects if it is inside the noise levels of the walls.
bool Ecosystem::isInBorderBounds(float x, float y) {

    return !border.caveWalls->isInWalls(x, y);
}

b2Vec2 Ecosystem::getRandomSpawnPoint() {

    // spawns are split into circular regions of density
    // spawns can slowly move over time

    // guaranteed to be out of range
    float x = world_size.x * 2;
    float y = world_size.y * 2;

    // won't select an OOB point or a point inside the walls. 
    int attemptsLeft = 100;
    while (!isInWorldBounds(x, y) || !isInBorderBounds(x, y)) {
        x = (random() * 2 - 1) * world_size.x / 2;
        y = (random() * 2 - 1) * world_size.y / 2;

        if (attemptsLeft-- == 0) {
            std::cerr << "Couldn't find valid spawn." << std::endl;
            throw std::runtime_error("Couldn't find valid spawn.");
        }
    }
    
    return b2Vec2(x, y);
}

bool Ecosystem::isInWorldBounds(float x, float y) {
    return x >= -world_size.x / 2 + 1e-6 && y >= -world_size.y / 2 + 1e-6 &&
        x < world_size.x / 2 - 1e-6 && y < world_size.y / 2 - 1e-6;
}

void Ecosystem::removeEntity(EnvironmentBody* entity) {
    // only remove entities in post-update! 

    // TODO: Implement removing entity from bindedTo bodies

    allEntities.erase(std::remove(allEntities.begin(), allEntities.end(), entity), allEntities.end());
    plantsList.erase(std::remove(plantsList.begin(), plantsList.end(), entity), plantsList.end());
    entitiesList.erase(std::remove(entitiesList.begin(), entitiesList.end(), entity), entitiesList.end());

    delete entity;
}

void Ecosystem::addEntity(EnvironmentBody* entity) {
    if (!entity->hasBody) {
        entity->buildBody();
    }

    allEntities.push_back(entity);
    if (entity->isPlant) {
        plantsList.push_back(static_cast<PlantEntity*>(entity));
    }
    else {
        Entity* ent = static_cast<Entity*>(entity);
        entitiesList.push_back(ent);
    }
    entity->borderData = &border;

    entity->syncBox2d();
    spatialHash.insert(entity);
}

void Ecosystem::createBorder() {
    border.seed = worldSeed;

    b2BodyDef bodyDef;
    bodyDef.position.Set(0, 0);
    bodyDef.type = b2_staticBody;
    border.body = world.CreateBody(&bodyDef);
    border.body->SetBullet(false);


    float halfWidth = static_cast<float>(world_size.x) / 2.0f;
    float halfHeight = static_cast<float>(world_size.y) / 2.0f;

    b2Vec2 bottomLeft(-halfWidth, -halfHeight);
    b2Vec2 bottomRight(halfWidth, -halfHeight);
    b2Vec2 topLeft(-halfWidth, halfHeight);
    b2Vec2 topRight(halfWidth, halfHeight);
    border.bounds[0] = bottomLeft;
    border.bounds[1] = topRight;
    border.size = b2Vec2(world_size.x, world_size.y);


    b2EdgeShape left;
    left.SetTwoSided(bottomLeft, topLeft);
    b2EdgeShape top;
    top.SetTwoSided(topLeft, topRight);
    b2EdgeShape right;
    right.SetTwoSided(topRight, bottomRight);
    b2EdgeShape bottom;
    bottom.SetTwoSided(bottomRight, bottomLeft);

    addEdgeToBorder(left);
    addEdgeToBorder(top);
    addEdgeToBorder(right);
    addEdgeToBorder(bottom);


    // build cave
    border.caveWalls = new CaveOutline(worldSeed, -world_size.x / 2, -world_size.y / 2, world_size.x, world_size.y, 0.1, 1.0, 5.0);
    renderer.buildCavesTexture(worldSeed, border); // build cave noise map
    border.caveWalls->build(); // build cave borders
    border.caveWalls->buildShape(); // build 3d polygon for shadows
    renderer.initModelShadows(border); // setup opengl for cave shadows
    renderer.bakeModelShadows(border); // bake shadows to a shadowMap
    renderer.buildCavesModel(border); // generate line to render to display borders
    border.caveWalls->erosion(); // add visual texture to caves
    renderer.setCavesTexture(border.caveWalls->textureData, border); // send updated textureData to opengl

    b2BodyDef caveDef;
    caveDef.position.Set(0, 0);
    caveDef.type = b2_staticBody;
    border.caveBody = world.CreateBody(&caveDef);
    b2Filter caveFilter;
    caveFilter.categoryBits = WALL_BITS;
    caveFilter.maskBits = CELL_BITS;

    float totalArea = 0.0f;
    for (std::vector<CaveLine*>& list : border.caveWalls->chains) {

        // check if it loops
        int e = list.size() - 1;
        bool loops = abs(list[0]->x1 - list[e]->x2) < 0.001 && abs(list[0]->y1 - list[e]->y2) < 0.001;
        if (loops) {

            //// add all lines as edges
            //for (int index = 0; index < list.size(); index++) {
            //    CaveLine* line = list[index];

            //    b2EdgeShape edge;
            //    edge.SetTwoSided(b2Vec2(line->x1, line->y1), b2Vec2(line->x2, line->y2));

            //    addEdgeToBorder(edge);
            //}

            b2ChainShape chain;

            b2Vec2* vecs = new b2Vec2[list.size()];
            // create chains
            for (int index = 0; index < list.size(); index++) {
                CaveLine* line = list[index];
                vecs[index] = b2Vec2(line->x1, line->y1);
            }
            totalArea += LineOptimizer::calculatePolygonArea(std::vector<b2Vec2>(vecs, vecs + sizeof(vecs) / sizeof(vecs[0])));

            chain.CreateLoop(vecs, list.size());
            delete vecs; // vecs is stored in the function

            b2FixtureDef* fixtureDef = new b2FixtureDef();
            fixtureDef->shape = &chain;
            fixtureDef->density = 0.0f;
            fixtureDef->friction = 1.0f;
            fixtureDef->filter = caveFilter;

            // add chain to body
            border.body->CreateFixture(fixtureDef);

        }
        else {
            for (int index = 0; index < list.size(); index++) {
                CaveLine* line = list[index];

                b2EdgeShape edge;
                edge.SetTwoSided(b2Vec2(line->x1, line->y1), b2Vec2(line->x2, line->y2));

                addEdgeToBorder(edge);
            }
            //std::cerr << "Can't create loop box2d shape for non-loops." << std::endl;
            //throw std::runtime_error("Not a loop.");
        }

    }
    border.totalInaccessibleWorldArea = totalArea;



}

void Ecosystem::addEdgeToBorder(b2EdgeShape& edge) {
    b2Filter borderFilter;
    borderFilter.categoryBits = WALL_BITS;
    borderFilter.maskBits = CELL_BITS | SPAWN_CIRCLE_BITS;

    b2FixtureDef* fixtureDef = new b2FixtureDef();
    fixtureDef->shape = &edge;
    fixtureDef->density = 0.0f;
    fixtureDef->friction = 1.0f;
    fixtureDef->filter = borderFilter;

    // Top edge
    border.body->CreateFixture(fixtureDef);
}

/*
* Syncs every entities box2d location with their entity location
*/
void Ecosystem::syncEntitiesBox2d() {
    spatialHash.clear();
    int listSize = allEntities.size();

    #pragma omp parallel for schedule(runtime) if(shouldUseOpenMP)
    for (int i = 0; i < listSize; i++) {
        EnvironmentBody* entity = allEntities[i];
        entity->syncBox2d();
        b2Vec2 screenPos = worldCoordinatesToPixel(entity->position.x, entity->position.y);
        entity->screenPosition = worldCoordinatesToPixel(entity->position.x, entity->position.y);

        if (!isInWorldBounds(entity->position.x, entity->position.y)) {
            entity->isAlive = false;
            entity->requestsRemoval = true;
        }
        spatialHash.insert(entity);
    }

    if (focusedEntity != nullptr) {
        offsetX = focusedEntity->position.x;
        offsetY = focusedEntity->position.y;
    }
}


/*
* Represents the time of year between 0.0 and 4.0
*/
float Ecosystem::getSeasonTime() {
    double mod = fmod(totalStep, seasonLength);
    float time = (mod / seasonLength) * 4; // 1 for each season, the fraction is how far through

    return time;
}

/*
* Calculates the affect the season has on plant growth. Centered on 1.0
*/
float Ecosystem::getSeasonalGrowthFactor() {

    // rotate point on sphere axis


    // center is the mid-point value. variation is how much it varies between winter and summer
    float variation = 0.2;
    float center = 1.0;

    // minimum = center - variation
    // maximum = center + variation

    // a wave function that is fit into a certain range
    float expo = sin((totalStep / seasonLength - 0.1) * M_PI * 2);
    float seasonalChange = center + variation * ((pow(2.71828182846, expo) - 0.368) / 2.35 - 0.5) * 2;

    return seasonalChange;
}

std::string Ecosystem::getSeasonName() {
    int section = floor(getSeasonTime());

    if (section == 0) {
        return "Spring";
    }
    if (section == 1) {
        return "Summer";
    }
    if (section == 2) {
        return "Fall";
    }
    if (section == 3) {
        return "Winter";
    }

    return "idk";
}

/*
* Searches multiple locations, and spawns a plant in the least dense area.
*/
b2Vec2 Ecosystem::calculatePlantSpawnPoint() {
    // get X positions. return position with least plants.

    b2Vec2 lowestLocation;
    int leastPlants = -1;


    HashResult result;
    for (int i = 0; i < 6; i++) {
        b2Vec2 p = getRandomSpawnPoint();
        spatialHash.query(result, p.x, p.y, 32, nullptr);

        int plants = 0;
        for (HashedPosition* hp : result.nearby) {
            EnvironmentBody* eb = static_cast<EnvironmentBody*>(hp);
            if (eb->isPlant) {
                plants++;
            }
        }

        if (leastPlants == -1 || plants < leastPlants) {
            leastPlants = plants;
            lowestLocation = p;
        }

    }

    return lowestLocation;

}

/*
* Calculates total amount of plants to aim for. This varies by season, skill, and map.
*/
int Ecosystem::calculateAmountOfPlants() {
    float seasonalChange = getSeasonalGrowthFactor();
    float availableWorldArea = (world_size.x * world_size.y) - border.totalInaccessibleWorldArea;

    int amountOfPlants = 1 + floor(plantDensityPer1sq * availableWorldArea * seasonalChange * skillFactor);

    return amountOfPlants;
}

void Ecosystem::monitorEcosystem() {

    int plantCount = plantsList.size();
    int entityCount = entitiesList.size();

    // calculates season
    float seasonalChange = getSeasonalGrowthFactor();


    float living = livingEntities;
    float target = 500;
    float c = 1000000;
    float maxChange = 0.0001;
    skillFactor = skillFactor * std::clamp(exp((target - living) / (target * c)), 1-maxChange, 1+maxChange);
    if (skillFactor > 1)
        skillFactor = 1;


    if (plantCount < calculateAmountOfPlants()) {
        b2Vec2 bottomLeft = border.bounds[0];
        float width = world_size.x - 1;
        float height = world_size.y - 1;

        // spawn plant randomly
        b2Vec2 spawn = calculatePlantSpawnPoint();
        PlantEntity* entity = new PlantEntity(world, spawn.x, spawn.y);
        entity->origin = "spawned";
        addEntity(entity);
    }

    if (living < starting_entities) {
        b2Vec2 bottomLeft = border.bounds[0];
        float width = world_size.x - 1;
        float height = world_size.y - 1;

        // spawn plant randomly
        b2Vec2 spawn = getRandomSpawnPoint();
        Entity* entity = new Entity(world, spawn.x, spawn.y);
        entity->origin = "spawned";
        for(int i = 0; i < 10; i++)
            entity->mutate();
        entity->angle = random() * M_PI * 2 - M_PI;
        addEntity(entity);
        //living += 1; // use if looping
    }

    if (pressureTest) {
        while (entityCount < 5000) {
            b2Vec2 bottomLeft = border.bounds[0];
            float width = world_size.x - 1;
            float height = world_size.y - 1;

            // spawn plant randomly
            b2Vec2 spawn = getRandomSpawnPoint();
            Entity* entity = new Entity(world, spawn.x, spawn.y);
            entity->origin = "spawned";
            for (int i = 0; i < 10; i++)
                entity->mutate();
            entity->angle = random() * M_PI * 2 - M_PI;
            addEntity(entity);
            entityCount++;
        }
        while (plantCount < 1000) {
            b2Vec2 bottomLeft = border.bounds[0];
            float width = world_size.x - 1;
            float height = world_size.y - 1;

            // spawn plant randomly
            b2Vec2 spawn = getRandomSpawnPoint();
            PlantEntity* entity = new PlantEntity(world, spawn.x, spawn.y);
            entity->origin = "spawned";
            addEntity(entity);
            plantCount++;
        }
    }

}

std::string Ecosystem::getPrecisionString(double number, int precision) {
    std::stringstream stream;
    stream << std::fixed << std::setprecision(precision) << number;
    return stream.str();
}

void Ecosystem::updateText() {
    textData.clear();

    b2Vec3 color = b2Vec3(1, 1, 1);
    float y = 5;
    float h = 16;
    int i = 0;
    std::string seasonString = "Season: " + getPrecisionString(getSeasonalGrowthFactor(), 2) + " " + getSeasonName();
    textData.push_back(Render::createTextData("Time: " + getPrecisionString(totalStep, 2), 5, (i++ * h) + y, 16, color));
    textData.push_back(Render::createTextData(seasonString, 5, (i++ * h) + y, 16, color));
    textData.push_back(Render::createTextData("Entities: " + std::to_string(livingEntities), 5, (i++ * h) + y, 16, color));
    textData.push_back(Render::createTextData("Plants: " + std::to_string(plantsList.size()), 5, (i++ * h) + y, 16, color));
    textData.push_back(Render::createTextData("Skill Factor: " + std::to_string(skillFactor), 5, (i++ * h) + y, 16, color));
    textData.push_back(Render::createTextData("Sleeping: " + std::to_string(sleepingPlants), 5, (i++ * h) + y, 16, color));

    if (focusedEntity != nullptr) {

        std::string awakeStatus = focusedEntity->body->IsAwake() ? "true" : "false";
        i++;
        textData.push_back(Render::createTextData("Generation: " + std::to_string(focusedEntity->generation), 5, (i++ * h) + y, 16, color));
        textData.push_back(Render::createTextData("Age: " + std::to_string(focusedEntity->age), 5, (i++ * h) + y, 16, color));
        textData.push_back(Render::createTextData("Protein levels: " + std::to_string(focusedEntity->proteinLevels), 5, (i++ * h) + y, 16, color));
        textData.push_back(Render::createTextData("Sugar levels: " + std::to_string(focusedEntity->sugarLevels), 5, (i++ * h) + y, 16, color));
        //textData.push_back(Render::createTextData("Alive: " + std::to_string(focusedEntity->isAlive), 5, (i++ * h) + y, 16, color));
        textData.push_back(Render::createTextData("Radius: " + std::to_string(focusedEntity->radius), 5, (i++ * h) + y, 16, color));
        //textData.push_back(Render::createTextData("Awake: " + awakeStatus, 5, (i++ * h) + y, 16, color));
        //textData.push_back(Render::createTextData("Grad: " + std::to_string(focusedEntity->gradualVelocity.x) + ", " + std::to_string(focusedEntity->gradualVelocity.y), 5, (i++ * h) + y, 16, color));


        if (!focusedEntity->isPlant) {
            i++;
            Entity* entity = static_cast<Entity*>(focusedEntity);
            float stomachProtein = 0;
            float stomachSugar = 0;
            for (EntityBite* bite : entity->stomach.contents) {
                stomachProtein += bite->remainingProtein;
                stomachSugar += bite->remainingSugar;
            }
            textData.push_back(Render::createTextData("Stored: p" + std::to_string(entity->storedProtein) + ", s" + std::to_string(entity->storedSugar),
                5, (i++ * h) + y, 16, color));
            textData.push_back(Render::createTextData("growthRadius: " + std::to_string(entity->growthRadius), 5, (i++* h) + y, 16, color));

            textData.push_back(Render::createTextData("Max Size: " + std::to_string(entity->genetics.getValue("maxSize")), 5, (i++ * h) + y, 16, color));
            textData.push_back(Render::createTextData("Acidity: " + std::to_string(entity->genetics.getValue("digestionAcidity")), 5, (i++ * h) + y, 16, color));;
            textData.push_back(Render::createTextData("Weights: " + std::to_string(entity->brain.activeWeights), 5, (i++ * h) + y, 16, color));
            
            textData.push_back(Render::createTextData(
                "Stomach (" + std::to_string(entity->stomach.contents.size()) + "): p" + std::to_string(stomachProtein) + ", s" + std::to_string(stomachSugar),
                5, (i++ * h) + y, 16, color));

            i++;
            textData.push_back(Render::createTextData("Income: p" + std::to_string(entity->proteinIncome) + " / s" + std::to_string(entity->sugarIncome), 5, (i++ * h) + y, 16, color));;
            textData.push_back(Render::createTextData("Expense: p" + std::to_string(entity->proteinExpense) + " / s" + std::to_string(entity->sugarExpense), 5, (i++ * h) + y, 16, color));;

        }

    }

}

void Ecosystem::updateGui() {
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();

    if (showWorldPanel) {

        ImGui::Begin("World Settings");
        ImGui::Checkbox("Render borders", &border.shouldRenderBorders);
        ImGui::Checkbox("OpenMP", &shouldUseOpenMP);
        bool relocate = ImGui::Button("Relocate");
        ImGui::Checkbox("Reload Shaders", &wantsToReloadShaders);

        if (wantsToReloadShaders) {
            bool confirm = ImGui::Button("Confirm reload");
            if (confirm) {
                renderer.loadShaders();
                wantsToReloadShaders = false;
            }
        }
        ImGui::End();

        if (relocate) {
            // randomize all entity positions

            for (Entity* entity : entitiesList) {
                b2Vec2 newPos = getRandomSpawnPoint();

                entity->body->SetTransform(newPos, entity->angle);
                entity->syncBox2d();
            }

        }
    }

    ImGui::Begin("Tinker");
    ImGui::SliderFloat("Translate speed", &renderer.viewTranslationSpeed, 0.00001f, 0.001f, "%.6f", ImGuiSliderFlags_NoRoundToFormat);
    ImGui::SliderFloat("Scale speed", &renderer.viewScaleSpeed, 0.00001f, 0.001f, "%.6f", ImGuiSliderFlags_NoRoundToFormat);
    ImGui::Text(("Current scale: " + std::to_string(renderer.currentViewScale)).c_str());
    ImGui::Text(("Current x/y: " + std::to_string(renderer.currentWorldViewCenter.x) + "/" + std::to_string(renderer.currentWorldViewCenter.y)).c_str());
    ImGui::Text(("Translate interp: " + std::to_string(renderer.currentViewTranslateVelocity)).c_str());
    ImGui::Text(("Scale interp: " + std::to_string(renderer.currentViewScaleVelocity)).c_str());
    ImGui::SliderFloat("Translate rate", &renderer.viewTranslateRate, 0, 500, "%.2f", ImGuiSliderFlags_NoRoundToFormat);
    ImGui::SliderFloat("Scale rate", &renderer.viewScaleRate, 0, 0.1, "%.4f", ImGuiSliderFlags_NoRoundToFormat);
    ImGui::End();

    //ImGui::Begin("Tinker");
    //ImGui::Checkbox("Continuous", &border.caveWalls->continuousErosion);
    //bool startErosion = ImGui::Button("Erode");
    //ImGui::SliderInt("Droplets", &border.caveWalls->totalDroplets, 10000, 100000);
    //ImGui::SliderInt("Lifetime", &border.caveWalls->maxDropletLifetime, 30, 1000);
    //ImGui::SliderFloat("Erode speed", &border.caveWalls->erodeSpeed, 0, 1);
    //ImGui::Checkbox("Enable Erosion", &border.caveWalls->doErosion);
    //ImGui::Checkbox("Enable Deposition", &border.caveWalls->doDeposition);
    //ImGui::SliderFloat("Deposit speed", &border.caveWalls->depositSpeed, 0, 1);
    //ImGui::SliderFloat("Gravity", &border.caveWalls->gravity, 1, 10);
    //ImGui::SliderFloat("Water", &border.caveWalls->initialWaterVolume, 1, 10);
    //ImGui::SliderFloat("Speed", &border.caveWalls->initialSpeed, 1, 10);
    //ImGui::SliderFloat("Evaporate", &border.caveWalls->evaporateSpeed, 0, 1);
    //ImGui::SliderFloat("S. Capacity", &border.caveWalls->sedimentCapacityFactor, 0.01, 5);
    //ImGui::SliderFloat("Min Sediment", &border.caveWalls->minSedimentCapacity, 0.00005f, 0.0005f, "%.5f", ImGuiSliderFlags_NoRoundToFormat);
    //bool updateRadius = ImGui::SliderInt("Radius", &border.caveWalls->brushRadius, 0, 10);
    //ImGui::SliderFloat("Inertia", &border.caveWalls->inertia, 0.01, 2.0);
    //ImGui::SliderFloat("Wind speed", &border.caveWalls->windSpeed, 0.0, 0.01, "%.6f", ImGuiSliderFlags_NoRoundToFormat);
    //ImGui::SliderFloat("Sand Hardness", &border.caveWalls->sandHardness, 0.0, 1.0, "%.2f", ImGuiSliderFlags_NoRoundToFormat);
    //ImGui::SliderFloat("Stone Hardness", &border.caveWalls->stoneHardness, 0.0, 2.0, "%.2f", ImGuiSliderFlags_NoRoundToFormat);
    //ImGui::RotatingKnob("Wind Direction", &border.caveWalls->windAngle, 0, M_PI * 2);
    //bool resetNoise = ImGui::Button("Reset");
    //ImGui::End();
    //if (updateRadius) {
    //    border.caveWalls->buildBrush();
    //}
    //if (startErosion || border.caveWalls->continuousErosion) {
    //    border.caveWalls->erosion();
    //    renderer.setCavesTexture(border.caveWalls->textureData, border);
    //}
    //if (resetNoise) {
    //    int ij = 0;
    //    for (float v : border.caveWalls->originalTextureData)
    //        border.caveWalls->textureData[ij++] = v;
    //    renderer.setCavesTexture(border.caveWalls->textureData, border);
    //    border.caveWalls->erosionCounts = 0;
    //}

    //ImGui::Begin("Texture");
    //ImVec2 availSize = ImGui::GetContentRegionAvail();
    //availSize.y = availSize.y / 2;
    //bool updateShadows = ImGui::SliderFloat("Bias high", &border.caveWalls->meshShadowsBiasValue, 0.0001, 0.001);
    //updateShadows = updateShadows || ImGui::SliderFloat("Bias low", &border.caveWalls->minMeshShadowsBiasValue, 0.00001, 0.001);
    //ImGui::Image(reinterpret_cast<void*>(static_cast<intptr_t>(renderer.shadowMapTexture)), availSize, ImVec2(0, 0), ImVec2(1, 1));
    //ImGui::Image(reinterpret_cast<void*>(static_cast<intptr_t>(renderer.depthMapTexture)), availSize, ImVec2(0, 0), ImVec2(1, 1));
    //ImGui::End();
    //if (updateShadows) {
    //    renderer.bakeModelShadows(border);
    //}

    ImGui::Begin("World Data");
    ImGui::Text(("Branches: " + std::to_string(treeOfLife.allBranches.size())).c_str());
    ImGui::Text(("Depth: " + std::to_string(treeOfLife.deepestChain)).c_str());

    bool isWindowVisible = !ImGui::IsWindowCollapsed();

    // only render new tree if window is open.
    if (treeOfLife.allBranches.size() != lastBranchSize) {

        std::map<int, b2Vec3> extantBranches = std::map<int, b2Vec3>();
        std::map<int, int> insertionEvents = std::map<int, int>();

        for (Entity* entity : entitiesList) {
            if (!entity->isOnTreeOfLife) {
                continue;
            }
            if (!entity->isAlive) {
                continue;
            }

            int branch = entity->branchIndex;

            if (extantBranches.find(branch) == extantBranches.end()) {
                extantBranches[branch] = b2Vec3(entity->scentValue);
                insertionEvents[branch]++;
            }
            else {
                // if it already exists, add the color to it.
                extantBranches[branch] += b2Vec3(entity->scentValue);
                insertionEvents[branch]++;
            }
        }

        // average colors
        for (auto entry : insertionEvents) {
            b2Vec3 color = extantBranches[entry.first];
            color.x /= float(entry.second);
            color.y /= float(entry.second);
            color.z /= float(entry.second);

            extantBranches[entry.first] = color;
        }

        treeOfLife.trim(extantBranches);

        // active branches are the branches that are extant
        renderer.treeRenderer.render(treeOfLife, extantBranches);
        lastBranchSize = treeOfLife.allBranches.size();
    }
    ImVec2 wsize = ImGui::GetContentRegionAvail();
    ImGui::Image(reinterpret_cast<void*>(static_cast<intptr_t>(renderer.treeRenderer.renderedTexture)), wsize, ImVec2(0, 0), ImVec2(1, 1));
    //ImGui::Image(reinterpret_cast<void*>(static_cast<intptr_t>(border.noiseTexture)), wsize, ImVec2(0, 0), ImVec2(1, 1));
    ImGui::End();

    if (focusedEntity != nullptr) {

        ImGui::Begin("Entity");
        if (!focusedEntity->isPlant) {
            Entity* entity = static_cast<Entity*>(focusedEntity);

            ImGui::Text(("Species: " + entity->speciesName).c_str());
            for (std::pair<std::string, Gene*> entry : entity->genetics.phenotype) {
                std::string data = entry.second->name + ": " + std::to_string(entry.second->value);
                ImGui::Text(data.c_str());
            }

            Eigen::VectorXf& outputs = entity->outputs;
            std::string outputText = "";
            for (int i = 0; i < outputs.size(); i++) {
                outputText = outputText.append(std::to_string(i) + ": " + std::to_string(outputs[i]) + ", ");
            }
            ImGui::Text(outputText.c_str());

            // only re-render when if it is open.
            renderer.brainRenderer.render(entity->uid, entity->brain, false);

            bool b = ImGui::Checkbox("Trim brain", &renderer.brainRenderer.trimConnections);
            if (b) {
                renderer.brainRenderer.render(entity->uid, entity->brain, true);
            }

            ImGui::Image(reinterpret_cast<void*>(static_cast<intptr_t>(renderer.brainRenderer.renderedTexture)), ImGui::GetContentRegionAvail(), ImVec2(0, 0), ImVec2(1, 1));
        }

        ImGui::End();
    }

}

void Ecosystem::run() {

    float framesSinceFpsCheck = 0;
    float stepsSinceFpsCheck = 0;
    double currentFps = 0;

    auto lastTickTime = std::chrono::high_resolution_clock::now();
    auto lastRenderTime = std::chrono::high_resolution_clock::now();
    auto lastSaveTime = std::chrono::high_resolution_clock::now();
    auto lastCheckTime = std::chrono::high_resolution_clock::now();
    auto lastInputTime = std::chrono::high_resolution_clock::now();

    double saveFrameDuration = 60 * 10;
    double renderFrameDuration = 1.0f / double(targetFps);
    double inputFrameDuration = 1.0f / double(targetFps);
    while (shouldRun) {
        double tickFrameDuration = 1.0f / double(60 * speedUpRate);
        //std::chrono::duration<double> timeSinceLastInput = now - lastInputTime;

        // save every ten minutes
        auto now = std::chrono::high_resolution_clock::now();
        std::chrono::duration<double> timeSinceLastSave = now - lastSaveTime;
        if (timeSinceLastSave.count() >= saveFrameDuration) {
            saveToFile();

            lastSaveTime = std::chrono::high_resolution_clock::now();
        }

        // update at max speed if above 10, otherwise try sticking to speedUp
        now = std::chrono::high_resolution_clock::now();
        std::chrono::duration<double> timeSinceLastTick = now - lastTickTime;
        if (speedUpRate >= 10 || timeSinceLastTick.count() >= tickFrameDuration) {
            lastTickTime = now;

            // update entities every X steps
            if (total_steps % entityUpdateRate == 0) {
                updateEntities();
                syncEntitiesBox2d(); // sync this due to removed/added entities
            }

            monitorEcosystem(); // adds entities if too low
            float ts = timeStep;
            world.Step(ts, velIter, posIter);
            syncEntitiesBox2d(); // syncs after step
            totalStep = totalStep + ts;
            total_steps++;
            stepsSinceFpsCheck++;

        }

        // render at a fixed rate
        now = std::chrono::high_resolution_clock::now();
        std::chrono::duration<double> timeSinceLastRender = now - lastRenderTime;
        if (timeSinceLastRender.count() >= renderFrameDuration) {
            lastRenderTime = now;
            updateText();
            glfwPollEvents();
            updateGui();
            float seasonTime = getSeasonTime();
            float deltaTime = (timeSinceLastRender.count()) / (1000.0f / targetFps); // used for animations
            renderer.render(deltaTime, totalStep, entitiesList, plantsList, border, offsetX, offsetY, getScale(), focusedEntity, textData, seasonTime);
            framesSinceFpsCheck++;

            std::chrono::duration<double> timeSinceLastCheckUpdate = std::chrono::high_resolution_clock::now() - lastCheckTime;
            if (timeSinceLastCheckUpdate.count() >= 5.0) {
                double fps = framesSinceFpsCheck / timeSinceLastCheckUpdate.count();
                currentFps = fps;

                double stepsPerSecond = stepsSinceFpsCheck / timeSinceLastCheckUpdate.count();

                std::cout << "FPS: " << int(currentFps) << ", SPS: " << int(stepsPerSecond) << std::endl;
                framesSinceFpsCheck = 0;
                stepsSinceFpsCheck = 0;
                lastCheckTime = std::chrono::high_resolution_clock::now();
            }
        }

    }

    saveToFile();

    renderer.cleanUp();

}

float Ecosystem::getScale() {
    return pow(1.1, scaleStep);
}

b2Vec2 Ecosystem::worldCoordinatesToPixel(float x, float y) {

    b2Vec2 viewportSize = renderer.viewportSize;
    float wx = static_cast<float>(viewportSize.x);
    float wy = static_cast<float>(viewportSize.y);
    glm::mat4 mvp = renderer.modelViewProjectionMatrix;

    glm::vec4 clipCoordinates = mvp * glm::vec4(x, y, 0, 1);
    float w = clipCoordinates.w;
    clipCoordinates /= clipCoordinates.w;

    float screenX = 0.5f * (clipCoordinates.x + 1.0f) * wx;
    float screenY = 0.5f * (1.0f - clipCoordinates.y) * wy;

    return b2Vec2(screenX, screenY);
}

b2Vec2 Ecosystem::pixelToWorldCoordinates(float mx, float my) {
    b2Vec2 viewportSize = renderer.viewportSize;
    float wx = static_cast<float>(viewportSize.x);
    float wy = static_cast<float>(viewportSize.y);
    glm::mat4 mvp = renderer.calculateMVPMatrix(offsetX, offsetY, getScale(), true); // calculate more accurately

    float ndcX = (2.0f * mx) / wx - 1.0f;
    float ndcY = 1.0f - (2.0f * my) / wy;

    // Create an NDC coordinate with a Z value within the range (-1, 1) if needed.
    // For 2D, you can use 0 as the Z value.

    glm::vec3 ndcCoordinates(ndcX, ndcY, 0.0f);

    // Apply the MVP^-1 matrix to map NDC coordinates back to world coordinates.
    glm::vec4 worldCoordinates = glm::inverse(mvp) * glm::vec4(ndcCoordinates, 1.0f);
    worldCoordinates /= worldCoordinates.w;

    return b2Vec2(worldCoordinates.x, worldCoordinates.y);
}

EnvironmentBody* Ecosystem::getClosestTo(float x, float y, float range, EnvironmentBody* ignore) {
    HashResult result;
    spatialHash.query(result, x, y, range, ignore);

    float closestDist = 0;
    HashedPosition*found = nullptr;

    for (int i = 0; i < result.nearby.size(); i++) {
        float d = result.distances[i];

        if (d < closestDist || found == nullptr) {
            closestDist = d;
            found = result.nearby[i];
        }
    }

    return static_cast<EnvironmentBody*>(found);
}

void Ecosystem::updateEntities() {
    syncEntitiesBox2d();


    // query hash
    //#pragma omp parallel for schedule(runtime) if(shouldUseOpenMP)
    for (int i = 0; i < allEntities.size(); ++i) {
        EnvironmentBody* entity = allEntities[i];
        if (entity == nullptr) {
            continue;
        }

        if (entity->isAlive) {
            spatialHash.query((entity->hashResult), entity->position.x, entity->position.y, entity->sightRange, entity);
        }
    }

    // preupdate
    for (int i = 0; i < allEntities.size(); ++i) {
        EnvironmentBody* entity = allEntities[i];
        if (entity == nullptr) {
            continue;
        }

        entity->preupdate();
    }

    //// full update. thread-safe functions here
    #pragma omp parallel for schedule(runtime) if(shouldUseOpenMP)
    for (int i = 0; i < allEntities.size(); ++i) {
        EnvironmentBody* entity = allEntities[i];
        if (entity == nullptr) {
            continue;
        }
        entity->update();
    }

    livingEntities = 0;

    sleepingPlants = 0;
    // post update
    std::vector<EnvironmentBody*> allToAdd;
    for (int i = 0; i < allEntities.size(); ++i) {
        EnvironmentBody* entity = allEntities[i];
        if (entity == nullptr) {
            continue;
        }
        entity->postupdate();

        if(!entity->isPlant && entity->isAlive && !entity->requestsRemoval) {
            livingEntities++;
        }
        if (entity->isPlant) {
            if (!entity->body->IsAwake()) {
                sleepingPlants++;
            }
        }

        std::vector<EnvironmentBody*>& toAdd = entity->addToWorld;
        if (toAdd.size() > 0 && !pressureTest) {
            // adding to tree before adding children
            if (!entity->isPlant) {
                Entity* ent = static_cast<Entity*>(entity);
                if (!ent->isOnTreeOfLife && ent->generation >= 3) {
                    TreeEntry te = TreeEntry(ent->brain, ent->genetics, totalStep, ent->scentValue, ent->parentBranch);

                    int branch = treeOfLife.add(te);

                    ent->speciesName = branch == -1 ? "prima vita" : treeOfLife.allBranches[branch]->name;
                    ent->branchIndex = branch;
                    ent->isOnTreeOfLife = true;

                    // set parentBranch of children to add
                    // setting the branch lets us find its nearest species faster
                    for (EnvironmentBody* e : toAdd) {
                        Entity* e2 = static_cast<Entity*>(e);
                        e2->parentBranch = branch;
                    }
                }
            }

            for (EnvironmentBody* ent : toAdd) {
                allToAdd.push_back(ent);
            }

            entity->addToWorld.clear();
        }


    }


    // add all entities that were requested
    for (EnvironmentBody* ent : allToAdd) {
        addEntity(ent);
    }

    removeEntities();

}

void Ecosystem::removeEntities() {
    std::vector<EnvironmentBody*> toRemove;

    // find entities that need to be removed
    for (int i = 0; i < allEntities.size(); i++) {
        EnvironmentBody* entity = allEntities[i];
        if (entity->requestsRemoval) {
            entity->destroy();

            toRemove.push_back(entity);
        }
    }


    // remove entities from lists
    allEntities.erase(std::remove_if(allEntities.begin(), allEntities.end(),
        [](const EnvironmentBody* obj) {
            return obj->requestsRemoval;
        }), allEntities.end());

    plantsList.erase(std::remove_if(plantsList.begin(), plantsList.end(),
        [](const EnvironmentBody* obj) {
            return obj->requestsRemoval;
        }), plantsList.end());

    entitiesList.erase(std::remove_if(entitiesList.begin(), entitiesList.end(),
        [](const EnvironmentBody* obj) {
            return obj->requestsRemoval;
        }), entitiesList.end());

    // delete entities
    for (int i = 0; i < toRemove.size(); i++) {
        EnvironmentBody* entity = toRemove[i];

        if(entity == focusedEntity)
            setFocusedEntity(nullptr);

        delete entity;
    }

}

double Ecosystem::random() {
    return rndEngine.randomFloat();
}

bool Ecosystem::loadFromFile() {

    std::string path = worldFilePath + "/ecosystem-data.bin";

    std::cout << "Loading from " << path << std::endl;
    std::ifstream inputFile(path, std::ios::in | std::ios::binary);

    if (!inputFile.is_open()) {
        std::cerr << "Failed to open the file for reading." << std::endl;
        return false;
    }

    inputFile.read(reinterpret_cast<char*>(&offsetX), sizeof(float));
    inputFile.read(reinterpret_cast<char*>(&offsetY), sizeof(float));
    inputFile.read(reinterpret_cast<char*>(&scaleStep), sizeof(int));
    inputFile.read(reinterpret_cast<char*>(&totalStep), sizeof(double));
    inputFile.read(reinterpret_cast<char*>(&speedUpRate), sizeof(int));
    inputFile.read(reinterpret_cast<char*>(&worldSeed), sizeof(int));
    inputFile.read(reinterpret_cast<char*>(&skillFactor), sizeof(float));

    int xpos, ypos, rwidth, rheight;
    inputFile.read(reinterpret_cast<char*>(&xpos), sizeof(int));
    inputFile.read(reinterpret_cast<char*>(&ypos), sizeof(int));
    inputFile.read(reinterpret_cast<char*>(&rwidth), sizeof(int));
    inputFile.read(reinterpret_cast<char*>(&rheight), sizeof(int));
    renderer.nextWindowPosition.x = xpos;
    renderer.nextWindowPosition.y = ypos;
    renderer.nextWindowSize.x = rwidth;
    renderer.nextWindowSize.y = rheight;
    //std::cout << xpos << " " << ypos << " " << rwidth << " " << rheight << std::endl;

    int numEntities = 0;
    inputFile.read(reinterpret_cast<char*>(&numEntities), sizeof(int));

    for (int i = 0; i < numEntities; i++) {
        float x;
        float y;
        float angle;
        float radius;
        float growthRadius;

        float proteinLevels;
        float sugarLevels;
        float storedProtein;
        float storedSugar;
        int generation;
        int uid;
        int age;

        int branchIndex;
        int parentBranch;
        bool isOnTreeOfLife;
        size_t speciesNameSize;
        std::string speciesName;
        bool isAlive;
        bool requestsRemoval;
        bool initDeath;

        // load basic data
        inputFile.read(reinterpret_cast<char*>(&x), sizeof(float));
        inputFile.read(reinterpret_cast<char*>(&y), sizeof(float));
        inputFile.read(reinterpret_cast<char*>(&angle), sizeof(float));
        inputFile.read(reinterpret_cast<char*>(&radius), sizeof(float));
        inputFile.read(reinterpret_cast<char*>(&growthRadius), sizeof(float));
        inputFile.read(reinterpret_cast<char*>(&generation), sizeof(int));
        inputFile.read(reinterpret_cast<char*>(&uid), sizeof(int));
        inputFile.read(reinterpret_cast<char*>(&age), sizeof(int));
        inputFile.read(reinterpret_cast<char*>(&isAlive), sizeof(bool));
        inputFile.read(reinterpret_cast<char*>(&requestsRemoval), sizeof(bool));



        // read tree of life data
        inputFile.read(reinterpret_cast<char*>(&branchIndex), sizeof(int));
        inputFile.read(reinterpret_cast<char*>(&parentBranch), sizeof(int));
        inputFile.read(reinterpret_cast<char*>(&isOnTreeOfLife), sizeof(bool));

        inputFile.read(reinterpret_cast<char*>(&speciesNameSize), sizeof(size_t));
        speciesName.resize(speciesNameSize);
        inputFile.read(&speciesName[0], speciesNameSize);

        // read resources
        inputFile.read(reinterpret_cast<char*>(&proteinLevels), sizeof(float));
        inputFile.read(reinterpret_cast<char*>(&sugarLevels), sizeof(float));
        inputFile.read(reinterpret_cast<char*>(&storedProtein), sizeof(float));
        inputFile.read(reinterpret_cast<char*>(&storedSugar), sizeof(float));
        inputFile.read(reinterpret_cast<char*>(&initDeath), sizeof(bool));

        Entity* entity = new Entity(world, x, y);
        entity->age = age;
        entity->uid = uid;
        entity->angle = angle;
        entity->radius = radius;
        entity->growthRadius = growthRadius;
        entity->generation = generation;
        entity->isAlive = isAlive;
        entity->requestsRemoval = requestsRemoval;

        entity->branchIndex = branchIndex;
        entity->parentBranch = parentBranch;
        entity->speciesName = speciesName;
        entity->isOnTreeOfLife = isOnTreeOfLife;

        entity->proteinLevels = proteinLevels;
        entity->sugarLevels = sugarLevels;
        entity->storedProtein = storedProtein;
        entity->storedSugar = storedSugar;
        entity->initDeath = initDeath;

        // load genes
        int geneCount;
        inputFile.read(reinterpret_cast<char*>(&geneCount), sizeof(int));
        Genetics& genetics = entity->genetics;
        for (int j = 0; j < geneCount; j++) {
            size_t nameSize;
            inputFile.read(reinterpret_cast<char*>(&nameSize), sizeof(size_t));
            std::string name(nameSize, '\0');
            inputFile.read(&name[0], nameSize);

            Gene* gene = genetics.phenotype[name];
            // insert loaded gene data over default genes
            inputFile.read(reinterpret_cast<char*>(&gene->value), sizeof(float));
            inputFile.read(reinterpret_cast<char*>(&gene->min), sizeof(float));
            inputFile.read(reinterpret_cast<char*>(&gene->max), sizeof(float));
            inputFile.read(reinterpret_cast<char*>(&gene->step), sizeof(float));

            //std::cout << name << " values: " << gene->value << ", step: " << gene->step << std::endl;
        }

        // load brain
        CellBrain& brain = entity->brain;
        inputFile.read(reinterpret_cast<char*>(brain.inputLayerWeights.data()), sizeof(float) * brain.inputLayerWeights.rows() * brain.inputLayerWeights.cols());
        inputFile.read(reinterpret_cast<char*>(brain.inputLayerBias.data()), sizeof(float) * brain.inputLayerBias.size());
        inputFile.read(reinterpret_cast<char*>(brain.outputLayerWeights.data()), sizeof(float) * brain.outputLayerWeights.rows() * brain.outputLayerWeights.cols());
        inputFile.read(reinterpret_cast<char*>(brain.outputLayerBias.data()), sizeof(float) * brain.outputLayerBias.size());

        inputFile.read(reinterpret_cast<char*>(brain.inputLayerWeightsMask.data()), sizeof(float) * brain.inputLayerWeightsMask.rows() * brain.inputLayerWeightsMask.cols());
        inputFile.read(reinterpret_cast<char*>(brain.inputLayerBiasMask.data()), sizeof(float) * brain.inputLayerBiasMask.size());
        inputFile.read(reinterpret_cast<char*>(brain.outputLayerWeightsMask.data()), sizeof(float) * brain.outputLayerWeightsMask.rows() * brain.outputLayerWeightsMask.cols());
        inputFile.read(reinterpret_cast<char*>(brain.outputLayerBiasMask.data()), sizeof(float) * brain.outputLayerBiasMask.size());

        inputFile.read(reinterpret_cast<char*>(brain.activationPerHidden.data()), sizeof(float) * brain.activationPerHidden.size());

        // load stomach
        int objectsInStomach;
        inputFile.read(reinterpret_cast<char*>(&objectsInStomach), sizeof(int));
        for (int j = 0; j < objectsInStomach; j++) {
            float acidity;
            float proteinComplexity;
            float sugarComplexity;
            float protein;
            float sugar;
            float digestionSteps;
            inputFile.read(reinterpret_cast<char*>(&acidity), sizeof(float));
            inputFile.read(reinterpret_cast<char*>(&proteinComplexity), sizeof(float));
            inputFile.read(reinterpret_cast<char*>(&sugarComplexity), sizeof(float));
            inputFile.read(reinterpret_cast<char*>(&protein), sizeof(float));
            inputFile.read(reinterpret_cast<char*>(&sugar), sizeof(float));
            inputFile.read(reinterpret_cast<char*>(&digestionSteps), sizeof(float));

            EntityBite* bite = new EntityBite(acidity, proteinComplexity, sugarComplexity, protein, sugar);
            bite->digestionSteps = digestionSteps;

            entity->stomach.contents.push_back(bite);
        }

        // after loading genes, make sure they're loaded in the entity
        entity->updateGenetics();
        addEntity(entity);
    }

    int numPlants = plantsList.size();
    inputFile.read(reinterpret_cast<char*>(&numPlants), sizeof(int));

    for (int i = 0; i < numPlants; i++) {
        float x;
        float y;
        inputFile.read(reinterpret_cast<char*>(&x), sizeof(float));
        inputFile.read(reinterpret_cast<char*>(&y), sizeof(float));


        PlantEntity* plant = new PlantEntity(world, x, y);
        inputFile.read(reinterpret_cast<char*>(&plant->angle), sizeof(float));
        inputFile.read(reinterpret_cast<char*>(&plant->radius), sizeof(float));
        inputFile.read(reinterpret_cast<char*>(&plant->generation), sizeof(int));
        inputFile.read(reinterpret_cast<char*>(&plant->uid), sizeof(int));
        inputFile.read(reinterpret_cast<char*>(&plant->age), sizeof(int));
        inputFile.read(reinterpret_cast<char*>(&plant->isAlive), sizeof(bool));
        inputFile.read(reinterpret_cast<char*>(&plant->requestsRemoval), sizeof(bool));

        inputFile.read(reinterpret_cast<char*>(&plant->proteinLevels), sizeof(float));
        inputFile.read(reinterpret_cast<char*>(&plant->sugarLevels), sizeof(float));

        addEntity(plant);
    }

    // load tree of life

    inputFile.read(reinterpret_cast<char*>(&treeOfLife.deepestChain), sizeof(int));
    int allBranchesSize;
    inputFile.read(reinterpret_cast<char*>(&allBranchesSize), sizeof(int));

    TreeEntry te = { CellBrain(), Genetics(), 0, b2Vec3(0, 0, 0), 0 };
    TreeBranch* trunk = new TreeBranch("", nullptr, te);
    treeOfLife.trunk = trunk;
    std::map<int, TreeBranch*> allBranches; // used to sort later into proper indices. map is self sorting
    readTrunk(inputFile, allBranches, trunk);

    treeOfLife.allBranches.clear();
    treeOfLife.allBranches.resize(allBranchesSize, nullptr);
    for (auto entry : allBranches) {
        treeOfLife.allBranches[entry.first] = entry.second;
    }

    inputFile.close();

    // update rng engine
    rndEngine.setSeed(worldSeed);

    return true;
}

void Ecosystem::saveToFile() {
    int worldSettingsBytes, entitiesBytes, plantsBytes, trunkBytes;

    std::string path = worldFilePath + "/ecosystem-data.bin";
    std::cout << "Saving to file " << path << std::endl;


    // Create the directory if it doesn't exist
    if (!std::filesystem::exists(worldFilePath)) {
        if (!std::filesystem::create_directory(worldFilePath)) {
            std::cerr << "Failed to create the directory." << std::endl;
            return;
        }
    }

    // rename old
    std::filesystem::path currentFileName = worldFilePath + "/ecosystem-data.bin";
    std::filesystem::path newFileName = worldFilePath + "/ecosystem-data-" + std::to_string(total_steps) + ".bin";

    if (std::filesystem::exists(currentFileName)) {
        try {
            std::filesystem::rename(currentFileName, newFileName);
        }
        catch (const std::filesystem::filesystem_error& err) {
            std::cerr << "Error renaming file: " << err.what() << std::endl;
        }
    }
    else {
        std::cerr << "File to be renamed does not exist." << std::endl;
    }


    std::ofstream outputStream = std::ofstream(path, std::ios::out | std::ios::binary);
    ByteWriter outputFile(outputStream);

    if (!outputFile.output.is_open()) {
        std::cerr << "Failed to open the file for writing." << std::endl;
        return;
    }
    int lastSize = 0;

    // offsetX, offsetY, scaleStep
    outputFile.write(reinterpret_cast<const char*>(&offsetX), sizeof(float));
    outputFile.write(reinterpret_cast<const char*>(&offsetY), sizeof(float));
    outputFile.write(reinterpret_cast<const char*>(&scaleStep), sizeof(int));
    outputFile.write(reinterpret_cast<const char*>(&totalStep), sizeof(double));
    outputFile.write(reinterpret_cast<const char*>(&speedUpRate), sizeof(int));
    outputFile.write(reinterpret_cast<const char*>(&worldSeed), sizeof(int));
    outputFile.write(reinterpret_cast<const char*>(&skillFactor), sizeof(float));


    int xpos, ypos, rwidth, rheight;
    glfwGetWindowPos(renderer.getWindow(), &xpos, &ypos);
    glfwGetWindowSize(renderer.getWindow(), &rwidth, &rheight);
    std::cout << xpos << " " << ypos << " " << rwidth << " " << rheight << std::endl;
    outputFile.write(reinterpret_cast<const char*>(&xpos), sizeof(int));
    outputFile.write(reinterpret_cast<const char*>(&ypos), sizeof(int));
    outputFile.write(reinterpret_cast<const char*>(&rwidth), sizeof(int));
    outputFile.write(reinterpret_cast<const char*>(&rheight), sizeof(int));

    int numEntities = entitiesList.size();
    outputFile.write(reinterpret_cast<const char*>(&numEntities), sizeof(int));

    worldSettingsBytes = outputFile.bytesWritten - lastSize;
    lastSize = outputFile.bytesWritten;

    for (Entity* entity : entitiesList) {
        float x = entity->position.x;
        float y = entity->position.y;
        float angle = entity->angle;
        int geneCount = entity->genetics.phenotype.size();

        // write position data
        outputFile.write(reinterpret_cast<const char*>(&x), sizeof(float));
        outputFile.write(reinterpret_cast<const char*>(&y), sizeof(float));
        outputFile.write(reinterpret_cast<const char*>(&angle), sizeof(float));
        outputFile.write(reinterpret_cast<const char*>(&entity->radius), sizeof(float));
        outputFile.write(reinterpret_cast<const char*>(&entity->growthRadius), sizeof(float));
        outputFile.write(reinterpret_cast<const char*>(&entity->generation), sizeof(int));
        outputFile.write(reinterpret_cast<const char*>(&entity->uid), sizeof(int));
        outputFile.write(reinterpret_cast<const char*>(&entity->age), sizeof(int));
        outputFile.write(reinterpret_cast<const char*>(&entity->isAlive), sizeof(bool));
        outputFile.write(reinterpret_cast<const char*>(&entity->requestsRemoval), sizeof(bool));

        // write tree of life data
        outputFile.write(reinterpret_cast<const char*>(&entity->branchIndex), sizeof(int));
        outputFile.write(reinterpret_cast<const char*>(&entity->parentBranch), sizeof(int));
        outputFile.write(reinterpret_cast<const char*>(&entity->isOnTreeOfLife), sizeof(bool));
        size_t speciesNameSize = entity->speciesName.size();
        outputFile.write(reinterpret_cast<const char*>(&speciesNameSize), sizeof(size_t));
        outputFile.write(reinterpret_cast<const char*>(entity->speciesName.c_str()), speciesNameSize);

        // write resources
        outputFile.write(reinterpret_cast<const char*>(&entity->proteinLevels), sizeof(float));
        outputFile.write(reinterpret_cast<const char*>(&entity->sugarLevels), sizeof(float));
        outputFile.write(reinterpret_cast<const char*>(&entity->storedProtein), sizeof(float));
        outputFile.write(reinterpret_cast<const char*>(&entity->storedSugar), sizeof(float));
        outputFile.write(reinterpret_cast<const char*>(&entity->initDeath), sizeof(bool));

        // write gene data
        outputFile.write(reinterpret_cast<const char*>(&geneCount), sizeof(int));
        for (auto& entry : entity->genetics.phenotype) {
            std::string name = entry.first;
            Gene* gene = entry.second;

            size_t nameSize = name.size();
            outputFile.write(reinterpret_cast<const char*>(&nameSize), sizeof(size_t));
            outputFile.write(reinterpret_cast<const char*>(name.c_str()), nameSize);

            outputFile.write(reinterpret_cast<const char*>(&gene->value), sizeof(float));
            outputFile.write(reinterpret_cast<const char*>(&gene->min), sizeof(float));
            outputFile.write(reinterpret_cast<const char*>(&gene->max), sizeof(float));
            outputFile.write(reinterpret_cast<const char*>(&gene->step), sizeof(float));

            //std::cout << name << " values: " << gene->value << ", step: " << gene->step << std::endl;
        }

        // write brain data
        CellBrain& brain = entity->brain;
        outputFile.write(reinterpret_cast<const char*>(brain.inputLayerWeights.data()), sizeof(float) * brain.inputLayerWeights.rows() * brain.inputLayerWeights.cols());
        outputFile.write(reinterpret_cast<const char*>(brain.inputLayerBias.data()), sizeof(float) * brain.inputLayerBias.size());
        outputFile.write(reinterpret_cast<const char*>(brain.outputLayerWeights.data()), sizeof(float) * brain.outputLayerWeights.rows() * brain.outputLayerWeights.cols());
        outputFile.write(reinterpret_cast<const char*>(brain.outputLayerBias.data()), sizeof(float) * brain.outputLayerBias.size());

        outputFile.write(reinterpret_cast<const char*>(brain.inputLayerWeightsMask.data()), sizeof(float) * brain.inputLayerWeightsMask.rows() * brain.inputLayerWeightsMask.cols());
        outputFile.write(reinterpret_cast<const char*>(brain.inputLayerBiasMask.data()), sizeof(float) * brain.inputLayerBiasMask.size());
        outputFile.write(reinterpret_cast<const char*>(brain.outputLayerWeightsMask.data()), sizeof(float) * brain.outputLayerWeightsMask.rows() * brain.outputLayerWeightsMask.cols());
        outputFile.write(reinterpret_cast<const char*>(brain.outputLayerBiasMask.data()), sizeof(float) * brain.outputLayerBiasMask.size());

        outputFile.write(reinterpret_cast<const char*>(brain.activationPerHidden.data()), sizeof(float) * brain.activationPerHidden.size());

        // write stomach data
        Stomach& stomach = entity->stomach;
        int objectsInStomach = stomach.contents.size();
        outputFile.write(reinterpret_cast<const char*>(&objectsInStomach), sizeof(int));
        for (EntityBite* bite : stomach.contents) {
            outputFile.write(reinterpret_cast<const char*>(&bite->stomachAcidity), sizeof(float));
            outputFile.write(reinterpret_cast<const char*>(&bite->proteinComplexity), sizeof(float));
            outputFile.write(reinterpret_cast<const char*>(&bite->sugarComplexity), sizeof(float));
            outputFile.write(reinterpret_cast<const char*>(&bite->remainingProtein), sizeof(float));
            outputFile.write(reinterpret_cast<const char*>(&bite->remainingSugar), sizeof(float));
            outputFile.write(reinterpret_cast<const char*>(&bite->digestionSteps), sizeof(float));
        }
    }
    entitiesBytes = outputFile.bytesWritten - lastSize;
    lastSize = outputFile.bytesWritten;


    int numPlants = plantsList.size();
    outputFile.write(reinterpret_cast<const char*>(&numPlants), sizeof(int));

    for (PlantEntity* plant : plantsList) {
        outputFile.write(reinterpret_cast<const char*>(&plant->position.x), sizeof(float));
        outputFile.write(reinterpret_cast<const char*>(&plant->position.y), sizeof(float));
        outputFile.write(reinterpret_cast<const char*>(&plant->angle), sizeof(float));
        outputFile.write(reinterpret_cast<const char*>(&plant->radius), sizeof(float));
        outputFile.write(reinterpret_cast<const char*>(&plant->generation), sizeof(int));
        outputFile.write(reinterpret_cast<const char*>(&plant->uid), sizeof(int));
        outputFile.write(reinterpret_cast<const char*>(&plant->age), sizeof(int));
        outputFile.write(reinterpret_cast<const char*>(&plant->isAlive), sizeof(bool));
        outputFile.write(reinterpret_cast<const char*>(&plant->requestsRemoval), sizeof(bool));

        outputFile.write(reinterpret_cast<const char*>(&plant->proteinLevels), sizeof(float));
        outputFile.write(reinterpret_cast<const char*>(&plant->sugarLevels), sizeof(float));
    }
    plantsBytes = outputFile.bytesWritten - lastSize;
    lastSize = outputFile.bytesWritten;

    // write tree of life
    outputFile.write(reinterpret_cast<const char*>(&treeOfLife.deepestChain), sizeof(int));
    int allBranchesSize = treeOfLife.allBranches.size();
    outputFile.write(reinterpret_cast<const char*>(&allBranchesSize), sizeof(int));
    writeTrunk(outputFile, treeOfLife.trunk);

    trunkBytes = outputFile.bytesWritten - lastSize;
    lastSize = outputFile.bytesWritten;

    outputFile.output.close();


    std::cout << std::fixed << std::setprecision(3);
    std::cout << "Total size: " << outputFile.bytesWritten * 0.0009765625 << "kb" << std::endl;
    std::cout << "World size: " << worldSettingsBytes * 0.0009765625 << "kb" << std::endl;
    std::cout << "Entities size: " << entitiesBytes * 0.0009765625 << "kb" << std::endl;
    std::cout << "Plants size: " << plantsBytes * 0.0009765625 << "kb" << std::endl;
    std::cout << "Trunk size: " << trunkBytes * 0.0009765625 << "kb" << std::endl;

}

void Ecosystem::readTrunk(std::ifstream& inputFile, std::map<int, TreeBranch*>& allBranches, TreeBranch* branch) {

    int childrenCount = 0;

    size_t nameSize;
    inputFile.read(reinterpret_cast<char*>(&nameSize), sizeof(size_t));
    std::string spName(nameSize, '\0');
    inputFile.read(&spName[0], nameSize);
    branch->name = spName;


    inputFile.read(reinterpret_cast<char*>(&branch->depth), sizeof(int));
    inputFile.read(reinterpret_cast<char*>(&childrenCount), sizeof(int));

    TreeEntry& te = branch->branchData;
    inputFile.read(reinterpret_cast<char*>(&te.branchIndex), sizeof(int));
    inputFile.read(reinterpret_cast<char*>(&te.worldTotalStepAtCreation), sizeof(float));
    inputFile.read(reinterpret_cast<char*>(&te.color.x), sizeof(float));
    inputFile.read(reinterpret_cast<char*>(&te.color.y), sizeof(float));
    inputFile.read(reinterpret_cast<char*>(&te.color.z), sizeof(float));

    //std::cout << "Reading branch " << branch->branchData.branchIndex << std::endl;
    allBranches[branch->branchData.branchIndex] = branch;

    // load brain
    CellBrain& brain = te.brain;
    inputFile.read(reinterpret_cast<char*>(brain.inputLayerWeights.data()), sizeof(float) * brain.inputLayerWeights.rows() * brain.inputLayerWeights.cols());
    inputFile.read(reinterpret_cast<char*>(brain.inputLayerBias.data()), sizeof(float) * brain.inputLayerBias.size());
    inputFile.read(reinterpret_cast<char*>(brain.outputLayerWeights.data()), sizeof(float) * brain.outputLayerWeights.rows() * brain.outputLayerWeights.cols());
    inputFile.read(reinterpret_cast<char*>(brain.outputLayerBias.data()), sizeof(float) * brain.outputLayerBias.size());

    inputFile.read(reinterpret_cast<char*>(brain.inputLayerWeightsMask.data()), sizeof(float) * brain.inputLayerWeightsMask.rows() * brain.inputLayerWeightsMask.cols());
    inputFile.read(reinterpret_cast<char*>(brain.inputLayerBiasMask.data()), sizeof(float) * brain.inputLayerBiasMask.size());
    inputFile.read(reinterpret_cast<char*>(brain.outputLayerWeightsMask.data()), sizeof(float) * brain.outputLayerWeightsMask.rows() * brain.outputLayerWeightsMask.cols());
    inputFile.read(reinterpret_cast<char*>(brain.outputLayerBiasMask.data()), sizeof(float) * brain.outputLayerBiasMask.size());

    inputFile.read(reinterpret_cast<char*>(brain.activationPerHidden.data()), sizeof(float) * brain.activationPerHidden.size());

    // load genes
    int geneCount;
    inputFile.read(reinterpret_cast<char*>(&geneCount), sizeof(int));
    Genetics& genetics = te.genetics;
    for (int j = 0; j < geneCount; j++) {
        size_t gene_nameSize;
        inputFile.read(reinterpret_cast<char*>(&gene_nameSize), sizeof(size_t));
        std::string name(gene_nameSize, '\0');
        inputFile.read(&name[0], gene_nameSize);

        Gene* gene = genetics.phenotype[name];
        // insert loaded gene data over default genes
        inputFile.read(reinterpret_cast<char*>(&gene->value), sizeof(float));
        inputFile.read(reinterpret_cast<char*>(&gene->min), sizeof(float));
        inputFile.read(reinterpret_cast<char*>(&gene->max), sizeof(float));
        inputFile.read(reinterpret_cast<char*>(&gene->step), sizeof(float));

        //std::cout << name << " values: " << gene->value << ", step: " << gene->step << std::endl;
    }

    for (int i = 0; i < childrenCount; i++) {
        TreeEntry te = { CellBrain(), Genetics(), 0, b2Vec3(0, 0, 0), 0 };
        TreeBranch* cbranch = new TreeBranch("", branch, te);
        branch->children.push_back(cbranch);
        readTrunk(inputFile, allBranches, cbranch);
    }
}

void Ecosystem::writeTrunk(ByteWriter& outputFile, TreeBranch* branch) {
    size_t nameSize = branch->name.size();
    int children = branch->children.size();

    outputFile.write(reinterpret_cast<const char*>(&nameSize), sizeof(size_t));
    outputFile.write(reinterpret_cast<const char*>(branch->name.c_str()), branch->name.size());
    outputFile.write(reinterpret_cast<const char*>(&branch->depth), sizeof(int));
    outputFile.write(reinterpret_cast<const char*>(&children), sizeof(int));

    TreeEntry& te = branch->branchData;
    outputFile.write(reinterpret_cast<const char*>(&te.branchIndex), sizeof(int));
    outputFile.write(reinterpret_cast<const char*>(&te.worldTotalStepAtCreation), sizeof(float));
    outputFile.write(reinterpret_cast<const char*>(&te.color.x), sizeof(float));
    outputFile.write(reinterpret_cast<const char*>(&te.color.y), sizeof(float));
    outputFile.write(reinterpret_cast<const char*>(&te.color.z), sizeof(float));

    // write brain
    CellBrain& brain = te.brain;
    outputFile.write(reinterpret_cast<const char*>(brain.inputLayerWeights.data()), sizeof(float) * brain.inputLayerWeights.rows() * brain.inputLayerWeights.cols());
    outputFile.write(reinterpret_cast<const char*>(brain.inputLayerBias.data()), sizeof(float) * brain.inputLayerBias.size());
    outputFile.write(reinterpret_cast<const char*>(brain.outputLayerWeights.data()), sizeof(float) * brain.outputLayerWeights.rows() * brain.outputLayerWeights.cols());
    outputFile.write(reinterpret_cast<const char*>(brain.outputLayerBias.data()), sizeof(float) * brain.outputLayerBias.size());

    outputFile.write(reinterpret_cast<const char*>(brain.inputLayerWeightsMask.data()), sizeof(float) * brain.inputLayerWeightsMask.rows() * brain.inputLayerWeightsMask.cols());
    outputFile.write(reinterpret_cast<const char*>(brain.inputLayerBiasMask.data()), sizeof(float) * brain.inputLayerBiasMask.size());
    outputFile.write(reinterpret_cast<const char*>(brain.outputLayerWeightsMask.data()), sizeof(float) * brain.outputLayerWeightsMask.rows() * brain.outputLayerWeightsMask.cols());
    outputFile.write(reinterpret_cast<const char*>(brain.outputLayerBiasMask.data()), sizeof(float) * brain.outputLayerBiasMask.size());

    outputFile.write(reinterpret_cast<const char*>(brain.activationPerHidden.data()), sizeof(float) * brain.activationPerHidden.size());

    // write genetics
    int geneCount = te.genetics.phenotype.size();
    outputFile.write(reinterpret_cast<const char*>(&geneCount), sizeof(int));
    for (auto& entry : te.genetics.phenotype) {
        std::string name = entry.first;
        Gene* gene = entry.second;

        size_t gene_nameSize = name.size();
        outputFile.write(reinterpret_cast<const char*>(&gene_nameSize), sizeof(size_t));
        outputFile.write(reinterpret_cast<const char*>(name.c_str()), gene_nameSize);

        outputFile.write(reinterpret_cast<const char*>(&gene->value), sizeof(float));
        outputFile.write(reinterpret_cast<const char*>(&gene->min), sizeof(float));
        outputFile.write(reinterpret_cast<const char*>(&gene->max), sizeof(float));
        outputFile.write(reinterpret_cast<const char*>(&gene->step), sizeof(float));

        //std::cout << name << " values: " << gene->value << ", step: " << gene->step << std::endl;
    }

    // write children
    if(!branch->children.empty()) {

        for (int i = 0; i < children; i++) {
            writeTrunk(outputFile, branch->children[i]);
        }

    }

}

void Ecosystem::setFocusedEntity(EnvironmentBody* toFocus) {
    if (focusedEntity != nullptr)
        focusedEntity->focused = false;
    focusedEntity = toFocus;
    if (focusedEntity != nullptr)
    toFocus->focused = true;
}


void Ecosystem::framebuffer_size_callback(GLFWwindow* window, int width, int height) {
    // update renderer viewport size
    renderer.viewportSize.x = width;
    renderer.viewportSize.y = height;
    glViewport(0, 0, width, height);
}

void Ecosystem::scroll_callback(GLFWwindow* window, double xoffset, double yoffset) {
    ImGuiIO& io = ImGui::GetIO();
    //ImGui_ImplGlfw_ScrollCallback(window, xoffset, yoffset);
    if (io.WantCaptureMouse) {
        return;
    }

    double xpos;
    double ypos;
    glfwGetCursorPos(window, &xpos, &ypos);

    if (!isDragging) {
        float mx = xpos;
        float my = ypos;


        b2Vec2 worldCoords = pixelToWorldCoordinates(mx, my);
        // calculate world coordinates from mouse coordinates
        float oldWorldX = worldCoords.x;
        float oldWorldY = worldCoords.y;

        if (yoffset > 0.001) {
            scaleStep += 1;
        }
        else if (yoffset < -0.001) {
            scaleStep -= 1;
        }

    }
}

void Ecosystem::cursor_position_callback(GLFWwindow* window, double xpos, double ypos) {

    ImGuiIO& io = ImGui::GetIO();
    double mouse_x, mouse_y;
    glfwGetCursorPos(window, &mouse_x, &mouse_y);
    io.MousePos = ImVec2((float)mouse_x, (float)mouse_y);

    if (io.WantCaptureMouse) {
        return;
    }

    if (isDragging) {
        // stop focusing on entity
        setFocusedEntity(nullptr);

        float mouseX = xpos;
        float mouseY = ypos;

        // Calculate the current world position under the mouse cursor
        b2Vec2 currentWorldCoords = pixelToWorldCoordinates(mouseX, mouseY);

        // Calculate the offset change to keep the original world position under the mouse cursor
        float offsetXChange = currentWorldCoords.x - originalWorldCoords.x;
        float offsetYChange = currentWorldCoords.y - originalWorldCoords.y;

        float scale = getScale();
        offsetX = originalOffset.x - offsetXChange;
        offsetY = originalOffset.y - offsetYChange;

        dragStart.x = xpos;
        dragStart.y = ypos;
        originalWorldCoords = pixelToWorldCoordinates(xpos, ypos);
        originalOffset.x = offsetX;
        originalOffset.y = offsetY;
    }
}

void Ecosystem::key_callback(GLFWwindow* window, int key, int scancode, int action, int mods) {
    ImGuiIO& io = ImGui::GetIO();
    //ImGui_ImplGlfw_KeyCallback(window, key, scancode, action, mods);

    if (io.WantCaptureKeyboard) {
        return;
    }

    double xpos;
    double ypos;
    glfwGetCursorPos(window, &xpos, &ypos);
    b2Vec2 mousePosition = b2Vec2(xpos, ypos);

    if (key == GLFW_KEY_ESCAPE) {
        // if control is being pressed
        if ((mods | GLFW_MOD_SHIFT) == mods) {
            shouldRun = false;
            shouldSave = false;
        }
    }
    if (key == GLFW_KEY_W) {
        offsetY -= 50;
        setFocusedEntity(nullptr);
    }
    if (key == GLFW_KEY_A) {
        offsetX -= 50;
        setFocusedEntity(nullptr);
    }
    if (key == GLFW_KEY_S) {
        offsetY += 50;
        setFocusedEntity(nullptr);
    }
    if (key == GLFW_KEY_D) {
        offsetX += 50;
        setFocusedEntity(nullptr);
    }
    if (key == GLFW_KEY_Q) {
        if (action == GLFW_PRESS) {
            showWorldPanel = !showWorldPanel;
        }
    }

    if (key == GLFW_KEY_R) {
        if (action == GLFW_PRESS) {
            setFocusedEntity(entitiesList[rand() % entitiesList.size()]);
        }
    }
    if (key == GLFW_KEY_F11) {
        if (action == GLFW_PRESS) {
            setFullscreen(!isFullscreen);
        }
    }
    if (key == GLFW_KEY_SPACE) {

        if (focusedEntity == nullptr) {
            offsetX = 0;
            offsetY = 0;
            scaleStep = 0;
        }
        else {
            if (focusedEntity != nullptr)
                focusedEntity->focused = false;
            //just stop tracking entity
            focusedEntity = nullptr;
        }

    }
    if (key == GLFW_KEY_B) {
        if (action == GLFW_PRESS) { 
            border.shouldRenderBorders = !border.shouldRenderBorders;
        }
    }
    if (key == GLFW_KEY_P) {

        if (action == GLFW_PRESS) {
            b2Vec2 w = pixelToWorldCoordinates(mousePosition.x, mousePosition.y);
            if (isInWorldBounds(w.x, w.y)) {
                std::cout << "Placing plant at " << w.x << ", " << w.y << std::endl;
                PlantEntity* plant = new PlantEntity(world, w.x, w.y);
                plant->origin = "spawned";
                addEntity(plant);
            }
        }
    }
    if (key == GLFW_KEY_I) {
        if (action == GLFW_PRESS) {
            b2Vec2 w = pixelToWorldCoordinates(mousePosition.x, mousePosition.y);

            EnvironmentBody* focused = getClosestTo(w.x, w.y, 32, nullptr);

            if (focused != nullptr) {
                setFocusedEntity(focused);
            }
        }
    }

    if (key == GLFW_KEY_UP) {
        speedUpRate = 10;
        std::cout << "SpeedUp: " << speedUpRate << std::endl;
    }
    if (key == GLFW_KEY_DOWN) {
        speedUpRate = 1;
        std::cout << "SpeedUp: " << speedUpRate << std::endl;
    }
    if (action == GLFW_PRESS) {
        if (key == GLFW_KEY_LEFT) {
            if (speedUpRate <= 1) {
                speedUpRate = speedUpRate / 2;
            }
            else {
                speedUpRate -= 1;
            }
            std::cout << "SpeedUp: " << speedUpRate << std::endl;
        }
        if (key == GLFW_KEY_RIGHT) {
            if (speedUpRate < 1) {
                speedUpRate = speedUpRate * 2;
            }
            else {
                speedUpRate += 1;
            }
            std::cout << "SpeedUp: " << speedUpRate << std::endl;
        }
    }
}

void Ecosystem::mouse_button_callback(GLFWwindow* window, int button, int action, int mods) {
    ImGuiIO& io = ImGui::GetIO();
    //ImGui_ImplGlfw_MouseButtonCallback(window, button, action, mods);

    if (io.WantCaptureMouse) {
        return;
    }

    double xpos;
    double ypos;
    glfwGetCursorPos(window, &xpos, &ypos);
    b2Vec2 mousePosition = b2Vec2(xpos, ypos);


    if (action == GLFW_PRESS) {
        if (button == GLFW_MOUSE_BUTTON_1) {
            b2Vec2 w = pixelToWorldCoordinates(mousePosition.x, mousePosition.y);

            EnvironmentBody* focused = getClosestTo(w.x, w.y, 32, nullptr);

            if (focused != nullptr) {
                setFocusedEntity(focused);
            }
        }
    }
    if (button == GLFW_MOUSE_BUTTON_2) {
        if (action == GLFW_PRESS) {
            isDragging = true;
            dragStart.x = xpos;
            dragStart.y = ypos;
            originalWorldCoords = pixelToWorldCoordinates(xpos, ypos);
            originalOffset.x = offsetX;
            originalOffset.y = offsetY;
        }
        else if (action == GLFW_RELEASE) {
            isDragging = false;
        }
    }
}

void Ecosystem::close_window_callback(GLFWwindow* window) {
    shouldRun = false;
}

void GLFWCallbacks::mouse_button_callback(GLFWwindow* window, int button, int action, int mods) {
    Ecosystem* ecosystem = static_cast<Ecosystem*>(glfwGetWindowUserPointer(window));
    ecosystem->mouse_button_callback(window, button, action, mods);
}

void GLFWCallbacks::key_callback(GLFWwindow* window, int key, int scancode, int action, int mods) {
    Ecosystem* ecosystem = static_cast<Ecosystem*>(glfwGetWindowUserPointer(window));
    ecosystem->key_callback(window, key, scancode, action, mods);
}
void GLFWCallbacks::cursor_position_callback(GLFWwindow* window, double xpos, double ypos) {
    Ecosystem* ecosystem = static_cast<Ecosystem*>(glfwGetWindowUserPointer(window));
    ecosystem->cursor_position_callback(window, xpos,  ypos);
}
void GLFWCallbacks::framebuffer_size_callback(GLFWwindow* window, int width, int height) {
    Ecosystem* ecosystem = static_cast<Ecosystem*>(glfwGetWindowUserPointer(window));
    ecosystem->framebuffer_size_callback(window, width, height);
}
void GLFWCallbacks::scroll_callback(GLFWwindow* window, double xoffset, double yoffset) {
    Ecosystem* ecosystem = static_cast<Ecosystem*>(glfwGetWindowUserPointer(window));
    ecosystem->scroll_callback(window, xoffset, yoffset);
}
void GLFWCallbacks::close_window_callback(GLFWwindow* window) {
    Ecosystem* ecosystem = static_cast<Ecosystem*>(glfwGetWindowUserPointer(window));
    ecosystem->close_window_callback(window);
}
void GLFWCallbacks::glfw_error_callback(int error, const char* description) {
    fprintf(stderr, "GLFW Error %d: %s\n", error, description);
}