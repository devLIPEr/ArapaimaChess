#include "types.h"

#include <cstring>
#include <cstdlib>

#ifndef ENTRY_H
#define ENTRY_H

namespace arapaimachess{

class Entry{
    public:
        int depth = -1, eval = 0;
        unsigned long long count = 0;
        Move move;
        unsigned long long key = 0;
        TT_FLAGS flag;

        Entry(int depth, unsigned long long count, unsigned long long key);
        Entry(int depth, unsigned long long count, unsigned long long key, int eval);
        Entry(int depth, unsigned long long count, unsigned long long key, int eval, TT_FLAGS flag);
        Entry(int depth, unsigned long long count, unsigned long long key, int eval, TT_FLAGS flag, Move move);
        Entry() = default;
        Entry(const Entry& other) = default;
        ~Entry() = default;
        Entry& operator=(const Entry& other) = default;
        bool is_board_equal(unsigned long long key);
};

}

#endif