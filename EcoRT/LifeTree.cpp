#include "LifeTree.h"

TreeEntry::TreeEntry(CellBrain& brain2, Genetics& genetics2, 
	float worldTotalStepAtCreation2, b2Vec3 color2, int branchIndex2) : 
	worldTotalStepAtCreation(worldTotalStepAtCreation2), 
	color(color2), branchIndex(branchIndex2) {
	brain.setValuesFrom(brain2);
	brain.build(); // have to build brain after setting
	genetics.copy(genetics2);
}

TreeBranch::TreeBranch(std::string speciesName, TreeBranch* parentBranch, TreeEntry& branchData2)
	: name(speciesName), parent(parentBranch) {
	// override branch data
	branchData.brain.setValuesFrom(branchData2.brain);
	branchData.genetics.copy(branchData2.genetics);
	branchData.branchIndex = branchData2.branchIndex;
	branchData.color = b2Vec3(branchData2.color);
	branchData.worldTotalStepAtCreation = branchData2.worldTotalStepAtCreation;

	branchData.brain.build();
}

void TreeOfLife::trim(std::map<int, b2Vec3>& extantBranches) {
	// delete all branches that are extinct unless they have extant children

	
	for (int i = 0; i < allBranches.size(); i++) {
		TreeBranch* branch = allBranches[i];
		if (branch == trunk)
			continue;

		// deleted branches set to nullptr, this should ignore deleted branches.
		if (branch == nullptr)
			continue;

		bool survives = doesLineageSurvive(branch, extantBranches);

		if (!survives) {
			// set branch to nullptr. we need to keep track of its index, but not its data.
			// deletes all children and sets their index to nullptr

			// delete branch from parent
			std::vector<TreeBranch*>& parentChildren = branch->parent->children;
			parentChildren.erase(std::find(parentChildren.begin(), parentChildren.end(), branch));

			deleteLineage(branch);

		}

	}

	int maxDepth = 1;
	for (TreeBranch* branch : allBranches) {
		if (branch != nullptr) {
			if (maxDepth < branch->depth) {
				maxDepth = branch->depth;
			}
		}
	}

	deepestChain = maxDepth;
}

void TreeOfLife::deleteLineage(TreeBranch* branch) {


	if (!branch->children.empty()) {

		int children = branch->children.size();
		for (int i = 0; i < children; i++) {
			deleteLineage(branch->children[i]);
		}

	}

	allBranches[branch->branchData.branchIndex] = nullptr;
	delete branch;
}

bool TreeOfLife::doesLineageSurvive(TreeBranch* branch, std::map<int, b2Vec3>& extantBranches) {
	if (branch == nullptr)
		return false;

	// lineage survives by default if it is still alive
	bool lineageSurvives = extantBranches.find(branch->branchData.branchIndex) != extantBranches.end();

	// lineage can also survive if a child down the line is alive
	if (!branch->children.empty()) {
		
		int children = branch->children.size();
		for (int i = 0; i < children; i++) {
			lineageSurvives = lineageSurvives || doesLineageSurvive(branch->children[i], extantBranches);
		}

	}

	return lineageSurvives;
}

TreeOfLife::TreeOfLife() {

	latinGen.init();
}


void TreeOfLife::setFirstSpecies(TreeEntry& proto) {
	allBranches.clear();
	deepestChain = 0;

	trunk = new TreeBranch("prima vita", nullptr, proto);
	
	allBranches.push_back(trunk);
}

// 0 if identical, higher values mean more different
float TreeOfLife::getDistanceToSpecies(TreeBranch* branch, TreeEntry& individual) {
	if (branch == nullptr) {
		std::cerr << "Can't get distance to nullptr branch" << std::endl;
		throw std::runtime_error("branch is nullptr");
	}

	float weightDifference = 0.0;
	float maskDifference = 0.0;
	float activationDifference = 0.0;

	CellBrain& pbrain = branch->branchData.brain;
	Genetics& pgenes = branch->branchData.genetics;
	CellBrain& obrain = individual.brain;
	Genetics& ogenes = individual.genetics;

	int totalWeights = pbrain.totalWeights;
	int activeWeights = std::max(pbrain.activeWeights, obrain.activeWeights);

	float geneDifference = 0;
	for (auto& entry : pgenes.phenotype) {
		geneDifference += abs(entry.second->value - ogenes.phenotype[entry.first]->value);
	}

	// performs a simple or operation and gets the sum. provides the number of unique masks
	int actualMaskCount =
		(pbrain.inputLayerWeightsMask + obrain.inputLayerWeightsMask).cwiseMax(1).sum() +
		(pbrain.outputLayerWeightsMask + obrain.outputLayerWeightsMask).cwiseMax(1).sum() +

		(pbrain.inputLayerBiasMask + obrain.inputLayerBiasMask).cwiseMax(1).sum() +
		(pbrain.outputLayerBiasMask + obrain.outputLayerBiasMask).cwiseMax(1).sum();

	weightDifference =
		(pbrain.actual_inputLayerWeights - obrain.actual_inputLayerWeights).cwiseAbs().sum() +
		(pbrain.actual_outputLayerWeights - obrain.actual_outputLayerWeights).cwiseAbs().sum() +

		(pbrain.actual_inputLayerBias - obrain.actual_inputLayerBias).cwiseAbs().sum() +
		(pbrain.actual_outputLayerBias - obrain.actual_outputLayerBias).cwiseAbs().sum();

	maskDifference =
		(pbrain.inputLayerWeightsMask - obrain.inputLayerWeightsMask).cwiseAbs().sum() +
		(pbrain.outputLayerWeightsMask - obrain.outputLayerWeightsMask).cwiseAbs().sum() +

		(pbrain.inputLayerBiasMask - obrain.inputLayerBiasMask).cwiseAbs().sum() +
		(pbrain.outputLayerBiasMask - obrain.outputLayerBiasMask).cwiseAbs().sum();

	for (int i = 0; i < pbrain.h1_size; i++) {
		activationDifference += (pbrain.activationPerHidden[i] == obrain.activationPerHidden[i]) ? 0 : 1;
	}

	//average values
	weightDifference /= activeWeights;
	maskDifference /= actualMaskCount;
	activationDifference /= pbrain.h1_size;
	geneDifference /= pgenes.phenotype.size();

	float a = 0.4; // weight
	float b = 7.0; // mask
	float c = 0.1; // activations
	float d = 0.3; // gene
	//std::cout << "Sp dist: " << weightDifference << " " << maskDifference << " " << activationDifference << " " << geneDifference << std::endl;

	float value = a * weightDifference + b * maskDifference + c * activationDifference + d * geneDifference;
	//std::cout << "Sp dist: " << value << std::endl;

	return value;
}

// returns true branch index of individual
int TreeOfLife::add(TreeEntry& individual) {
	DistanceResult result = getDistanceToNearestSpecies(individual);

	if (result.distance >= speciationThreshold) {
		// unique enough to have its own branch
		TreeBranch* parent = result.closestBranch; // closest branch is parent

		int newIndex = allBranches.size();
		individual.branchIndex = newIndex; // update index
		TreeBranch* newBranch = new TreeBranch(latinGen.getRandomBinomial(), parent, individual);
		newBranch->depth = parent->depth + 1;

		// pop it into the main branch line and onto the child list
		parent->children.push_back(newBranch);
		allBranches.push_back(newBranch);

		if (deepestChain < newBranch->depth) {
			deepestChain = newBranch->depth;
		}

		return newIndex;
	}
	else {
		// has a close relative. doesn't need a new branch
		return result.branchIndex;
	}
}


DistanceResult TreeOfLife::getDistanceToNearestSpecies(TreeEntry& individual) {

	TreeBranch* closestBranch = nullptr;
	float minDist = -1;
	int branchIndex = -1;

	// search around branch index, until all are searched.
	int maxSize = allBranches.size();
	int searchStart = std::max(0, individual.branchIndex); // may be -1 if first generation, just search everything.
	int searchesLeft = maxSize;
	int searchIndex = 0;

	while (searchesLeft > 0) {

		// set to 0 for only positives. this prevents child branches from confusing their lineage with older ones.
		for (int i = 0; i <= 0; i++) {

			// get index to search
			int f = (i == 0 ? 1 : -1);
			int j = searchStart + f * searchIndex;

			if (j >= 0 && j < maxSize) {
				TreeBranch* branch = allBranches[j];
				if (branch != nullptr) {
					float dist = getDistanceToSpecies(branch, individual);
					if (minDist < 0 || dist < minDist) {
						minDist = dist;
						closestBranch = branch;
						branchIndex = j;
					}
				}

			}

			searchesLeft--;
		}

		searchIndex++;
	}

	DistanceResult result;
	result.closestBranch = closestBranch;
	result.distance = minDist;
	result.branchIndex = branchIndex;

	return result;
}

//////////////////
// LatinNameGen //
//////////////////

void LatinNameGen::init() {

	// Open the file
	std::ifstream inputFile("LatinNames.txt");

	// Check if the file is open
	if (!inputFile.is_open()) {
		std::string currentDirectory = std::filesystem::current_path().string();

		// Print the current working directory
		std::cout << "Current Working Directory: " << currentDirectory << std::endl;

		std::cerr << "Error opening file LatinNames.txt" << std::endl;
		return;  // Exit with an error code
	}

	// Read names from the file into the array
	int i = 0;
	std::string line;
	while (i < 1000 && std::getline(inputFile, line)) {
		words[i] = line;
		i++;
	}

	std::cout << "Loaded " << (i) << " latin names." << std::endl;

	// Close the file
	inputFile.close();

}

std::string LatinNameGen::getRandomBinomial() {
	std::string first = words[rndEngine.randomInt(0, 1000)];
	std::string second = words[rndEngine.randomInt(0, 1000)];

	std::string binomial = first + " " + second;
	binomial[0] = std::toupper(binomial[0]); // capitalize first letter

	return binomial;
}