#include "Genetics.h"

// Gene types: Digestion_speed

Gene::Gene(std::string genename, float valuef, float minf, float maxf, float stepf) {
	name = genename;
	value = valuef;
	min = minf;
	max = maxf;
	step = stepf;
}

Genetics::Genetics() {
	toDefault();
}

void Genetics::mutateAll(float chance) {
	for (const auto& entry : phenotype) {
		if(rndEngine.randomFloat() < chance) {
			std::string key = entry.first;

			Gene* gene = entry.second;
			float rnd = rndEngine.randomFloat() * 2 - 1;

			float value = gene->value + rnd * gene->step;

			if (value < gene->min) {
				value = gene->min;
			}
			if (value > gene->max) {
				value = gene->max;
			}

			gene->value = value;
		}
	}
}

float Genetics::getValue(std::string name) {
	return phenotype[name]->value;
}
void Genetics::copy(Genetics& genetics) {
	phenotype.clear();

	for (const auto& entry : genetics.phenotype) {
		std::string key = entry.first;
		Gene* gene = entry.second;

		Gene* clone = new Gene(gene->name, gene->value, gene->min, gene->max, gene->step);
		addGene(clone);
	}


}

void Genetics::crossover(Genetics& other, float percent) {
	for (const auto& entry : phenotype) {
		if (random() < percent) {
			entry.second->value = other.phenotype[entry.first]->value;
		}
	}
}

void Genetics::toDefault() {
	float defAcidity = rndEngine.randomFloat() * 0.05;
	float defMaxSize = 1 + rndEngine.randomFloat() * 2;
	float defAsexuality = rndEngine.randomFloat();
	float defSpeed = 1 + rndEngine.randomFloat() * 2;

	Gene* digestionAcidity = new Gene("digestionAcidity", defAcidity, 0.0f, 10.0f, 0.005f);
	addGene(digestionAcidity);
	Gene* maxSize = new Gene("maxSize", defMaxSize, 0.0f, 100.0f, 0.1f);
	addGene(maxSize);

	// chance to breed asexually
	Gene* asexuality = new Gene("asexuality", defAsexuality, 0.0f, 1.0f, 0.05f);
	addGene(asexuality);

	float a = abs(random());
	float b = abs(random());
	float c = abs(random());
	Gene* scentA = new Gene("scentA", a, 0.0f, 1.0f, 0.1f);
	Gene* scentB = new Gene("scentB", b, 0.0f, 1.0f, 0.1f);
	Gene* scentC = new Gene("scentC", c, 0.0f, 1.0f, 0.1f);
	addGene(scentA);
	addGene(scentB);
	addGene(scentC);

	Gene* geneSpeedModifier = new Gene("geneSpeedModifier", defSpeed, 0.0f, 5.0f, 0.1f);
	addGene(geneSpeedModifier);
}

void Genetics::addGene(Gene* gene) {
	phenotype[gene->name] = gene;
}

double Genetics::random() {
	return rndEngine.randomFloat();
}

void CellBrain::test() {

	
}

CellBrain::CellBrain() {

	// Create random weight matrices and bias vectors
	inputLayerWeights = xavierInitWeightMatrix(h1_size, input_size);
	inputLayerBias = xavierInitBiasVector(h1_size);
	outputLayerWeights = xavierInitWeightMatrix(output_size, h1_size);
	outputLayerBias = xavierInitBiasVector(output_size);

	inputLayerWeightsMask = Eigen::MatrixXf::Zero(h1_size, input_size);
	inputLayerBiasMask = Eigen::VectorXf::Zero(h1_size);
	outputLayerWeightsMask = Eigen::MatrixXf::Zero(output_size, h1_size);
	outputLayerBiasMask = Eigen::VectorXf::Zero(output_size);

	totalWeights = inputLayerWeightsMask.rows() * inputLayerWeightsMask.cols() +
		outputLayerWeightsMask.rows() * outputLayerWeightsMask.cols();

	// set activations
	for (int i = 0; i < h1_size; i++) {
		activationPerHidden.push_back(rndEngine.randomInt(0, TOTAL_NEURON_TYPES));
	}

	// set default brain
	// move forward while slightly turning, if an entity is close, go faster and don't turn
	int forwardID = 0;
	int turnID = 1;
	int attackID = 2;
	int splitID = 3;
	int growID = 4;

	int inputCenterEyeGValue = 14;
	int inputCenterEyeCos = 18;
	int inputCenterEyeSin = 19;
	int inputHeartbeat = 1;
	int inputProtein = 2;
	int inputSugar = 3;

	int hiddenIndex = 0; // forward hidden
	int hiddenIndex1 = 1; // turn hidden
	int hiddenIndex2 = 2; // attack hidden
	int hiddenIndex3 = 3;
	int hiddenIndex4 = 4; // grow hidden

	// move forward when facing food
	inputLayerWeights(hiddenIndex, inputCenterEyeCos) = 1.0;
	inputLayerWeightsMask(hiddenIndex, inputCenterEyeCos) = 1.0;
	activationPerHidden[hiddenIndex] = TYPE_LINEAR;
	outputLayerWeights(forwardID, hiddenIndex) = 0.5;
	outputLayerWeightsMask(forwardID, hiddenIndex) = 1;

	// turn when not facing food
	inputLayerWeights(hiddenIndex1, inputCenterEyeSin) = 1.0;
	inputLayerWeightsMask(hiddenIndex1, inputCenterEyeSin) = 1.0;
	activationPerHidden[hiddenIndex1] = TYPE_LINEAR;
	outputLayerWeights(turnID, hiddenIndex1) = -0.1;
	outputLayerWeightsMask(turnID, hiddenIndex1) = 1;

	// grow slightly in bursts
	inputLayerWeights(hiddenIndex4, inputHeartbeat) = 1.0;
	inputLayerWeightsMask(hiddenIndex4, inputHeartbeat) = 1;
	activationPerHidden[hiddenIndex4] = TYPE_LINEAR;
	outputLayerWeights(growID, hiddenIndex4) = 0.05; // connect hidden to output
	outputLayerWeightsMask(growID, hiddenIndex4) = 1;

	// eat bias. attacks if entity is green enough
	inputLayerWeights(hiddenIndex2, inputCenterEyeGValue) = 1.0;
	inputLayerWeightsMask(hiddenIndex2, inputCenterEyeGValue) = 1;
	activationPerHidden[hiddenIndex2] = TYPE_RELU;
	outputLayerWeights(attackID, hiddenIndex2) = 2.0; // connect hidden to output
	outputLayerWeightsMask(attackID, hiddenIndex2) = 1;

	// sex bias. Will reproduce if sum of resources is over a threshold
	inputLayerWeights(hiddenIndex3, inputProtein) = 0.1;
	inputLayerWeights(hiddenIndex3, inputSugar) = 0.1;
	inputLayerBias(hiddenIndex3) = -1.0;
	outputLayerWeights(splitID, hiddenIndex3) = 1.0;
	activationPerHidden[hiddenIndex3] = TYPE_LINEAR;
	inputLayerWeightsMask(hiddenIndex3, inputProtein) = 1;
	inputLayerWeightsMask(hiddenIndex3, inputSugar) = 1;
	inputLayerBiasMask(hiddenIndex3) = 1.0;
	outputLayerWeightsMask(splitID, hiddenIndex3) = 1.0;
}

std::string CellBrain::outputName(int index) {

	if (index == 0) {
		return "Forward";
	}
	else if (index == 1) {
		return "Turn";
	}
	else if (index == 2) {
		return "Attack";
	}
	else if (index == 3) {
		return "DTF";
	}
	else if (index == 4) {
		return "Growth";
	}

	return "idk";
}

float CellBrain::activation(int type, float sum) {
	// performs an operation on the sum

	if (type == 0) {
		// linear
		return sum;
	}
	if (type == 1) {
		// sigmoid
		return 1.0f / (1.0f / exp(-sum));
	}
	if (type == 2) {
		// tanh
		return tanh(sum);
	}
	if (type == 3) {
		// relu
		return sum > 0 ? sum : 0;
	}
	if (type == 4) {
		// sin
		return sin(sum);
	}
	if (type == 5) {
		// cos
		return cos(sum);
	}
	if (type == 6) {
		// inverse
		if (sum == 0) {
			return 0;
		}

		return 1.0 / sum;
	}

	return 0;
}

std::string CellBrain::activationName(int type) {
	// performs an operation on the sum

	if (type == 0) {
		// linear
		return "Linear";
	}
	if (type == 1) {
		// sigmoid
		return "Sigmoid";
	}
	if (type == 2) {
		// tanh
		return "Tanh";
	}
	if (type == 3) {
		// relu
		return "Relu";
	}
	if (type == 4) {
		// sin
		return "Sin";
	}
	if (type == 5) {
		// cos
		return "Cos";
	}
	if (type == 6) {
		// inverse
		return "Inverse";
	}

	return "idk";
}

void CellBrain::build() {
	isBuilt = true;
	hasMutated = false;

	actual_inputLayerWeights = inputLayerWeights.cwiseProduct(inputLayerWeightsMask).sparseView();
	actual_inputLayerBias = inputLayerBias.cwiseProduct(inputLayerBiasMask).sparseView();
	actual_outputLayerWeights = outputLayerWeights.cwiseProduct(outputLayerWeightsMask).sparseView();
	actual_outputLayerBias = outputLayerBias.cwiseProduct(outputLayerBiasMask).sparseView();

	// get total number of non-zero weights
	activeWeights = actual_inputLayerWeights.nonZeros() + actual_inputLayerBias.nonZeros() +
		actual_outputLayerWeights.nonZeros() + actual_outputLayerBias.nonZeros();
}

Eigen::VectorXf CellBrain::forward(Eigen::VectorXf& input) {
	if (!isBuilt) {
		std::cerr << "Trying to forward without building brain." << std::endl;
		throw std::runtime_error("Can't forward yet.");
	}

	Eigen::VectorXf hiddenLayerOutput = (actual_inputLayerWeights * input + actual_inputLayerBias);

	// use custom activation for each
	for (int i = 0; i < h1_size; i++) {
		int type = activationPerHidden[i];
		hiddenLayerOutput(i) = activation(type, hiddenLayerOutput(i));
	}

	Eigen::VectorXf output = (actual_outputLayerWeights * hiddenLayerOutput + actual_outputLayerBias);
	output = output.array().tanh();

	return output;
}

Eigen::MatrixXf CellBrain::xavierInitWeightMatrix(int rows, int cols) {
	float scale = sqrt(6.0 / (rows + cols));
	return Eigen::MatrixXf::Random(rows, cols) * scale;
}

// Xavier initialization for a given layer's size
Eigen::VectorXf CellBrain::xavierInitBiasVector(int size) {
	return Eigen::VectorXf::Random(size);
}

void CellBrain::mutate(float shiftChance, float shiftScale, float chanceToEnable, float chanceToDisable, float neuronTypeChance) {
	hasMutated = true;

	int hiddenBiasCount = inputLayerBias.size();
	int outBiasCount = outputLayerBias.size();

	// modify weight values
	if (shiftChance != 0) {
		mutateMatrix(inputLayerWeights, inputLayerWeightsMask, shiftChance, shiftScale);
		mutateVector(inputLayerBias, inputLayerBiasMask, shiftChance, shiftScale);
		mutateMatrix(outputLayerWeights, outputLayerWeightsMask, shiftChance, shiftScale);
		mutateVector(outputLayerBias, outputLayerBiasMask, shiftChance, shiftScale);
	}

	// enable weights
	if (chanceToEnable != 0) {
		toggleMatrixIndex(inputLayerWeightsMask, chanceToEnable);
		toggleVectorIndex(inputLayerBiasMask, chanceToEnable);
		toggleMatrixIndex(outputLayerWeightsMask, chanceToEnable);
		toggleVectorIndex(outputLayerBiasMask, chanceToEnable);
	}

	// disable weights
	if (chanceToDisable != 0) {
		toggleMatrixIndex(inputLayerWeightsMask, -chanceToDisable);
		toggleVectorIndex(inputLayerBiasMask, -chanceToDisable);
		toggleMatrixIndex(outputLayerWeightsMask, -chanceToDisable);
		toggleVectorIndex(outputLayerBiasMask, -chanceToDisable);



	}

	if (neuronTypeChance != 0) {
		neuronTypeChance *= 1.0 / h1_size;
		for (int i = 0; i < h1_size; i++) {
			if (random() < neuronTypeChance) {
				activationPerHidden[i] = rndEngine.randomInt(0, TOTAL_NEURON_TYPES);
			}
		}
	}

}

// Function to mutate the matrix
void CellBrain::mutateMatrix(Eigen::MatrixXf& matrix, Eigen::MatrixXf& mask, float chance, float scale) {
	chance *= 1.0 / mask.nonZeros();

	for (int i = 0; i < matrix.rows(); i++) {
		for (int j = 0; j < matrix.cols(); j++) {
			if (random() < chance) {
				if (mask(i, j) > 0.5) { // only mutate if enabled
					matrix(i, j) += (random() * 2 - 1) * scale;
				}
			}
		}
	}
}

// Function to mutate the matrix
void CellBrain::mutateVector(Eigen::VectorXf& vector, Eigen::VectorXf& mask, float chance, float scale) {
	chance *= 1.0 / (mask.nonZeros());

	for (int i = 0; i < vector.rows(); i++) {
		if (random() < chance) {
			if(mask(i) > 0.5) { // only if enabled
				vector(i) += (random() * 2 - 1) * scale;
			}
		}
	}
}


// Function to mutate the matrix
// positive chance can enable connections. Negative chance disables them.
void CellBrain::toggleMatrix(Eigen::MatrixXf& matrix, float chance) {
	bool toggleMode = chance >= 0;
	chance = abs(chance);
	chance *= 1.0 / (matrix.rows() * matrix.cols());

	for (int i = 0; i < matrix.rows(); i++) {
		for (int j = 0; j < matrix.cols(); j++) {
			if (random() < chance) {
				matrix(i, j) = toggleMode ? 1 : 0;
			}
		}
	}
}

// Function to mutate the matrix
void CellBrain::toggleVector(Eigen::VectorXf& vector, float chance) {
	bool toggleMode = chance >= 0;
	chance = abs(chance);
	chance *= 1.0 / (vector.size());

	for (int i = 0; i < vector.rows(); i++) {
		if (random() < chance) {
			vector(i) = toggleMode ? 1 : 0;
		}
	}
}

// Function to mutate the matrix
// If random is under chance, select a single index.
void CellBrain::toggleMatrixIndex(Eigen::MatrixXf& matrix, float chance) {
	bool toggleMode = chance >= 0;
	chance = abs(chance);


	if (random() < chance) {
		int index = rndEngine.randomInt(0, matrix.rows() * matrix.cols());
		matrix(index % matrix.rows(), index / matrix.rows()) = toggleMode ? 1 : 0;
	}
}

// Function to mutate the matrix
void CellBrain::toggleVectorIndex(Eigen::VectorXf& vector, float chance) {
	bool toggleMode = chance >= 0;
	chance = abs(chance);


	if (random() < chance) {
		int index = rndEngine.randomInt(0, vector.size());
		vector(index) = toggleMode ? 1 : 0;
	}
}

float CellBrain::random() {
	return rndEngine.randomFloat();
}

// returns the average distance per weight and mask.
float CellBrain::distanceFrom(CellBrain& other) {
	float distance = 0.0f;

	for (int i = 0; i < inputLayerWeights.rows(); i++) {
		for (int j = 0; j < inputLayerWeights.cols(); j++) {
			distance += abs(inputLayerWeights(i, j) - other.inputLayerWeights(i, j));
		}
	}
	for (int i = 0; i < outputLayerWeights.rows(); i++) {
		for (int j = 0; j < outputLayerWeights.cols(); j++) {
			distance += abs(outputLayerWeights(i, j) - other.outputLayerWeights(i, j));
		}
	}
	for (int i = 0; i < inputLayerWeightsMask.rows(); i++) {
		for (int j = 0; j < inputLayerWeightsMask.cols(); j++) {
			distance += abs(inputLayerWeightsMask(i, j) - other.inputLayerWeightsMask(i, j));
		}
	}
	for (int i = 0; i < outputLayerWeightsMask.rows(); i++) {
		for (int j = 0; j < outputLayerWeightsMask.cols(); j++) {
			distance += abs(outputLayerWeightsMask(i, j) - other.outputLayerWeightsMask(i, j));
		}
	}

	for (int i = 0; i < inputLayerBias.rows(); i++) {
		distance += abs(inputLayerBias(i) - other.inputLayerBias(i));
	}
	for (int i = 0; i < outputLayerBias.rows(); i++) {
		distance += abs(outputLayerBias(i) - other.outputLayerBias(i));
	}

	for (int i = 0; i < inputLayerBiasMask.rows(); i++) {
		distance += abs(inputLayerBiasMask(i) - other.inputLayerBiasMask(i));
	}
	for (int i = 0; i < outputLayerBias.rows(); i++) {
		distance += abs(outputLayerBiasMask(i) - other.outputLayerBiasMask(i));
	}


	// totalWeights * 2 because there is a mask for each weight.
	return distance / ((totalWeights + h1_size + output_size) * 2);
}

// percent is the chance that the other brain will be used for that index.
void CellBrain::crossover(CellBrain& other, float percent) {

	// crossover weights
	crossoverMatrix(inputLayerWeights, other.inputLayerWeights, percent);
	crossoverMatrix(outputLayerWeights, other.outputLayerWeights, percent);
	crossoverMatrix(inputLayerWeightsMask, other.inputLayerWeightsMask, percent);
	crossoverMatrix(outputLayerWeightsMask, other.outputLayerWeightsMask, percent);

	// cross over bias
	crossoverVector(inputLayerBias, other.inputLayerBias, percent);
	crossoverVector(outputLayerBias, other.outputLayerBias, percent);
	crossoverVector(inputLayerBiasMask, other.inputLayerBiasMask, percent);
	crossoverVector(outputLayerBiasMask, other.outputLayerBiasMask, percent);

	// cross over activations
	for (int i = 0; i < h1_size; i++) {
		if (random() < percent) {
			activationPerHidden[i] = other.activationPerHidden[i];
		}
	}
}

// Function to mutate the matrix
void CellBrain::crossoverMatrix(Eigen::MatrixXf& matrixA, Eigen::MatrixXf& matrixB, float percent) {

	for (int i = 0; i < matrixA.rows(); i++) {
		for (int j = 0; j < matrixA.cols(); j++) {
			if (random() < percent) {
				matrixA(i, j) = matrixB(i, j);
			}
		}
	}
}

void CellBrain::crossoverVector(Eigen::VectorXf& vectorA, Eigen::VectorXf& vectorB, float percent) {

	for (int i = 0; i < vectorA.rows(); i++) {
		if (random() < percent) {
			vectorA(i) = vectorB(i);
		}
	}

}

void CellBrain::setValuesFrom(CellBrain& other) {
	inputLayerWeights = Eigen::MatrixXf(other.inputLayerWeights);
	inputLayerBias = Eigen::VectorXf(other.inputLayerBias);
	outputLayerWeights = Eigen::MatrixXf(other.outputLayerWeights);
	outputLayerBias = Eigen::VectorXf(other.outputLayerBias);

	inputLayerWeightsMask = Eigen::MatrixXf(other.inputLayerWeightsMask);
	inputLayerBiasMask = Eigen::VectorXf(other.inputLayerBiasMask);
	outputLayerWeightsMask = Eigen::MatrixXf(other.outputLayerWeightsMask);
	outputLayerBiasMask = Eigen::VectorXf(other.outputLayerBiasMask);

	for (int i = 0; i < h1_size; i++) {
		activationPerHidden[i] = other.activationPerHidden[i];
	}
}