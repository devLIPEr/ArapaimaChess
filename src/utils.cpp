#include "types.h"
#include "zobrist.h"
#include "entry.h"
#include <iostream>
#include <vector>
#include <cstring>

#ifndef UTILS_H
#define UTILS_H

using namespace std;
using namespace arapaimachess;

void set_en_passant(u8 &passant, u8 row, u8 col) { passant = (row << 3) | col; }
u8 get_en_passant_row(u8 passant) { return ((passant & 56) >> 3); }
u8 get_en_passant_col(u8 passant) { return passant & 7; }
bool is_en_passant(u8 passant) { return passant != 255; }

/// @brief Get uci square
/// @param col column/file
/// @param row line/rank
/// @param out_of_range if not valid square, used for printing en passant when it hasn't been played
/// @return square string, or "-" when out_of_range
string get_square(u8 col, u8 row, bool out_of_range=false){
    if(out_of_range){
        return "-";
    }
    string letters = "hgfedcba", numbers = "87654321";
    char result[3] = {letters[col], numbers[row], '\0'};
    return result;
}

/// @brief Get castling rights string
/// @param cr castling rights
/// @return castling rights string
string get_castling_rights(CastlingRights cr){
    if(cr == NO_CASTLING){
        return "-";
    }
    char result[5] = {' ', ' ', ' ', ' ', '\0'};
    int i = 0;
    if((cr & WHITE_OO) > 0){
        result[i] = 'K';
        i++;
    }
    if((cr & WHITE_OOO) > 0){
        result[i] = 'Q';
        i++;
    }
    if((cr & BLACK_OO) > 0){
        result[i] = 'k';
        i++;
    }
    if((cr & BLACK_OOO) > 0){
        result[i] = 'q';
    }
    return result;
}

/// @brief Get uci move string (eg. e2e4)
/// @param move move to convert
/// @return move string
string get_move_string(Move move){
    static u8 pieces[6] = {0, 'n', 'b', 'r', 'q', 0};
    u8 from_r = move.get_row_from(), from_c = move.get_col_from(), to_r = move.get_row_to(), to_c = move.get_col_to();
    char promo = move.promotion_piece != 255 ? pieces[move.promotion_piece] : '\0';
    string sq1 = get_square(from_c, from_r), sq2 = get_square(to_c, to_r);
    char uci_move[6] = {sq1[0], sq1[1], sq2[0], sq2[1], promo, '\0'};
    return (move.from != 255 ? uci_move : "\0");
}

/// @brief Get move index from a uci move string
/// @param move uci move string
/// @return move index
u16 get_move_idx(string move){
    u16 key = 0;
    key |= ((move[0]-'a') & 7) << 9;
    key |= ((8-(move[1]-'0')) & 7) << 6;
    key |= ((move[2]-'a') & 7) << 3;
    key |= ((8-(move[3]-'0')) & 7);
    return key;
}

/// @brief print list of moves to stdin
/// @param moves list of moves
void print_moves(vector<Move> moves){
    for(Move move: moves){
        cout << get_move_string(move) << ' ';
    }
    printf("\n");
}

/// @brief print a given bitboard to stdin
/// @param board bitboard to print
void print_bitboard(Bitboard board){
    for(int i = 0; i < 8; i++){
        for(int j = 0; j < 8; j++){
            printf("%lld ", (board >> (i*8+j)) & 1);
        }
        printf("\n");
    }
}

/// @brief Compute zobrist key
/// @param zobrist_table reference to Zobrist object
/// @param board array of bitboards
/// @param color current player
/// @param cr castling rights
/// @param passant en passant square
/// @param piece_zob reference to last key value for fast update
/// @param use_loop if true loop through all bitboards to compute key, otherwise use fast update
/// @return zobrist key
u64 zob_key(Zobrist &zobrist_table, Bitboard board[12], Color color, CastlingRights cr, uint8_t passant, u64 *piece_zob=NULL, bool use_loop=true){
    u64 h = 0;
    if(color == BLACK){
        h ^= zobrist_table[zobrist_table.black_to_move];
    }
    if(cr & WHITE_OO){
        h ^= zobrist_table[zobrist_table.black_to_move+1];
    }
    if(cr & WHITE_OOO){
        h ^= zobrist_table[zobrist_table.black_to_move+2];
    }
    if(cr & BLACK_OO){
        h ^= zobrist_table[zobrist_table.black_to_move+3];
    }
    if(cr & BLACK_OOO){
        h ^= zobrist_table[zobrist_table.black_to_move+4];
    }
    if(is_en_passant(passant)){
        h ^= zobrist_table[zobrist_table.black_to_move+5+(passant & 7)];
    }
    if(use_loop){
        u64 x = 0;
        for(int i = 0; i < 12; i++){
            Bitboard bb = board[i];
            int index;
            while(bb){
                index = __builtin_ctzll(bb);
                x ^= zobrist_table[i*64+index];
                bb &= bb-1;
            }
        }
        if(piece_zob != NULL)
            *(piece_zob) = x;
        h ^= x; 
    }else if(piece_zob != NULL){
        h ^= *(piece_zob);
    }
    return h;
}

/// @brief parse hex string
/// @param value hex string
/// @return 64 bit value equivalent to the hex string
u64 read_hex(char *value){
    u64 val = 0;
    int len = strlen(value);
    if(len > 8){
        len = 8;
    }
    for(int i = 0; i < len; i++){
        val |= value[i] << (56-i*8);
    }
    return val;
}

/// @brief Calculates how many entries to fill a given amount of MBs
/// @param MB memory amount to use
/// @return amount of entries
int MB_to_TT(int MB){
    return MB*1024*1024 / sizeof(Entry);
}

/// @brief Check if a player has only pawns
/// @param board array of bitboards
/// @param player player to check
/// @return true if player only have pawns, false otherwise
bool has_only_pawns(Bitboard board[12], Color player){
    int idx = player*6;
    if(board[idx+1] != 0 || board[idx+2] != 0 || board[idx+3] != 0 || board[idx+4] != 0) return false;
    return true;
}

/// @brief Adapt the bitboard indexing to Fathom indexing
/// @param bb bitboard to convert
/// @return converted bitboard
Bitboard bswap(Bitboard bb){
    return __builtin_bswap64(bb);
}

#endif