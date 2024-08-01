#pragma once

#include "Eigen/Core"
#include <iostream>
#include <vector>
#include "MyRandom.h"

const int NEURON_TYPES = 6;

const int i_sugarLevels = 0;

class Connection;

class Neuron {
public:
	int id; // negative values are inputs, positive values are outputs. others are hidden
	int layer = 0;
	float outValue = 0.0f;
	int type = 0;

	std::vector<Connection*> connections;
	float calculate(float sum);
};

class Connection {
public:
	int fromID;
	int toID;
	float weight;
	bool isNew = false;

	Neuron* from = nullptr;
	Neuron* to = nullptr;

};

class NeatBrain {
public:
	int input_size;
	int output_size;
	int outputLayer;

	std::vector<Neuron*> neurons;
	std::vector<Neuron*> outputNeurons;
	std::vector<Connection*> connections;
	
	NeatBrain(int inputs, int outputs);
	Eigen::VectorXf forward(Eigen::VectorXf& input);
	void build();
	void buildNeuron(Neuron* neuron);
	Connection* addConnection(int from, int to);
	void removeConnection(Connection* conn);
	Neuron* createNeuron();
	void mutate(float mutationScale, float mutateConnectionChance,
		float addConnectionChance, float removeConnectionChance,
		float createNeuronChance, float deleteNeuronChance, float mutateNeuronChance);
	int getIndexOfLayer(int layer);
	void discard();

private:
	BRandom rndEngine;
	bool requiresBuilding = true;

	int generateNeuronID();
	static bool compareByLayer(const Neuron* obj1, const Neuron* obj2);
};
