#pragma once

#include <vector>
#include <iostream>
#include <map>
#include <torch/torch.h>
#include "Eigen/Dense"
#include "Eigen/Sparse"
#include <random>
#include "MyRandom.h"

const int TYPE_LINEAR = 0;
const int TYPE_SIGMOID = 1;
const int TYPE_TANH = 2;
const int TYPE_RELU = 3;
const int TYPE_SIN = 4;
const int TYPE_COS = 5;
const int TYPE_INVERSE = 6;
const int TOTAL_NEURON_TYPES = 7;

class Gene {
public:
	std::string name;
	float value;
	float min;
	float max;
	float step;
	Gene(std::string name, float value, float min, float max, float step);
};

class Genetics {
public:
	std::map<std::string, Gene*> phenotype;
	BRandom rndEngine;

	Genetics();
	void toDefault();
	void addGene(Gene* gene);
	float getValue(std::string name);
	double random();
	void copy(Genetics& genetics);
	void mutateAll(float chance);
	void crossover(Genetics& other, float percent);
};

class CellBrain {
public:
	int input_size = 50;
	int output_size = 5; // attack, split, turn, forward
	int h1_size = 8;

	int activeWeights = 0;
	int totalWeights = 0;

	Eigen::MatrixXf inputLayerWeights;
	Eigen::MatrixXf outputLayerWeights;
	Eigen::VectorXf inputLayerBias;
	Eigen::VectorXf outputLayerBias;

	Eigen::MatrixXf inputLayerWeightsMask;
	Eigen::MatrixXf outputLayerWeightsMask;
	Eigen::VectorXf inputLayerBiasMask;
	Eigen::VectorXf outputLayerBiasMask;

	Eigen::SparseMatrix<float> actual_inputLayerWeights;
	Eigen::SparseMatrix<float> actual_outputLayerWeights;
	Eigen::SparseVector<float> actual_inputLayerBias;
	Eigen::SparseVector<float> actual_outputLayerBias;
	std::vector<int> activationPerHidden;
	bool isBuilt = false;
	bool hasMutated = false;

	BRandom rndEngine;
	CellBrain();

	// Forward pass implementation
	Eigen::VectorXf forward(Eigen::VectorXf& input);
	void mutate(float chance, float scale, float chanceToToggle, float chanceToDisable, float neuronTypeChance);
	void setValuesFrom(CellBrain& other);
	void build();
	float distanceFrom(CellBrain& other);

	void crossover(CellBrain& other, float chance);
	void crossoverMatrix(Eigen::MatrixXf& matrixA, Eigen::MatrixXf& matrixB, float chance);
	void crossoverVector(Eigen::VectorXf& vectorA, Eigen::VectorXf& vectorB, float percent);
	float activation(int type, float sum);
	std::string activationName(int type);
	std::string outputName(int index);
	static void test();

private:

	void mutateMatrix(Eigen::MatrixXf& matrix, Eigen::MatrixXf& mask, float mutationProbability, float scale);
	void mutateVector(Eigen::VectorXf& vector, Eigen::VectorXf& mask, float chance, float scale);
	void toggleMatrix(Eigen::MatrixXf& matrix, float chance);
	void toggleVector(Eigen::VectorXf& matrix, float chance);
	void toggleMatrixIndex(Eigen::MatrixXf& matrix, float chance);
	void toggleVectorIndex(Eigen::VectorXf& matrix, float chance);

	Eigen::MatrixXf xavierInitWeightMatrix(int rows, int cols);
	Eigen::VectorXf xavierInitBiasVector(int size);
	float random();
};