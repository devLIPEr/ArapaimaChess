#include <iostream>
#include <sstream>

#include "engine.h"
#include "book.h"

#ifndef UCI_H
#define UCI_H

using namespace std;

namespace arapaimachess{

class UCI{
    private:
        Engine *engine;
        Book *book;
        const Bitboard start_pos[12] = {65280, 66, 36, 129, 8, 16, 71776119061217280ULL, 4755801206503243776ULL, 2594073385365405696ULL, 9295429630892703744ULL, 576460752303423488ULL, 1152921504606846976ULL};
        bool is_start_pos = false;
        vector<string> moves;
        string book_move = "(none)";
    public:
        UCI(Engine *engine);
        UCI(Engine *engine, Book *book);
        ~UCI();

        void read();

        void go(string args);
        void position(istringstream& stream);
        void setoption(istringstream& stream);
};

}

#endif