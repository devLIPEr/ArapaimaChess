#ifndef BOOH_H
#define BOOK_H

#include <iostream>
#include <vector>

#include "types.h"
#include "./prng/cgw64.h"

using namespace std;

namespace arapaimachess{

struct Trie{
    vector<Trie*> children;
    char move[7] = "(none)";
    u8 n = 0;

    Trie(vector<Trie*> children, string move, u8 n){
        this->children = children;
        for(int i = 0; i < 6; i++){
            this->move[i] = move[i];
        }
        this->move[6] = '\0';
        this->n = n;
    }
};

class Book{
    private:
        Trie *start_book = NULL;
        Trie *opening = NULL;
        CGW64 rng = CGW64(8428114415715405298ULL);
        int chances[1 << 12];
        bool first_move = true;
        bool valid_opening = true;
    public:
        u8 max_depth = 0;
        Book() = default;
        Book(string path, u64 seed);
        ~Book() = default;

        void reset_book();
        void read_book(string path);
        string get_move(bool go_deep);
        void go_move(string move);
};

}

#endif