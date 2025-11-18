#include "types.h"
#include "zobrist.h"
#include <vector>
#include <cstring>
#include <iostream>

#ifndef UTILS_H
#define UTILS_H

using namespace std;
using namespace arapaimachess;

template <typename T>
bool in_range(T val, T min, T max){ return val >= min && val <= max; }
template <typename T>
bool in_range(T val, u8 min, u8 max){ return val >= min && val <= max; }

void set_en_passant(u8 &passant, u8 row, u8 col);
u8 get_en_passant_row(u8 passant);
u8 get_en_passant_col(u8 passant);
bool is_en_passant(u8 passant);

string get_square(u8 col, u8 row, bool out_of_range=false);

string get_castling_rights(CastlingRights cr);

u16 get_move_idx(string move);

string get_move_string(Move move);

void print_moves(vector<Move> moves);

void print_bitboard(Bitboard board);

u64 zob_key(Zobrist &zobrist_table, Bitboard board[12], Color color, CastlingRights cr, uint8_t passant, u64 *piece_zob=NULL, bool use_loop=true);

/// @brief Read char values and convert to type T
/// @tparam T type to convert to
/// @param value char array of bytes to convert
/// @return value converted from array
template <typename T>
T char_to_bytes(char *value){
    T val = 0;
    int len = strlen(value);
    for(int i = 0; i < len || i < sizeof(T); i++){
        val *= 10;
        val += (value[i]-48);
    }
    return val;
}

u64 read_hex(char *value);

int MB_to_TT(int MB);

bool has_only_pawns(Bitboard board[12], Color player);

Bitboard bswap(Bitboard bb);

#endif