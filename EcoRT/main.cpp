#include "Ecosystem.h"

int main() {
    omp_set_num_threads(4);
    MyMath::init();

    std::cout << __cplusplus << std::endl;

    //try {
    	Ecosystem eco = Ecosystem();
    	eco.run();
    //} catch (const std::exception& e) {
    //	std::cerr << "Exception caught during forward pass: " << e.what() << std::endl;
    //}
}

