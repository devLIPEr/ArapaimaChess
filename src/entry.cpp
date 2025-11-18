#include "entry.h"
#include "types.h"

#include <iostream>
#include <cstring>

using namespace std;

namespace arapaimachess{

/// @brief Create an Entry for the transposition table
/// @param depth depth to store
/// @param count amount of nodes searched
/// @param key zobrist key for this position
Entry::Entry(int depth, unsigned long long count, unsigned long long key){
    this->depth = depth;
    this->count = count;
    this->eval = 0;
    this->flag = NO_FLAG;
    this->key = key;
}

/// @brief Create an Entry for the transposition table
/// @param depth depth to store
/// @param count amount of nodes searched
/// @param key zobrist key for this position
/// @param eval evaluation of this position
Entry::Entry(int depth, unsigned long long count, unsigned long long key, int eval){
    this->depth = depth;
    this->count = count;
    this->eval = eval;
    this->flag = NO_FLAG;
    this->key = key;
}

/// @brief Create an Entry for the transposition table
/// @param depth depth to store
/// @param count amount of nodes searched
/// @param key zobrist key for this position
/// @param eval evaluation of this position
/// @param flag prune type (UPPER, LOWER, EXACT) for this entry
Entry::Entry(int depth, unsigned long long count, unsigned long long key, int eval, TT_FLAGS flag){
    this->depth = depth;
    this->count = count;
    this->eval = eval;
    this->flag = flag;
    this->key = key;
}

/// @brief Create an Entry for the transposition table
/// @param depth depth to store
/// @param count amount of nodes searched
/// @param key zobrist key for this position
/// @param eval evaluation of this position
/// @param flag prune type (UPPER, LOWER, EXACT) for this entry
/// @param move move made to get here
Entry::Entry(int depth, unsigned long long count, unsigned long long key, int eval, TT_FLAGS flag, Move move){
    this->depth = depth;
    this->count = count;
    this->eval = eval;
    this->flag = flag;
    this->key = key;
    this->move = move;
}

/// @brief Check if the entry key is equal to a given key
/// @param k key to check against
/// @return true if keys are equal, false otherwise
bool Entry::is_board_equal(unsigned long long k){
    return (key == k);
}


}