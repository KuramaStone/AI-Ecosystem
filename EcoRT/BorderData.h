#pragma once

#include <box2d/box2d.h>
#include <iostream>
#include "Borders.h"

class Border {
public:
	b2Body* body = nullptr;
	b2Vec2 center;
	b2Vec2 bounds[2];
	b2Vec2 size;
	CaveOutline* caveWalls = nullptr;
	b2Body* caveBody = nullptr;
	bool shouldRenderBorders = false;
	float step = 1;
	int seed;
	float totalInaccessibleWorldArea = 0.0f;

	Border();
};