#ifndef SEARCH_H
#define SEARCH_H

#include "types.h"
#include "transposition_table.h"
#include "move_generator.h"
#include "entry.h"
#include "config.h"

namespace arapaimachess{

class Search{
    private:
        #if defined(MATERIAL_EVAL)
            int evaluation[12] = {
                // Black Pieces
                PAWN_VALUE,
                KNIGHT_VALUE,
                BISHOP_VALUE,
                ROOK_VALUE,
                QUEEN_VALUE,
                KING_VALUE,

                // White Pieces
                PAWN_VALUE,
                KNIGHT_VALUE,
                BISHOP_VALUE,
                ROOK_VALUE,
                QUEEN_VALUE,
                KING_VALUE
            };
        #endif
        MoveGenerator<MAGIC> *move_gen;
        Zobrist *zobrist_table;

        bool null_move;
        bool late_move;
        bool futility;
        bool razoring;

    public:
        int hits = 0;
        Search(MoveGenerator<MAGIC> *move_gen, Zobrist *zobrist_table);
        Search();
        ~Search() = default;

        void set_null_move(bool set);
        void set_late_move(bool set);
        void set_futility(bool set);
        void set_razoring(bool set);

        bool is_mate(Bitboard board[], Color opp, CastlingRights cr, u8 ep);
        bool is_stalemate(Bitboard board[], Color opp, CastlingRights cr, u8 ep);
        bool is_insufficient_material(Bitboard board[]);
        bool is_terminal(Bitboard board[], Color opp, CastlingRights cr, u8 ep);

        int AlphaBeta(unsigned int rule50, atomic<bool> *stop, PVLine *pv, u64 &nodes, int max_depth, int depth, int alpha, int beta, Bitboard board[], Color player, CastlingRights cr, u8 en_passant, TT &tt, vector<Move> search_moves, bool search_order, bool book_hint);
        
        int Quiesce(unsigned int rule50, atomic<bool> *stop, u64 &nodes, int alpha, int beta, Bitboard board[], Color player, CastlingRights cr, u8 en_passant, TT &tt);
};

}

#endif