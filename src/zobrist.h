#ifndef ZOBRIST_H
#define ZOBRIST_H
#include "./prng/cgw64.h"

namespace arapaimachess{

class Zobrist{
    private:
        unsigned long long keys[781];
        CGW64 rng;
    public:
        const static int black_to_move = 768;
        Zobrist();
        Zobrist(unsigned long long seed);
        ~Zobrist();
        unsigned long long& operator[](int index); 
};

}

#endif