#ifndef BOARD_H
#define BOARD_H

#include "types.h"
#include "move_generator.h"
#include "zobrist.h"
#include <iostream>

using namespace std;

namespace arapaimachess{

template <typename Magic>
class Board{
    private:
        u8 piece[115], piece_type[115];
        Zobrist *zobrist_table;
        MoveGenerator<Magic> *move_generator;
    public:
        Bitboard board[12];
        Color curr_player;
        CastlingRights castling_rights;
        unsigned int curr_turn;
        u8 en_passant;
        unsigned int rule50 = 0;

        Board();
        Board(Zobrist *zobrist_table, MoveGenerator<Magic> *move_generator);
        Board(Zobrist *zobrist_table, MoveGenerator<Magic> *move_generator, string board_fen);
        ~Board() = default;
        
        constexpr void add_pieces();
        void initialize_board(string fen);
        void print_board(bool print_extra, bool print_bb);
        void print_board(bool print_ranks, bool print_files, bool print_extra, bool print_bb);
        
        bool is_square_attacked(u8 square, Bitboard empty_pieces);
        bool is_square_attacked(u8 square, Bitboard board[], Bitboard empty_pieces, Color color);
        bool in_check(Bitboard empty_pieces);
        bool in_check(Bitboard board[], Bitboard empty_pieces, Color color);

        static void do_move(Bitboard board[], Move move, Color color, CastlingRights &crs, u8 &eps);
        void do_move(Move move);

        u64 zob_hash();

        string get_board();

        unsigned int count_pieces();
        static unsigned int count_pieces(Bitboard board[]);
};


}

#endif