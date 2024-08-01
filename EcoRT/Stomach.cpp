#include "Stomach.h"

EntityBite::EntityBite(float acidity, float proteinComplexity, float sugarComplexity, float protein, float sugar) :
    stomachAcidity(acidity),
    proteinComplexity(proteinComplexity), sugarComplexity(sugarComplexity),
    remainingProtein(protein), remainingSugar(sugar) {

}

b2Vec2 EntityBite::digest(float multiplier) {
    digestionSteps++; // first step should be 1

    // Increases in efficiency per step.
    // Low complexity=0.0 means 100% effiency always
    // High complexity=(any positive value) means it reaches 100% efficiency slower.

    float proteinToDigest = std::min(stomachAcidity * multiplier, remainingProtein);
    float proteinEfficiency = 1 - exp(-(digestionSteps / 400.0f) / std::max(0.005f, proteinComplexity));
    remainingProtein -= proteinToDigest;
    float proteinToIngest = proteinToDigest * proteinEfficiency;

    float sugarToDigest = std::min(stomachAcidity * multiplier, remainingSugar);
    float sugarEfficiency = 1 - exp(-(digestionSteps / 400) / std::max(0.005f, sugarComplexity));
    remainingSugar -= sugarToDigest;
    float sugarToIngest = sugarToDigest * sugarEfficiency;


    b2Vec2 results = b2Vec2(proteinToIngest, sugarToIngest);

    return results;
}

void Stomach::addBite(float proteinComplexity, float sugarComplexity, float protein, float sugar) {
    contents.push_back(new EntityBite(acidity, proteinComplexity, sugarComplexity, protein, sugar));
}
bool Stomach::canFit(float toAdd) {
    float inStomach = 0;

    for (EntityBite* bite : contents) {
        inStomach += bite->remainingProtein;
        inStomach += bite->remainingSugar;
    }

    if (inStomach + toAdd <= maxSize) {
        return true;
    }

    return false;
}

b2Vec2 Stomach::digestion(float multiplier) {
    std::vector<EntityBite*> toRemove;

    b2Vec2 results = b2Vec2(0, 0);
    for (EntityBite* bite : contents) {
        results += bite->digest(multiplier / contents.size());

        if (bite->remainingProtein < 0.001 && bite->remainingSugar < 0.001) {
            toRemove.push_back(bite);
        }
    }

    // remove empty bites
    contents.erase(std::remove_if(contents.begin(), contents.end(),
        [](const EntityBite* obj) { return obj->remainingProtein < 0.001 && obj->remainingSugar < 0.001; }), contents.end());

    // delete
    for (int i = 0; i < toRemove.size(); i++) {
        delete toRemove[i];
    }

    return results;
}