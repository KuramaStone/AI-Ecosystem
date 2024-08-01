#pragma once

#include <vector>
#include "Eigen/Dense"
#include <iostream>
#include <box2d/box2d.h>

class EntityBite {
public:
	float stomachAcidity;
	float proteinComplexity, sugarComplexity;
	float remainingProtein;
	float remainingSugar;
	float digestionSteps = 0;
	EntityBite(float acidity, float proteinComplexity, float sugarComplexity, float protein, float sugar);

	b2Vec2 digest(float multiplier);
};

class Stomach {
public:
	float maxSize = 100.0f;
	float acidity = 1.0f; 
	std::vector<EntityBite*> contents = std::vector<EntityBite*>();
	bool canFit(float total);

	void addBite(float proteinComplexity, float sugarComplexity, float protein, float sugar);
	b2Vec2 digestion(float multiplier);
};