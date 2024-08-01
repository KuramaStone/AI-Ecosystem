#pragma once

#include <vector>
#include <iomanip>
#include <iostream>
#include "SmoothNoise.h"
#include "SpatialHash.h"
#include "glm/glm.hpp"
#include <omp.h>

const float B_PI = 3.14159265f;
const std::string endl = "\n";

class RectangleBorders {
public:
	float x;
	float y;
	float width;
	float height;
	RectangleBorders(float x, float y, float w, float h);
};

class CaveLine : public HashedPosition {
public:
	float x1;
	float y1;
	float x2;
	float y2;
	CaveLine(float x1, float y1, float x2, float y2);
	virtual b2Vec2 getHashingPosition();
	virtual float getHashSize();
};

class Triangle {
public:
	b2Vec3 a;
	b2Vec3 b;
	b2Vec3 c;
	Triangle(b2Vec3 a, b2Vec3 b, b2Vec3 c);
};

class LineOptimizer {
public:
	static int areEndpointsEqual(CaveLine* line1, CaveLine* line2);
	static std::vector<CaveLine*> vwOptimization(std::vector<CaveLine*>& lines, float areaThreshold);
	static std::vector<CaveLine*> mergeRedundantLines(std::vector<CaveLine*>& lines, float angleThreshold);
	static float calculatePolygonArea(std::vector<b2Vec2>& vertices);
private:
	static float calculateTriangleArea(std::vector<b2Vec2>& vertices);
	static void mergeLines(CaveLine* line1, CaveLine* line2, bool flip);
};

class CaveOutline {
public:
	SmoothNoise noise;
	int seed;
	const int layers = 2;
	const float exponent = 2.0;
	const float persistence = 0.5;
	float threshold = 0.05;
	float areaThreshold = 10;
	float renderHeightScale = 50;
	float shadowMapScale = 4.0f;
	int compressionIterations = 8;
	float meshShadowsBiasValue = 0.006;
	float minMeshShadowsBiasValue = 0.0;

	int stepWidth = 1;
	int stepHeight = 1;

	float borderX;
	float borderY;
	float width; 
	float height;
	float step;
	float modelStep;
	float borderModifier;

	std::vector<CaveLine*> lines;
	std::vector<RectangleBorders*> chainBoxes;
	SpatialHash lineHashing;
	int outlineChainIndex = -1;
	
	unsigned char* validSpawningPositions; // array of objects stored as [w*h]
	unsigned int noiseTexture;


	bool continuousErosion = false;
	int totalDroplets = 1200000;
	int maxDropletLifetime = 65;
	float erodeSpeed = 0.080;
	bool doErosion = true;
	bool doDeposition = true;
	float depositSpeed = 0.001;
	float gravity = 10;
	float initialWaterVolume = 1;
	float initialSpeed = 10;
	float evaporateSpeed = 0.01f;
	float sedimentCapacityFactor = 2.053;
	float minSedimentCapacity = 0.00005;
	int brushRadius = 4;
	float inertia = 0.91f;
	float windSpeed = 0.00000;
	float windAngle = 0.6;
	float sandHardness = 1.0;
	float stoneHardness = 4.0;

	int erosionCounts = 0;
	std::vector<float> brush;


	CaveOutline(int seed, float x2, float y2, float w, float h, float noiseStep, float modelStep, float borderStep);
	void build();
	void buildShape();
	void buildChain();
	float getNoiseLevel(float x, float y);
	bool isInWalls(float x, float y);
	void erosion();
	void buildBrush();

	bool generateTexture = true;
	std::vector<float> textureData;
	std::vector<float> originalTextureData;
	std::vector<float> triangles;
	std::vector<unsigned int> indices;
	std::vector<float> flatTriangles;
	int indicesCount;
	std::vector<unsigned int> flatIndices;
	std::vector<std::vector<CaveLine*>> chains;
	int toStepIndex(float x, float y);

private:
	BRandom rndEngine;

	int getState(bool a, bool b, bool c, bool d);
	CaveLine* lineFrom(b2Vec2 a, b2Vec2 b);
	glm::vec3 calculateHeightAndGradient(std::vector<float>& map, float x, float y);
};