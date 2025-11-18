#include <cassert>
#include <vector>
#include <algorithm>

#include "config.h"
#include "search.h"
#include "evaluate.h"
#include "board.h"
#include "utils.h"

#ifdef __cplusplus
extern "C"{
#endif
#include "syzygy/tbprobe.h"
#ifdef __cplusplus
}
#endif

using namespace std;

namespace arapaimachess{

/// @brief Create Search object
/// @param move_gen reference to MoveGenerator object
/// @param zobrist_table reference to Zobrist object
Search::Search(MoveGenerator<MAGIC> *move_gen, Zobrist *zobrist_table){
    assert(zobrist_table != NULL && move_gen != NULL);
    this->move_gen = move_gen;
    this->zobrist_table = zobrist_table;
}
Search::Search(){}

void Search::set_null_move(bool set){
    this->null_move = set;
}
void Search::set_late_move(bool set){
    this->late_move = set;
}
void Search::set_futility(bool set){
    this->futility = set;
}
void Search::set_razoring(bool set){
    this->razoring = set;
}

/// @brief Check if position is checkmate
/// @param board array of bitboards
/// @param player player to check if checkmated
/// @param cr castling rights
/// @param ep en passant square
/// @return true if position is a checkmate, false otherwise
bool Search::is_mate(Bitboard board[], Color player, CastlingRights cr, u8 ep){
    Bitboard empty_pieces = 0;
    for(int i = NO_PIECE; i < WHITE_KING; i++){
        empty_pieces |= board[i];
    }
    empty_pieces = ~empty_pieces;
    if(this->move_gen->in_check(board, empty_pieces, player) && this->move_gen->legal_moves(board, player, cr, ep).size() == 0){
        return true;
    }

    return false;
}

/// @brief Check if position is stalemate
/// @param board array of bitboards
/// @param player player to check if stalemated
/// @param cr castling rights
/// @param ep en passant square
/// @return true if position is a stalemate, false otherwise
bool Search::is_stalemate(Bitboard board[], Color player, CastlingRights cr, u8 ep){
    Bitboard empty_pieces = 0;
    for(int i = NO_PIECE; i < WHITE_KING; i++){
        empty_pieces |= board[i];
    }
    empty_pieces = ~empty_pieces;
    if(!this->move_gen->in_check(board, empty_pieces, player) && this->move_gen->legal_moves(board, player, cr, ep).size() == 0){
        return true;
    }

    return false;
}

/// @brief Check if position is insufficient material
/// @param board array of bitboards
/// @return true if material is insufficient, false otherwise
bool Search::is_insufficient_material(Bitboard board[]){
    static u8 k_K[12] = {0,0,0,0,0,1, 0,0,0,0,0,1};

    static u8 k_KB[12] = {0,0,0,0,0,1, 0,0,1,0,0,1};
    static u8 kb_K[12] = {0,0,1,0,0,1, 0,0,0,0,0,1};
    
    static u8 k_KN[12] = {0,0,0,0,0,1, 0,1,0,0,0,1};
    static u8 kn_K[12] = {0,1,0,0,0,1, 0,0,0,0,0,1};
    
    u8 piece_count[12] = {
        (u8)__builtin_popcountll(board[0]),  // BLACK PAWN
        (u8)__builtin_popcountll(board[1]),  // BLACK KNIGHT
        (u8)__builtin_popcountll(board[2]),  // BLACK BISHOP
        (u8)__builtin_popcountll(board[3]),  // BLACK ROOK
        (u8)__builtin_popcountll(board[4]),  // BLACK QUEEN
        (u8)__builtin_popcountll(board[5]),  // BLACK KING

        (u8)__builtin_popcountll(board[6]),  // WHITE PAWN
        (u8)__builtin_popcountll(board[7]),  // WHITE KNIGHT
        (u8)__builtin_popcountll(board[8]),  // WHITE BISHOP
        (u8)__builtin_popcountll(board[9]),  // WHITE ROOK
        (u8)__builtin_popcountll(board[10]), // WHITE QUEEN
        (u8)__builtin_popcountll(board[11])  // WHITE KING
    };

    if(memcmp(piece_count, k_KB, 12*sizeof(u8)) == 0){
        return true;
    }else if(memcmp(piece_count, kb_K, 12*sizeof(u8)) == 0){
        return true;
    }else if(memcmp(piece_count, k_KN, 12*sizeof(u8)) == 0){
        return true;
    }else if(memcmp(piece_count, kn_K, 12*sizeof(u8)) == 0){
        return true;
    }else if(memcmp(piece_count, k_K, 12*sizeof(u8)) == 0){
        return true;
    }

    bool only_bishops = true;
    for(int i = 0; i < 12 && only_bishops; i++){
        if((i != BLACK_BISHOP-1 || i != WHITE_BISHOP-1 || i != BLACK_KING-1 || i != WHITE_KING-1) && piece_count[i]){
            only_bishops = false;
        }
    }
    if(only_bishops){
        if(piece_count[BLACK_BISHOP-1] == 1 && piece_count[WHITE_BISHOP-1] == 1){
            return ((__builtin_ctzll(board[BLACK_BISHOP-1]) & 1) ^ (__builtin_ctzll(board[WHITE_BISHOP-1]) & 1)) == 0; // kb_KB
        }
        u8 white_bishops = 0, black_bishops = 0;
        if(piece_count[WHITE_BISHOP-1] > 1){
            Bitboard bb = board[WHITE_BISHOP-1];
            while(bb && white_bishops < 3){
                int index = __builtin_ctzll(bb);
                if(index & 1){
                    white_bishops |= 1; // bishop on white square
                }else{
                    white_bishops |= 2; // bishop on black square
                }
                bb &= bb-1;
            }
        }
        if(piece_count[BLACK_BISHOP-1] > 1){
            Bitboard bb = board[BLACK_BISHOP-1];
            while(bb && black_bishops < 3){
                int index = __builtin_ctzll(bb);
                if(index & 1){
                    black_bishops |= 1; // bishop on white square
                }else{
                    black_bishops |= 2; // bishop on black square
                }
                bb &= bb-1;
            }
        }
        if(((white_bishops & 1) && (black_bishops & 2)) || ((white_bishops & 2) && (black_bishops & 1))){
            return false; // kb*_KB* opposite colors
        }else{
            return true; // kb*_KB* same colors
        }
    }

    return false;
}

/// @brief Check if a position is terminal (checkmate, stalemate or insufficient material)
/// @param board array of bitboards
/// @param player player to check if stalemated
/// @param cr castling rights
/// @param ep en passant square
/// @return true if the position is terminal, false otherwise
bool Search::is_terminal(Bitboard board[], Color player, CastlingRights cr, u8 ep){
    return (
        is_mate(board, player, cr, ep) ||
        is_stalemate(board, player, cr, ep) ||
        is_insufficient_material(board)
    );
}

/// @brief Search function using Negamax and Alpha-Beta framework
/// @param rule50 rule 50 counter
/// @param stop flag to stop search when time is over
/// @param pv principal variation line of search
/// @param nodes node counter
/// @param max_depth max depth to search for
/// @param depth current search depth
/// @param alpha alpha limit
/// @param beta beta limit
/// @param board array of bitboards
/// @param player current player
/// @param cr castling rights
/// @param en_passant en passant square
/// @param tt reference to transposition table object
/// @param search_moves moves to search in the first depth or moves to search at each depth
/// @param search_order toggle between moves to search in the first depth and moves to search at each depth
/// @return evaluation of the current position
int Search::AlphaBeta(unsigned int rule50, atomic<bool> *stop, PVLine *pv, u64 &nodes, int max_depth, int depth, int alpha, int beta, Bitboard board[], Color player, CastlingRights cr, u8 en_passant, TT &tt, vector<Move> search_moves, bool search_order){
    PVLine line;
    bool can_prune = max_depth != depth;
    nodes++;

    if(cr == NO_CASTLING && Board<MAGIC>::count_pieces(board) <= TB_LARGEST){
        Bitboard white_pieces = 0;
        Bitboard black_pieces = 0;
        for(int i = 0; i < 6; i++){
            black_pieces |= board[i];
        }
        for(int i = 6; i < 12; i++){
            white_pieces |= board[i];
        }
        Bitboard ep_bb = (en_passant) ? bswap(1ULL << en_passant) : 0;
        u8 ep = __builtin_ctzll(ep_bb);
        unsigned res = tb_probe_wdl(
            bswap(white_pieces),
            bswap(black_pieces),
            bswap(board[11] | board[5]),
            bswap(board[10] | board[4]),
            bswap(board[9]  | board[3]),
            bswap(board[8]  | board[2]),
            bswap(board[7]  | board[1]),
            bswap(board[6]  | board[0]),
            0, 0, ep, player == WHITE
        );
        if(res == TB_RESULT_FAILED){
            return -2147400002;
        }
        return eval_wdl[TB_GET_WDL(res)];
    }

    if(is_mate(board, player, cr, en_passant)){
        return -(2147400001-(max_depth-depth));
    }else if(is_stalemate(board, player, cr, en_passant) || is_insufficient_material(board) || rule50 >= 100){
        return 0;
    }

    if(depth <= 0){
        int score = Quiesce(rule50, stop, nodes, alpha, beta, board, player, cr, en_passant, tt);
        if(score == 2147400001){
            score -= max_depth;
        }else if(score == -2147400001){
            score += max_depth;
        }
        return score;
    }

    Move hash_move;
    int alpha_orig = alpha;
    u64 key = zob_key(*zobrist_table, board, player, cr, en_passant);
    Entry curr_entry = tt.atomic_read(key);
    if(curr_entry.depth >= depth && curr_entry.is_board_equal(key)){
        if(curr_entry.move.from != 255){
            hash_move = curr_entry.move;
        }
        hits++;
        if(can_prune){
            if(curr_entry.flag == TT_EXACT){
                return curr_entry.eval;
            }else if(curr_entry.flag == TT_LOWER && curr_entry.eval >= beta){
                return curr_entry.eval;
            }else if(curr_entry.flag == TT_UPPER && curr_entry.eval <= alpha){
                return curr_entry.eval;
            }
        }else if(curr_entry.flag == TT_EXACT){
            pv->argmove[0] = curr_entry.move;
            pv->flags[0] = 1;
            pv->eval[0] = curr_entry.eval;
            memcpy(pv->argmove + 1, line.argmove, line.cmove * sizeof(Move));
            memcpy(pv->eval + 1, line.eval, line.cmove * sizeof(int));
            pv->cmove = line.cmove + 1;
        }
    }

    Bitboard board_copy[12];

    // Null Move Pruning
    if(can_prune && !search_order && null_move && depth >= NULL_MOVE_DEPTH && !has_only_pawns(board, player)){
        memcpy(board_copy, board, 12*sizeof(Bitboard));
        CastlingRights cr_copy = cr;
        u8 ep = en_passant;
        Bitboard empty_pieces = 0;
        for(int i = NO_PIECE; i < WHITE_KING; i++){
            empty_pieces |= board_copy[i];
        }
        empty_pieces = ~empty_pieces;
        int reduction = NULL_REDUCTION;
        if(depth-reduction < NULL_MOVE_DEPTH){
            reduction = 0;
        }
        if(!this->move_gen->in_check(board_copy, empty_pieces, player)){
            PVLine null_line;
            null_move = false;
            int score = -AlphaBeta(rule50, stop, &null_line, nodes, max_depth, depth-1-reduction, -beta, -(beta-1), board_copy, Color(player^1), cr_copy, ep, tt, vector<Move>(), false);
            null_move = true;
            if(score >= beta){
                return beta;
            }
        }
    }

    int eval = 0;
    if(can_prune && !search_order){
        #if defined(NN_EVAL)
            eval = evaluate(board, cr, en_passant, player);
        #else
            eval = evaluate(board, evaluation, 12, 6);
        #endif
    }

    // Razoring
    if(can_prune && !search_order && razoring){
        if(eval < alpha - 514 - 294 * depth * depth){
            return Quiesce(rule50, stop, nodes, alpha, beta, board, player, cr, en_passant, tt);
        }
    }

    // Futility pruning
    if(can_prune && !search_order && !pv->flags[0] && futility){
        int l2 = (32-__builtin_clz(depth));
        l2 = (l2 != 32) ? (l2 >> 1) : 0;
        int futility_margin = 200 * l2;

        if(eval - futility_margin >= beta && eval >= beta){
            return (2 * beta + eval) / 3;
        }
    }

    vector<Move> moves;
    vector<Move> legal_moves = this->move_gen->legal_moves(board, player, cr, en_passant);
    if(hash_move.from != 255){
        auto it = find(legal_moves.begin(), legal_moves.end(), hash_move);
        if(it != legal_moves.end()){
            legal_moves.erase(it);
            moves.push_back(hash_move);
        }
    }
    if(search_moves.size() > 0){
        if(search_order){
            Move pv_move = create_move(255, 255, 255, 255, 255, 255, 255);
            if(max_depth-depth >= 0 && (size_t)(max_depth-depth) < search_moves.size()){
                pv_move = search_moves[max_depth-depth];
            }

            vector<Move> ordered = this->move_gen->order_moves(board, legal_moves, player, false);
            moves.reserve(moves.size() + ordered.size());
            if(pv_move.from != 255){
                moves.push_back(pv_move);
                auto it = find(ordered.begin(), ordered.end(), pv_move);
                if(it != ordered.end()){
                    ordered.erase(it);
                }
            }
            moves.insert(moves.end(), ordered.begin(), ordered.end());
        }else{
            vector<Move> ordered = this->move_gen->order_moves(board, search_moves, player, false);
            moves.reserve(moves.size() + ordered.size());
            moves.insert(moves.end(), ordered.begin(), ordered.end());
        }
    }else{
        vector<Move> ordered = this->move_gen->order_moves(board, legal_moves, player, false);
        moves.insert(moves.end(), ordered.begin(), ordered.end());
    }

    bool first_move = true;
    bool stopped_search = false;
    int i = 0;
    for(Move move: moves){
        line.cmove = 0;
        memcpy(board_copy, board, 12*sizeof(Bitboard));
        CastlingRights cr_copy = cr;
        u8 ep = en_passant;

        Board<MAGIC>::do_move(board_copy, move, player, cr_copy, ep);
        
        if(move.capture_piece != 255 || move.piece == (player*6)){
            rule50 = 0;
        }else{
            rule50++;
        }

        // Late Move Reduction/Pruning
        int reduction = 0;
        if(late_move && i >= 10){
            reduction = 32-__builtin_clz(i);
            reduction = (reduction == 32) ? 0 : (reduction >> 2);
            reduction = (reduction > MAX_LATE_REDUCTION) ? MAX_LATE_REDUCTION : reduction;
        }
        i++;
        int score = -AlphaBeta(rule50, stop, &line, nodes, max_depth, depth-1-reduction, -beta, -alpha, board_copy, Color(player^1), cr_copy, ep, tt, (first_move) ? search_moves : vector<Move>(), first_move);
        first_move = false;
        
        if(score >= beta){
            if(move.capture_piece == 255){
                this->move_gen->add_history(player, move, depth);
            }
            return beta;
        }
        if(score > alpha){
            alpha = score;
            pv->argmove[0] = move;
            pv->eval[0] = score;
            memcpy(pv->argmove + 1, line.argmove, line.cmove * sizeof(Move));
            memcpy(pv->eval + 1, line.eval, line.cmove * sizeof(int));
            pv->cmove = line.cmove + 1;
        }

        if(stop->load(memory_order_relaxed)){
            stopped_search = true;
            return alpha;
        }
    }

    if(!stopped_search){
        TT_FLAGS flag = TT_EXACT;
        if(alpha <= alpha_orig){
            flag = TT_UPPER;
            alpha = alpha_orig;
        }else if(alpha >= beta){
            flag = TT_LOWER;
            alpha = beta;
        }
        tt.add(key, Entry(depth, nodes, key, alpha, flag, pv->argmove[0]));
    }
    return alpha;
}

/// @brief Quiescence search function using Negamax framework
/// @param rule50 rule 50 counter
/// @param stop flag to stop search when time is over
/// @param nodes node counter
/// @param alpha alpha limit
/// @param beta beta limit
/// @param board array of bitboards
/// @param player current player
/// @param cr castling rights
/// @param en_passant en passant square
/// @param tt reference to transposition table object
/// @return evaluation of the position with quiescence search
int Search::Quiesce(unsigned int rule50, atomic<bool> *stop, u64 &nodes, int alpha, int beta, Bitboard board[], Color player, CastlingRights cr, u8 en_passant, TT &tt){
    nodes++;
    if(is_mate(board, player, cr, en_passant)){
        return -(2147400001);
    }else if(is_stalemate(board, player, cr, en_passant) || is_insufficient_material(board) || rule50 >= 100){
        return 0;
    }
    #if defined(NN_EVAL)
        int eval = evaluate(board, cr, en_passant, player) * (player == BLACK ? -1 : 1);
    #else
        int eval = evaluate(board, evaluation, 12, 6) * (player == BLACK ? -1 : 1);
    #endif

    if(eval >= beta){
        return beta;
    }
    if(eval > alpha){
        alpha = eval;
    }

    Bitboard board_copy[12];
    vector<Move> moves = move_gen->order_moves(board, move_gen->legal_moves(board, player, cr, en_passant), player, true);
    for(Move move : moves){
        memcpy(board_copy, board, 12*sizeof(Bitboard));
        CastlingRights cr_copy = cr;
        u8 ep = en_passant;

        // Delta Pruning
        int delta = QUEEN_VALUE;
        if(move.promotion_piece != 255 && move.promotion_piece != 0){
            delta += QUEEN_VALUE-200;
        }
        if(eval < alpha - delta){
            return alpha;
        }

        Board<MAGIC>::do_move(board_copy, move, player, cr_copy, ep);

        int score = -Quiesce(rule50+1, stop, nodes, -beta, -alpha, board_copy, Color(player^1), cr_copy, ep, tt);

        if(score >= beta){
            return beta;
        }
        if(score > alpha){
            alpha = score;
        }
        if(stop->load(memory_order_relaxed)){
            return alpha;
        }
    }

    return alpha;
}


}