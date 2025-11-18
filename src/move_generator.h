#ifndef MOVEGEN_H
#define MOVEGEN_H

#include "config.h"
#include "types.h"
#include "transposition_table.h"
#include "entry.h"
#include "zobrist.h"
#include <omp.h>
#include <vector>

using namespace std;

namespace arapaimachess{

template <typename Magic>
class MoveGenerator{
    private:
        Zobrist *zobrist_table;
        Magic *magic;
        int num_threads;
        int history[2][64][64];
    public:
        MoveGenerator();
        MoveGenerator(Zobrist *zorist_table, Magic *magic, int num_threads);
        ~MoveGenerator() = default;

        void reset_history();
        void add_history(Color player, Move move, int depth);

        Bitboard get_attack_rook(int sq, Bitboard occ);
        Bitboard get_attack_bishop(int sq, Bitboard occ);
        Bitboard get_attack_knight(int sq);
        Bitboard get_attack_king(int sq);
        
        bool sliding_attack(Bitboard board, Bitboard opp_pieces, Bitboard empty_pieces, PieceType pc);
        bool is_square_attacked(u8 square, Bitboard board[], Bitboard empty_pieces, Color color);
        bool in_check(Bitboard board[], Bitboard empty_pieces, Color color);

        void extract_pawn_captures(vector<Move> &orig, Bitboard boards[], Bitboard board, int offset, u8 opp_pawn, u8 piece, u8 promotion_piece, u8 passant);
        void extract_pawn_moves(vector<Move> &orig, Bitboard board, int offset, u8 piece, u8 promotion_piece);
        void extract_capture_moves(vector<Move> &orig, Bitboard boards[], Bitboard board, u8 from, u8 opp_pawn, u8 piece);
        void extract_moves(vector<Move> &orig, Bitboard board, u8 from, u8 piece);

        void generate_pawn_moves(vector<Move> &moves, Bitboard board[], Color color, int index, int opp_pawn, Bitboard opp_pieces, Bitboard empty_pieces, Bitboard en_passant_bb);
        void generate_knight_moves(vector<Move> &moves, Bitboard board[], int piece_index, int opp_pawn, Bitboard opp_pieces, Bitboard empty_pieces);
        void generate_king_moves(vector<Move> &moves, Bitboard board[], Color color, int piece_index, int opp_pawn, Bitboard all_pieces, Bitboard opp_pieces, Bitboard empty_pieces, CastlingRights crs);
        void generate_sliding_moves(vector<Move> &moves, Bitboard board[], Color color, int piece_index, int opp_pawn, Bitboard opp_pieces, Bitboard empty_pieces);
        
        vector<Move> pseudolegal_moves(Bitboard board[], Color color, CastlingRights crs, u8 eps);
        vector<Move> legal_moves(Bitboard board[], Color color, CastlingRights crs, u8 eps);

        vector<Move> captures_moves(vector<Move> moves);
        vector<Move> promotions_moves(vector<Move> moves);
        vector<Move> order_moves(Bitboard board[], vector<Move> moves, Color color, bool captures_only);

        u64 perft(int depth, Bitboard board[], Color color, TT &tt, CastlingRights &crs, uint8_t &eps, u64 *piece_zob, bool use_loop);

        u64 perft_parallel(int depth, Bitboard board[], Color color, CastlingRights cr, u8 ep, TT &tt, bool use_piece_zob);
};

}

#endif