#pragma once

#include <iostream>
#include <random>
#include <time.h>
#include <chrono>

#define RandomEngine std::linear_congruential_engine<uint_fast32_t, 48271, 0, 2147483647>

class BRandom {
public:
	std::random_device rd;
	RandomEngine e2;
	std::uniform_real_distribution<float> uniform_float;

	BRandom();
	float randomFloat();
	int randomInt();
	int randomInt(int min, int max);
	void setSeed(int seed);
	int generateSeed();
	void verify();

private:
	int seed;
};