#include "NeatBrain.h"

float Neuron::calculate(float sum) {
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

	return 0;
}


// Neat Brain

NeatBrain::NeatBrain(int inputs, int outputs) {
	input_size = inputs;
	output_size = outputs;
	rndEngine.setSeed(1234);

	for (int i = 0; i < output_size; i++) {
		Neuron* neuron = new Neuron();
		neuron->id = i;
		neuron->layer = 0; // outputs start on layer 0

		outputNeurons.push_back(neuron);
	}

	build();
}

int NeatBrain::getIndexOfLayer(int layer) {

	int index = 0;
	for (Neuron* neuron : neurons) {
		if (neuron->layer == layer) {
			return index;
		}
		index++;
	}

	return neurons.size();
}

void NeatBrain::removeConnection(Connection* conn) {
	// remove from global list
	int index = -1;
	for (Connection* c2 : connections) {
		if (c2 == conn) {
			break;
		}
		index++;
	}

	if (index != -1) {
		connections.erase(connections.begin() + index);

		// erase from to neuron list
		if (conn->to != nullptr) {
			std::vector<Connection*>& nconn = conn->to->connections;
			
			if (nconn.size() > 0) {
				auto it = std::find(nconn.begin(), nconn.end(), conn);

				if (it != nconn.end()) {  // Check if object is found

					nconn.erase(it);  // Erase the object from the vector
				}
			}
		}
	}
}

void NeatBrain::mutate(float mutationScale, float mutateConnectionChance,
	float addConnectionChance, float removeConnectionChance,
	float createNeuronChance, float deleteNeuronChance, float mutateNeuronChance) {
	requiresBuilding = true;

	// mutate connections
	for (Connection* conn : connections) {
		if (!conn->isNew) {
			if (rndEngine.randomFloat() < mutateConnectionChance) {
				conn->weight += rndEngine.randomFloat() * mutationScale;
			}
		}
	}

	// add/remove hidden neurons
	if (rndEngine.randomFloat() < deleteNeuronChance) {

		if (neurons.size() > 0) {

			if (connections.size() > 0) {

				// choose random hidden neuron to delete.
				int index = rndEngine.randomInt(0, neurons.size());

				Neuron* neuron = neurons[index];
				std::vector<Connection*>& toRemove = neuron->connections;

				// remove neuron
				auto it = std::find(neurons.begin(), neurons.end(), neuron);
				if (it != neurons.end()) {  // Check if object is found
					neurons.erase(it);  // Erase the object from the vector
				}

				// delete connections to this neuron
				for (Connection* conn : toRemove) {
					removeConnection(conn); // disconnect from network
					delete conn;
				}

				// remove connections from this neuron
				for (Neuron* n2 : neurons) {
					// remove connections from this neuron
					std::vector<Connection*> nconn = n2->connections;

					int idToRemove = neuron->id;
					nconn.erase(std::remove_if(nconn.begin(), nconn.end(), [idToRemove](Connection* obj) {
						return obj->fromID == idToRemove; // Condition for removal
						}), nconn.end());
				}


				delete neuron;
			}

		}
		// add neuron
		else if (rndEngine.randomFloat() < createNeuronChance) {
			// convert a connection into a neuron and two connections
			Connection* conn = connections[rndEngine.randomInt(0, connections.size())];

			if (!conn->isNew) {
				Neuron* neuron = createNeuron();
				neuron->type = rndEngine.randomInt(0, NEURON_TYPES);
				// if from is an input, there is no fromNeuron
				neuron->layer = conn->from == nullptr ? 0 : (conn->from->layer + 1);

				// adjust all output neurons to move forward by 1 layer if neuron layer is equal to output layer
				if (neuron->layer == outputLayer) {

					outputLayer++;
					for (Neuron* on : outputNeurons) {
						on->layer = outputLayer;
					}

				}

				// now create connections
				int fromID = conn->fromID;
				int midID = neuron->id;
				Connection* c1 = addConnection(fromID, neuron->id);

				int a = 1;
				Connection* c2 = addConnection(neuron->id, conn->toID);

				c1->weight = 1.0f;
				c2->weight = conn->weight;
				c1->isNew = true;
				c2->isNew = true;

				// remove connection
				removeConnection(conn);

				// remove conn from memory
				delete conn;
			}
		}


	}

	// add/remove connections
	if (rndEngine.randomFloat() < addConnectionChance) {
		// create a connection


		// Get a random input or hidden neuron
		int fromListIndex = rndEngine.randomInt(0, input_size+neurons.size());

		int fromID;
		int fromLayer = 0;
		if (fromListIndex < input_size) {
			fromID = -fromListIndex - 1;
			fromLayer = -1;
		}
		else {
			// get the hidden neuron

			Neuron* neuron = neurons[fromListIndex - input_size];
			fromID = neuron->id;
			fromLayer = neuron->layer;
		}

		// select toID. toID must be in a higher layer. this is guaranteed to exist since outputs are seperate neurons.

		int startOfNextLayerIndex = getIndexOfLayer(fromLayer + 1); // hidden layer neuron start

		int toListIndex = rndEngine.randomInt(startOfNextLayerIndex, neurons.size() + output_size);

		Neuron* toNeuron = nullptr;
		if (toListIndex < neurons.size()) {
			// connect to hidden neuron
			toNeuron = neurons[toListIndex];
		}
		else {
			// connect to output neuron
			toNeuron = outputNeurons[toListIndex - neurons.size()];
		}

		// check if it exists
		bool exists = false;
		for (Connection* c2 : connections) {
			if (c2->fromID == fromID && c2->toID == toNeuron->id) {
				exists = true;
				break;
			}
		}

		if (!exists) {
			// add connection
			addConnection(fromID, toNeuron->id);
		}


	}
	else if (rndEngine.randomFloat() < removeConnectionChance) {
		if (connections.size() > 0) {
			// remove random connection
			int index = rndEngine.randomInt(0, connections.size());
			Connection* conn = connections[index];

			if(!conn->isNew) {
				removeConnection(conn);
			}
		}

	}

	build();
}

int NeatBrain::generateNeuronID() {
	return rndEngine.randomInt(output_size, 10000000);
}

Neuron* NeatBrain::createNeuron() {
	requiresBuilding = true;

	Neuron* neuron = new Neuron();
	neuron->id = generateNeuronID();

	neurons.push_back(neuron);
	return neuron;
}

Connection* NeatBrain::addConnection(int from, int to) {

	if (from == to) {
		std::cerr << "From and to cannot be the same. from: " << from << " to: " << to << std::endl;
		throw std::runtime_error("cant add connection.");
	}

	requiresBuilding = true;

	Connection* conn = new Connection();
	conn->fromID = from;
	conn->weight = 0.0f;
	connections.push_back(conn);

	Neuron* neuron = nullptr;
	if (to < output_size) {
		neuron = outputNeurons[to];
	}
	else {
		std::cout << "Looking for neuron: " << std::endl;
		for (Neuron* n2 : neurons) {
			std::cout << "id: " << n2->id << std::endl;
			if (n2->id == to) {
				neuron = n2;
				break;
			}
		}
	}
	std::cout << "to: " << to << " nullptr: " << (neuron == nullptr) << std::endl;
	neuron->connections.push_back(conn);

	return conn;
}

void NeatBrain::buildNeuron(Neuron* neuron) {
	std::vector<Connection*>& inConnections = neuron->connections;

	std::vector<int> indexesToRemove;

	int index = 0;
	for (Connection* conn : inConnections) {
		conn->to = neuron;
		conn->toID = neuron->id;
		int fromID = conn->fromID;
		conn->isNew = false;

		// build connections's fromNeuron
		// any id below 0 is an input
		if (fromID < 0) {
			// do nothing
			// should be impossible since you cant go to an input
		}
		else {
			// get neuron with id

			Neuron* toUse = nullptr;
			for (Neuron* n2 : neurons) {
				if (n2 != neuron) {
					if (fromID == n2->id) {
						toUse = n2;
						break;
					}
				}
			}
			for (Neuron* n2 : outputNeurons) {
				if (n2 != neuron) {
					if (fromID == n2->id) {
						toUse = n2;
						break;
					}
				}
			}

			if (toUse == nullptr) {
				// this from connection no longer exists. remove this connection.
				indexesToRemove.push_back(index);
				//std::cerr << "Error: Couldn't find neuron using connection fromID. from: " << fromID << " to: " << neuron->id << std::endl;
				//throw std::runtime_error("Couldn't build network.");
			}
			else {
				conn->from = toUse;
			}


		}

		index++;

	}

	for (int idx : indexesToRemove) {
		inConnections.erase(inConnections.begin() + idx);
	}
}

void NeatBrain::build() {
	// assign neurons to connections

	for (Neuron* neuron : neurons) {
		buildNeuron(neuron);
	}
	for (Neuron* neuron : outputNeurons) {
		buildNeuron(neuron);
	}
	

	// sort by layer
	std::sort(neurons.begin(), neurons.end(), compareByLayer);

	outputLayer = outputNeurons[0]->layer;

	requiresBuilding = false;

}

Eigen::VectorXf NeatBrain::forward(Eigen::VectorXf& input) {
	if (requiresBuilding) {
		std::cerr << "Error: Can't forward without building." << std::endl;
		throw std::runtime_error("Couldn't forward network.");
	}

	// if neurons are sorted by layer, just interate

	for (Neuron* neuron : neurons) {
		std::vector<Connection*>& inConnections = neuron->connections;

		float sum = 0.0;
		for (Connection* conn : inConnections) {
			int fromID = conn->fromID;
			float inputValue = 0.0f;

			if (conn->from != nullptr) {
				inputValue = conn->from->outValue;
			}
			else {
				inputValue = input[-fromID - 1];
			}


			float weight = conn->weight;
			sum += weight * inputValue;
		}


		neuron->outValue = neuron->calculate(sum);
	}

	// do the same to calculate outputs. ignore neuron type.
	Eigen::VectorXf outputs = Eigen::VectorXf(output_size);
	int index = 0;
	for (Neuron* on : outputNeurons) {
		std::vector<Connection*>& inConnections = on->connections;

		float sum = 0.0;
		for (Connection* conn : inConnections) {
			int fromID = conn->fromID;
			float inputValue = 0.0f;

			if (conn->from != nullptr) {
				inputValue = conn->from->outValue;
			}
			else {
				inputValue = input[-fromID - 1];
			}


			float weight = conn->weight;
			sum += weight * inputValue;
		}

		on->outValue = tanh(sum);
		outputs(index) = on->outValue;
		index++;
	}

	return outputs;
}

void NeatBrain::discard() {
	for (Connection* conn : connections) {
		delete conn;
	}

	for (Neuron* neuron : neurons) {
		delete neuron;
	}
	for (Neuron* neuron : outputNeurons) {
		delete neuron;
	}
}

// Comparator function to sort objects based on their id
bool NeatBrain::compareByLayer(const Neuron* obj1, const Neuron* obj2) {
	return obj1->layer < obj2->layer;
}