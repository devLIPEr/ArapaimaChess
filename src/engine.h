#include <iostream>
#include <thread>

#include "utils.h"
#include "zobrist.h"
#include "board.h"
#include "move_generator.h"
#include "search.h"
#include "transposition_table.h"
#include "entry.h"
#include "config.h"
#include "types.h"

#ifndef ENGINE_H
#define ENGINE_H

using namespace std;

namespace arapaimachess{

class Engine{
    private:
        string version_number = "0.1";
        string version_type = "dev";
        string author = "devLIPEr";

        int num_threads;
        int tt_size;

        u64 seed;

        TT *tt;
        Zobrist *zobrist_table;
        Search *search;
    public:
        bool ready = true;
        bool syzygy = false;

        int hash_min = 1, hash_max = 1024;
        Board<MAGIC> *board;
        atomic<bool> stop_search;
        atomic<bool> stoped_search;
        MoveGenerator<MAGIC> *move_generator;
        string last_move = "(none)";

        PVLine pv;
        atomic<int> d = 0;
        atomic<u64> nodes_count = 0;
        atomic<int> eval;
        string pv_line;

        atomic<int> hits;

        Engine(TT *tt, Zobrist *zobrist_table, MoveGenerator<MAGIC> *move_generator, Search *search, Board<MAGIC> *board);
        ~Engine();

        string get_options();
        string get_name();
        string get_info();
        string get_ready();
        
        string print_board();
        
        void set_threads(int threads);
        void set_hash(int size);

        void set_null_move(bool set);
        void set_late_move(bool set);
        void set_futility(bool set);
        void set_razoring(bool set);

        void set_position(string fen);
        void stop(int type);
        void reset_search();
        void reset_history();
        void make_move(string move);
        void go_search(int depth, vector<string> moves, bool hint);
        u64 go_perft(int depth);
};

}

#endif