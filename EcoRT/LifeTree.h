#pragma once

#include <iostream>
#include <fstream>
#include <string>
#include <filesystem>
#include <box2d/box2d.h>
#include "MyRandom.h"
#include "Genetics.h"

/*
* This is made to track speciation over time.
*/

class TreeEntry {
public:
	CellBrain brain;
	Genetics genetics;
	float worldTotalStepAtCreation = 0.0f;
	b2Vec3 color;
	int branchIndex;

	TreeEntry(CellBrain& brain2, Genetics& genetics2, float worldTotalStepAtCreation2, b2Vec3 color2, int branchIndex2);
};

class TreeBranch; // create initial reference to tree branch for DistanceResult

class DistanceResult {
public:
	TreeBranch* closestBranch; // be careful about circularity here
	float distance;
	int branchIndex;
};

// Each species has its own branch
class TreeBranch {
public:
	std::string name;
	std::vector<TreeBranch*> children = std::vector<TreeBranch*>();
	TreeBranch* parent;
	TreeEntry branchData = TreeEntry(CellBrain(), Genetics(), 0, b2Vec3(0, 0, 0), 0);
	int depth = 0;

	TreeBranch(std::string name, TreeBranch* parent, TreeEntry& branchData);
};

class LatinNameGen {
public:
	std::string words[1000];
	BRandom rndEngine;

	void init();
	std::string getRandomBinomial();
};

class TreeOfLife {
public:
	TreeBranch* trunk = nullptr; // first branch of life
	int deepestChain = 0;
	LatinNameGen latinGen;

	// species by order of creation. used for more easily finding related species
	std::vector<TreeBranch*> allBranches;

	float speciationThreshold = 0.75f;

	TreeOfLife();
	void setFirstSpecies(TreeEntry& proto);
	float getDistanceToSpecies(TreeBranch* branch, TreeEntry& individual);
	DistanceResult getDistanceToNearestSpecies(TreeEntry& individual);
	// adds this individual to the tree of life
	int add(TreeEntry& individual);
	void trim(std::map<int, b2Vec3>& extantBranches);
	bool doesLineageSurvive(TreeBranch* branch, std::map<int, b2Vec3>& extantBranches);
	void deleteLineage(TreeBranch* branch);
};