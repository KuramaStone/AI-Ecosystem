#include "MyRandom.h"

BRandom::BRandom() {
	seed = generateSeed();
	e2 = RandomEngine(rd());
	e2.seed(seed);
	uniform_float = std::uniform_real_distribution<float>(0, 0.99999);
}

void BRandom::verify() {
	int originalSeed = seed;
	setSeed(1234);

	const int numValues = 100000000;
	const int buckets = 1024;
	std::cout << "Testing " << numValues << " for statistical variation using " << buckets << " buckets." << std::endl;

	std::vector<int> occurences(buckets);

	// generate X floats, put them into their bucket
	for (int i = 0; i < numValues; ++i) {
		float value = randomFloat();
		int bucketID = int(floor(value * buckets));

		occurences[bucketID]++;
	}

	// check how smooth all buckets are
	int average = 0;

	for (int j = 0; j < buckets; ++j) {
		average += occurences[j];
	}
	average /= buckets;

	float avgVariation = 0;
	int maxVariation = 0;

	int sqSize = sqrt(buckets);
	std::vector<int> deltaVariations(buckets);
	for (int j = 0; j < buckets; ++j) {
		int size = occurences[j];
		int variation = size - average;
		deltaVariations[j] = variation;

		if (variation > maxVariation) {
			maxVariation = variation;
		}

		avgVariation += variation;

		if ((j % sqSize == 0 && j != 0)) {
			std::cout << std::endl;
		}
		// print into 2x2 grid
		std::cout << variation << " ";
	}
	std::cout << std::endl;
	avgVariation = avgVariation / buckets;


	std::sort(deltaVariations.begin(), deltaVariations.end()); // sort it
	int medianVariation = deltaVariations[int(buckets / 2)];

	

	std::cout << "Average variation: " << avgVariation << std::endl;
	std::cout << "Max variation: " << maxVariation << std::endl;
	std::cout << "Median variation: " << medianVariation << std::endl;


	setSeed(originalSeed);
}

int BRandom::generateSeed() {

	// Get time-based seed
	auto timeSeed = static_cast<uint_fast32_t>(std::chrono::steady_clock::now().time_since_epoch().count());

	// Use random device for additional entropy
	std::random_device rd;
	auto randomSeed = rd();

	// Combine both seeds to create a more unique seed
	return timeSeed ^ randomSeed;
}

void BRandom::setSeed(int newseed) {
	seed = newseed;

	e2.seed(seed);
}

float BRandom::randomFloat() {
	return uniform_float(e2);
}

int BRandom::randomInt() {
	return static_cast<int>(randomFloat() * RAND_MAX);
}

// inclusive min, exclusive max
int BRandom::randomInt(int min, int max) {
	// will never choose max as randomFloat() never produces 1.0 so it will round down to max-1
	return min + static_cast<int>(randomFloat() * (max - min));
}