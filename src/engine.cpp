#include <iostream>
#include <functional>
#include <thread>
#include <cassert>
#include <chrono>
#include <cstring>
#include "engine.h"

#ifdef __cplusplus
extern "C"{
#endif
#include "syzygy/tbprobe.h"
#ifdef __cplusplus
}
#endif

using namespace std;

namespace arapaimachess{

/// @brief Create a engine object
/// @param tt reference to the transposition table object
/// @param zobrist_table reference to the Zobrist object
/// @param move_generator reference to the MoveGenerator object
/// @param search reference to the Search object
/// @param board reference to the board object
Engine::Engine(TT *tt, Zobrist *zobrist_table, MoveGenerator<MAGIC> *move_generator, Search *search, Board<MAGIC> *board){
    this->tt = tt;
    this->zobrist_table = zobrist_table;
    this->move_generator = move_generator;
    this->search = search;
    this->board = board;
}
Engine::~Engine(){}

string Engine::get_name(){ return "id name ArapaimaChess " + version_number + "-" + version_type; }
string Engine::get_info(){ return get_name() + "\nid author " + author + "\n"; }

/// @brief Get engine options for the uci command
/// @return engine options
string Engine::get_options(){
    char buffer[5];
    string options = "option name Hash type spin default 64 min ";
    sprintf(buffer, "%d", hash_min);
    options += string(buffer);
    options += " max ";
    sprintf(buffer, "%d", hash_max);
    options += string(buffer);
    options += "\noption name Clear Hash type button\n";
    options += "option name NullMove type check default false\n";
    options += "option name LateMove type check default false\n";
    options += "option name Futility type check default false\n";
    options += "option name Razoring type check default false\n";
    options += "option name AllPruning type check default false\n";
    options += "option name OpeningBook type string default opening_book.txt\n";
    options += "option name SyzygyPath type string default syzygy_table\n";
    return options;
}
string Engine::get_ready(){ return (ready ? "readyok\n" : "\0"); }

string Engine::print_board(){
    return this->board->get_board();
}

void Engine::set_threads(int threads){
    this->num_threads = threads;
}

void Engine::set_hash(int size){
    this->tt->resize(MB_to_TT(size));
}

void Engine::set_null_move(bool set){
    this->search->set_null_move(set);
}
void Engine::set_late_move(bool set){
    this->search->set_late_move(set);
}
void Engine::set_futility(bool set){
    this->search->set_futility(set);
}
void Engine::set_razoring(bool set){
    this->search->set_razoring(set);
}

void Engine::set_position(string fen){
    this->board->initialize_board(fen);
}

void Engine::reset_search(){
    this->tt->clear();
}
void Engine::reset_history(){
    this->move_generator->reset_history();
}

void Engine::stop(int type){
    if(type == 1){
        this->stop_search.store(true, memory_order_relaxed);
    }else if(type == 0){
        this->stop_search.store(true, memory_order_relaxed);
        exit(0);
    }
}

/// @brief Make a given move in uci notation
/// @param move move in uci notation
void Engine::make_move(string move){
    vector<Move> moves = move_generator->legal_moves(board->board, board->curr_player, board->castling_rights, board->en_passant);
    for(Move m : moves){
        if(get_move_string(m) == move){
            last_move = move;
            board->do_move(m);
            break;
        }
    }
}

/// @brief Start the search for a given position, sets the best move at the pv
/// @param depth max depth to search for
/// @param moves moves to search at start OR moves to search at each depth
/// @param hint toggle between moves to search at each depth (when true) or moves to search at the start (when false)
void Engine::go_search(int depth, vector<string> moves, bool hint){
    vector<Move> search_moves;
    if(moves.size() > 0){
        vector<Move> ms = move_generator->legal_moves(board->board, board->curr_player, board->castling_rights, board->en_passant);

        search_moves.reserve(moves.size());
        for(string move : moves){
            for(Move m : ms){
                if(get_move_string(m) == move){
                    search_moves.push_back(m);
                    break;
                }
            }
        }
    }

    bool fixed_search = search_moves.size() > 0 && !hint;
    stop_search.store(false, memory_order_relaxed);
    stoped_search.store(false, memory_order_relaxed);
    this->search->hits = 0;
    nodes_count.store(0, memory_order_relaxed);
    d.store(0, memory_order_relaxed);
    pv_line = "";
    eval = -2147400002;
    bool syzygy_fail = false;
    while(eval == -2147400002 && !stop_search.load(memory_order_relaxed)){
        if(!syzygy || syzygy_fail || board->count_pieces() > TB_LARGEST){
            for(int it_depth = 1; !stop_search.load(memory_order_relaxed) && it_depth <= depth && abs(eval) != 2147400000; it_depth++){
                u64 nodescount = 0;
                eval.store(search->AlphaBeta(board->rule50, &stop_search, &pv, nodescount, it_depth, it_depth, -2147400001, 2147400001, board->board, board->curr_player, board->castling_rights, board->en_passant, *tt, search_moves, !fixed_search) * (board->curr_player == BLACK ? -1 : 1), memory_order_relaxed);
                nodes_count.store(nodescount, memory_order_relaxed);
                fixed_search = false;
                
                search_moves.clear();
                search_moves.reserve(pv.cmove);
                for(int i = 0; i < pv.cmove; i++){
                    search_moves.push_back(pv.argmove[i]);
                }
                
                pv_line = "";
                for(int i = 0; i < pv.cmove; i++){
                    if(search_moves[i].from != 255){
                        pv_line += get_move_string(search_moves[i]);
                        pv_line += " ";
                    }
                }
                d.store(it_depth, memory_order_relaxed);
                hits.store(search->hits, memory_order_relaxed);
            }
        }else{
            pv.cmove = 1;
            Bitboard white_pieces = 0;
            Bitboard black_pieces = 0;
            for(int i = 0; i < 6; i++){
                black_pieces |= board->board[i];
            }
            for(int i = 6; i < 12; i++){
                white_pieces |= board->board[i];
            }
            Bitboard ep_bb = (board->en_passant != 255) ? bswap(1ULL << board->en_passant) : 0;
            u8 ep = __builtin_ctzll(ep_bb);
            unsigned res = tb_probe_root(
                bswap(white_pieces),
                bswap(black_pieces),
                bswap(board->board[11] | board->board[5]),
                bswap(board->board[10] | board->board[4]),
                bswap(board->board[9]  | board->board[3]),
                bswap(board->board[8]  | board->board[2]),
                bswap(board->board[7]  | board->board[1]),
                bswap(board->board[6]  | board->board[0]),
                board->rule50, board->castling_rights, ep, board->curr_player == WHITE,
                NULL
            );
            if(res == TB_RESULT_FAILED){
                syzygy_fail = true;
            }else{
                unsigned wdl   = TB_GET_WDL(res);
                unsigned from  = TB_GET_FROM(res);
                unsigned to    = TB_GET_TO(res);
                unsigned promo = TB_GET_PROMOTES(res);
                promo = (promo > 0) ? 5-promo : 255;
                u8 f = ((7-((from >> 3) & 7)) << 3) | (from & 7);
                u8 t = ((7-((to >> 3) & 7)) << 3) | (to & 7);
                Move m = create_move(
                    (f != t) ? f : 255,
                    t,
                    255, 255, promo, 255, 255);
                pv_line = get_move_string(m);
                pv.argmove[0] = m;
                pv.flags[0] = 2;
                pv.eval[0] = eval_wdl[wdl];
                pv.cmove = 0;
                eval = (board->curr_player) ? eval_wdl[wdl] : -eval_wdl[wdl];
            }
        }
    }
    stop_search.store(true, memory_order_relaxed);
    stoped_search.store(true, memory_order_relaxed);
}

/// @brief Run perft test for a given depth (go perft depth)
/// @param depth depth to run the perft test for
/// @return amount of positions found during the test
u64 Engine::go_perft(int depth){
    stoped_search.store(false, memory_order_relaxed);
    u64 nodes = move_generator->perft_parallel(depth, board->board, board->curr_player, board->castling_rights, board->en_passant, *tt, false);
    stop_search.store(true, memory_order_relaxed);
    stoped_search.store(true, memory_order_relaxed);
    return nodes;
}

}