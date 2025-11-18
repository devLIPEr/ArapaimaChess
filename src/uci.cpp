#include <iostream>
#include <sstream>
#include <thread>
#include <cassert>
#include <algorithm>
#include "uci.h"

#ifdef __cplusplus
extern "C"{
#endif
#include "syzygy/tbprobe.h"
#ifdef __cplusplus
}
#endif

namespace arapaimachess{

/// @brief Create a new UCI object with default opening book
/// @param engine reference to the Engine object
UCI::UCI(Engine *engine){
    assert(engine != NULL);
    this->engine = engine;
    this->book = new Book("./opening_book.txt", 914060149);
    engine->syzygy = false;
    if(tb_init("./syzygy_table") && TB_LARGEST > 0){
        engine->syzygy = true;
    }
}

/// @brief Create a new UCI object
/// @param engine reference to the Engine object
/// @param book reference to the opening Book object
UCI::UCI(Engine *engine, Book *book){
    assert(engine != NULL && book != NULL);
    this->engine = engine;
    this->book = book;
    engine->syzygy = false;
    if(tb_init("./syzygy_table") && TB_LARGEST > 0){
        engine->syzygy = true;
    }
}

UCI::~UCI(){
    delete book;
    book = NULL;
}

thread go_thread;
atomic<bool> going = false;

/// @brief Read UCI command from stdin
void UCI::read(){
    string token, cmd;

    is_start_pos = memcmp(start_pos, engine->board->board, 12*sizeof(Bitboard)) == 0;

    do{
        if(!getline(cin, cmd)){
            cmd = "quit";
        }

        istringstream is(cmd);

        token.clear();
        is >> skipws >> token;

        if(token == "go"){
            if(going.load(memory_order_relaxed)){
                going.store(false, memory_order_relaxed);
                engine->stop_search.store(true, memory_order_relaxed);
                engine->stoped_search.store(true, memory_order_relaxed);
            }
            if(!going.load(memory_order_relaxed)){
                string args;
                getline(is, args);
                go_thread = thread([this, args](){
                    go(args);
                });
                go_thread.detach();
                going.store(true, memory_order_relaxed);
            }
        }else if(token == "quit"){
            engine->stop(0);
        }else if(token == "stop"){
            going.store(false, memory_order_relaxed);
            engine->stop(1);
        }else if(token == "uci"){
            cout << engine->get_info();
            cout << engine->get_options();
            cout << "uciok\n" << flush;
        }else if(token == "ucinewgame"){
            engine->set_position("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0");
            engine->pv.cmove = 0;
            engine->reset_search();
            engine->reset_history();
            book->reset_book();
        }else if(token == "isready"){
            cout << engine->get_ready() << flush;
        }else if(token == "setoption"){
            setoption(is);
        }else if(token == "position"){
            going.store(false, memory_order_relaxed);
            engine->pv.cmove = 0;
            position(is);
        }else if(token == "d" || token == "display" || token == "print"){
            cout << engine->print_board();
        }else if(token == "move"){
            engine->pv.cmove = 0;
            is >> token;
            book->go_move(token);
            moves.push_back(token);
            engine->make_move(token);
        }
    }while(token != "quit");
}

/// @brief Start go command, which starts search/perft from engine
/// @param args arguments of the go command
void UCI::go(string args){
    string token = "";
    int depth, wtime, btime, winc, binc, movetime;
    depth = wtime = btime = winc = binc = movetime = -1;
    u64 nodes = 0;
    vector<string> moves;
    bool is_perft = false;

    istringstream stream(args);
    while(stream >> token){
        if(token == "depth"){
            stream >> depth;
        }else if(token == "infinite"){
            depth = 200;
        }else if(token == "wtime"){
            stream >> wtime;
        }else if(token == "winc"){
            stream >> winc;
        }else if(token == "btime"){
            stream >> btime;
        }else if(token == "binc"){
            stream >> binc;
        }else if(token == "nodes"){
            stream >> nodes;
        }else if(token == "movetime"){
            stream >> movetime;
        }else if(token == "perft"){
            is_perft = true;
            stream >> depth;
        }else if(token != "searchmoves"){
            moves.push_back(token);
        }
    };

    bool hint_book_move = moves.size() == 0;
    if(!is_perft){
        book_move = book->get_move(false);
        if(book_move != "(none)" && hint_book_move){
            moves.push_back(book_move);
        }else{
            hint_book_move = false;
        }
        double max_search = (engine->board->curr_player == WHITE) ? (wtime/20.0 + winc/2.0) : (btime/20.0 + binc/2.0);
        if(movetime != -1){
            max_search = movetime;
        }
        if(nodes != 0){
            max_search = -1;
        }
    
        if(nodes == 0 && max_search <= 0){
            max_search = 9e18;
        }
    
        if(depth == -1){
            depth = 200;
        }else{
            depth = min(200, max(depth, 0));
        }
            
        engine->nodes_count.store(0, memory_order_relaxed);
        engine->d.store(0, memory_order_relaxed);
        engine->pv_line = "";
        this->engine->stop_search.store(false, memory_order_relaxed);

        auto start = chrono::high_resolution_clock::now();

        thread t([this, depth, moves, hint_book_move](){
            engine->go_search(depth, moves, hint_book_move);
        });
        t.detach();

        int d = 0;
        auto ellapsed = chrono::duration_cast<chrono::milliseconds>(chrono::high_resolution_clock::now() - start).count();
        int64_t last_ellapsed = ellapsed;
        while(going.load(memory_order_relaxed) && !engine->stop_search.load(memory_order_relaxed) && (ellapsed < max_search || engine->nodes_count < nodes)){
            int d1 = engine->d.load(memory_order_relaxed);
            string pv_line = engine->pv_line;
            auto new_ellapsed = chrono::duration_cast<chrono::milliseconds>(chrono::high_resolution_clock::now() - start).count();
            if(d < d1 && pv_line.length() > 0){
                d = d1;
                cout << "info depth " << (d) << " score cp " << engine->eval.load(memory_order_relaxed) << " nps " << (u64)(engine->nodes_count.load(memory_order_relaxed)/((new_ellapsed-last_ellapsed)*1e-3)) << " nodes " << engine->nodes_count.load(memory_order_relaxed) << " tbhits " << engine->hits.load(memory_order_relaxed) << " time " << (new_ellapsed-last_ellapsed) << " pv " << pv_line << '\n' << flush;
                last_ellapsed = new_ellapsed;
            }
            ellapsed = new_ellapsed;
        }

        this->engine->stop_search.store(true, memory_order_relaxed);

        while(!engine->stoped_search.load(memory_order_relaxed)){}
        
        if(d < engine->d.load(memory_order_relaxed) && abs(engine->eval.load(memory_order_relaxed)) < 2147400001){
            auto new_ellapsed = chrono::duration_cast<chrono::milliseconds>(chrono::high_resolution_clock::now() - start).count();
            u64 nps = (engine->nodes_count.load(memory_order_relaxed)/((new_ellapsed-last_ellapsed)*1e-3));
            cout << "info depth " << engine->d.load(memory_order_relaxed) << " score cp " << engine->eval.load(memory_order_relaxed) << " nps " << nps << " nodes " << engine->nodes_count.load(memory_order_relaxed) << " tbhits " << engine->hits.load(memory_order_relaxed) << " time " << (new_ellapsed-last_ellapsed) << " pv " << engine->pv_line << '\n' << flush;
            ellapsed = new_ellapsed;
            last_ellapsed = new_ellapsed;
        }

        string move_string = "(none)";
        if(engine->pv.flags[0] != 2){
            vector<Move> legal = engine->move_generator->order_moves(engine->board->board, engine->move_generator->legal_moves(engine->board->board, engine->board->curr_player, engine->board->castling_rights, engine->board->en_passant), engine->board->curr_player, false);
            move_string = get_move_string(legal[0]);
            Move engine_move = engine->pv.argmove[0];
            auto it = find_if(legal.begin(), legal.end(), [&engine_move](const Move &move){
                return move.from == engine_move.from && move.to == engine_move.to;
            });
            if(it != legal.end()){
                legal.erase(it);
                move_string = get_move_string(engine->pv.argmove[0]);
            }else if(engine->last_move == get_move_string(engine->pv.argmove[1]) && get_move_string(engine->pv.argmove[2]).length()){
                move_string = get_move_string(engine->pv.argmove[2]);
            }
        }else{
            move_string = get_move_string(engine->pv.argmove[0]);
            engine->pv.flags[0] = 0;
        }
        cout << "bestmove " << ((move_string.length()) ? move_string : "(none)") << '\n' << flush;
    }else{
        this->engine->stop_search.store(false, memory_order_relaxed);
        auto start = chrono::high_resolution_clock::now();
        u64 perft_nodes = this->engine->go_perft(depth);
        auto ellapsed = chrono::duration_cast<chrono::milliseconds>(chrono::high_resolution_clock::now() - start).count();
        this->engine->reset_search();
        cout << perft_nodes << " nodes found at depth = " << depth << " with time of " << ellapsed << " ms and " << (u64)((double)(perft_nodes)/(ellapsed*1e-3)) << " NPS\n" << flush;
    }
    engine->pv.cmove = 0;
    going.store(false, memory_order_relaxed);
}

/// @brief Process position command
/// @param stream stream containing arguments for the command
void UCI::position(istringstream& stream){
    string token, fen;
    stream >> token;
    moves.clear();
    do{
        if(token == "moves"){
            while(!stream.eof()){
                stream >> token;
                book->go_move(token);
                moves.push_back(token);
                engine->make_move(token);
            }
        }else if(token == "startpos"){
            engine->set_position("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0");
            stream >> token;
            book->reset_book();
        }else if(token == "fen"){
            stream >> token;
            while(token != "moves" && !stream.eof()){
                fen += token + " ";
                stream >> token;
            }
            engine->set_position(fen);
            book->reset_book();
        }
    }while(!stream.eof());

    is_start_pos = memcmp(start_pos, engine->board->board, 12*sizeof(Bitboard)) == 0;
}

/// @brief Process setoption command
/// @param stream stream containing arguments for the command
void UCI::setoption(istringstream& stream){
    string token;
    int threads, hash_size;
    threads = hash_size = -1;
    do{
        stream >> token;

        if(token == "Clear" || token == "clear"){
            stream >> token;
            if(token == "Hash" || token == "hash"){
                engine->reset_search();
            }
        }else if(token == "Threads" || token == "threads"){
            stream >> token;
            stream >> threads;
            engine->set_threads(threads);
        }else if(token == "Hash" || token == "hash"){
            stream >> token;
            stream >> hash_size;
            hash_size = min(engine->hash_max, max(engine->hash_min, hash_size));
            engine->set_hash(hash_size);
        }else if(token == "NullMove" || token == "nullmove"){
            stream >> token;
            if(token == "value")
                stream >> token;
            engine->set_null_move((token == "true") ? true : false);
        }else if(token == "LateMove" || token == "latemove"){
            stream >> token;
            if(token == "value")
                stream >> token;
            engine->set_late_move((token == "true") ? true : false);
        }else if(token == "Futility" || token == "futility"){
            stream >> token;
            if(token == "value")
                stream >> token;
            engine->set_futility((token == "true") ? true : false);
        }else if(token == "Razoring" || token == "razoring"){
            stream >> token;
            if(token == "value")
                stream >> token;
            engine->set_razoring((token == "true") ? true : false);
        }else if(token == "AllPruning" || token == "allpruning"){
            stream >> token;
            if(token == "value")
                stream >> token;
            engine->set_null_move((token == "true") ? true : false);
            engine->set_late_move((token == "true") ? true : false);
            engine->set_futility((token == "true") ? true : false);
            engine->set_razoring((token == "true") ? true : false);
        }else if(token == "OpeningBook" || token == "openingbook"){
            engine->ready = false;
            stream >> token;
            if(token == "value")
                stream >> token;
            if(book != NULL){
                delete book;    
            }
            this->book = new Book(token, 914060149);
            engine->ready = true;
        }else if(token == "SyzygyPath" || token == "syzygypath"){
            engine->ready = false;
            stream >> token;
            if(token == "value")
                stream >> token;
            if(engine->syzygy){
                tb_free();
            }
            engine->syzygy = false;
            if(tb_init(token.c_str()) && TB_LARGEST > 0){
                engine->syzygy = true;
            }
            engine->ready = true;
        }
    }while(!stream.eof());
}

}