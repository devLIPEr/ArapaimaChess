#include "fixed.h"

using namespace std;

namespace arapaimachess{

FIXED_Magic::FIXED_Magic(){
    generate_magic_sliders();
    generate_magic_knights();
    generate_magic_kings();
}

/// @brief Based on the implementation of magic bitboard using PEXT/PDEP from https://github.com/syzygy1
void FIXED_Magic::init(Magic *magic_init, Bitboard *attacks[], Bitboard magics[], Bitboard masks[], int dir[][2], bool is_bishop){
    int squares[12];
    int sq88, num;
    Bitboard bb, bb2, edges;

    for(int sq = 0; sq < 64; sq++){
        magics[sq] = magic_init[sq].magic;
        attacks[sq] = &attacks_table[magic_init[sq].index];

        // Board edges are not considered in the relevant occupancies
        edges = ((RANK_1 | RANK_8) & ~Ranks(RANK_8 << (sq >> 3)*8)) | ((FILE_A | FILE_H) & ~Files(FILE_A << (sq & 7)));

        sq88 = sq + (sq & ~7);
        bb = 0;
        for(int i = 0; i < 4; i++){
            if((sq88 + dir[i][1]) & 0x88) continue;
            for(int d = 2; !((sq88 + d * dir[i][1]) & 0x88); d++){
                bb |= 1ULL << (sq + (d - 1) * dir[i][0]);
            }
        }
        masks[sq] = bb & ~edges;

        num = 0;
        while(bb){
            squares[num++] = __builtin_ctzll(bb);
            bb &= bb-1;
        }

        for(int i = 0; i < (1 << num); i++){
            bb = 0;
            for(int j = 0; j < num; j++){
                if(i & (1 << j)){
                    bb |= 1ULL << squares[j];
                }
            }
            bb2 = 0;
            for(int j = 0; j < 4; j++){
                for(int d = 1; !((sq88 + d * dir[j][1]) & 0x88); d++){
                    bb2 |= 1ULL << (sq + d * dir[j][0]);
                    if (bb & (1ULL << (sq + d * dir[j][0]))) break;
                }
            }
            attacks[sq][(is_bishop) ? magic_index_bishop(sq, bb) : magic_index_rook(sq, bb)] = bb2;
        }
    }
}

void FIXED_Magic::generate_magic_sliders(){
    int m_bishop_dir[4][2] = {
        { -9, -17 }, { -7, -15 }, { 7, 15 }, { 9, 17 }
    };
    int m_rook_dir[4][2] = {
        { -8, -16 }, { -1, -1 }, { 1, 1}, { 8, 16 }
    };

    init(rook_magics, RookAttacks, RookMagics, RookMasks,
                m_rook_dir, 0);
    init(bishop_magics, BishopAttacks, BishopMagics, BishopMasks,
                m_bishop_dir, 1);
}
constexpr void FIXED_Magic::generate_magic_knights(){
    for(int i = 0; i < 64; i++){
        Bitboard knight = (1ULL << i);
        knights[i] = 0;
        
        knights[i] |= (knight << 17) & ~FILE_A;
        knights[i] |= (knight << 10) & ~(FILE_A | FILE_B);
        knights[i] |= (knight >>  6) & ~(FILE_A | FILE_B);
        knights[i] |= (knight >> 15) & ~FILE_A;
        knights[i] |= (knight << 15) & ~FILE_H;
        knights[i] |= (knight <<  6) & ~(FILE_G | FILE_H);
        knights[i] |= (knight >> 10) & ~(FILE_G | FILE_H);
        knights[i] |= (knight >> 17) & ~FILE_H;
    }
}
constexpr void FIXED_Magic::generate_magic_kings(){
    for(int i = 0; i < 64; i++){
        Bitboard king = (1ULL << i);
        kings[i] = 0;

        kings[i] |= (king << 1) & ~(FILE_A);
        kings[i] |= (king << 9) & ~(FILE_A);
        kings[i] |= (king >> 7) & ~(FILE_A);
        kings[i] |= (king >> 1) & ~(FILE_H);
        kings[i] |= (king << 7) & ~(FILE_H);
        kings[i] |= (king >> 9) & ~(FILE_H);
        kings[i] |= (king << 8);
        kings[i] |= (king >> 8);
    }
}

int FIXED_Magic::magic_index_bishop(int s, Bitboard occupied){
  return ((occupied & BishopMasks[s]) * BishopMagics[s]) >> (55);
}
int FIXED_Magic::magic_index_rook(int s, Bitboard occupied){
  return ((occupied & RookMasks[s]) * RookMagics[s]) >> (52);
}

Bitboard FIXED_Magic::get_attack_bishop(int s, Bitboard occupied){
  return BishopAttacks[s][magic_index_bishop(s, occupied)];
}
Bitboard FIXED_Magic::get_attack_rook(int s, Bitboard occupied){
  return RookAttacks[s][magic_index_rook(s, occupied)];
}
Bitboard FIXED_Magic::get_attack_knight(int sq){
    return knights[sq];
}
Bitboard FIXED_Magic::get_attack_king(int sq){
    return kings[sq];
}

}