#pragma once

#include <cmath>
#include <box2d/box2d.h>
#include <iostream>
#include <time.h>
#include "MyRandom.h"


class SmoothNoise {
private:
	int seed;

	b2Vec3 floor(const b2Vec3& a);
	float dot(const b2Vec3& a, const b2Vec3& b);
	float mix(float x, float y, float a);
	float fract(float p);
	float hash(b2Vec3 p);
	float achnoise(b2Vec3 x);
	float snoise(b2Vec3 p);
	float smoothstep(float edge0, float edge1, float x);
public:
	SmoothNoise();
	void setSeed(int seed);
	float getSmoothNoiseAt(float x, float y, float z, int layers, float periodExp, float persistence);
	BRandom rndEngine;
};