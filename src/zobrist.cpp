#include "zobrist.h"

using namespace std;

namespace arapaimachess{

Zobrist::Zobrist(){
    this->rng = CGW64();
    for(int i = 0; i < 781; i++){
        this->keys[i] = rng.next();
    }
}

/// @brief Create a Zobrist object
/// @param seed seed for the random number generator
Zobrist::Zobrist(unsigned long long seed){
    this->rng = CGW64(seed);
    for(int i = 0; i < 781; i++){
        this->keys[i] = rng.next();
    }
}

Zobrist::~Zobrist(){}

/// @brief Get partial key for the given index
/// @param index index to get from
/// @return partial key
unsigned long long& Zobrist::operator[](int index){ return this->keys[index]; }

}