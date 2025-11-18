#include "pext.h"

using namespace std;

#ifndef USE_INTEL_PEXT
    u64 pext(u64 b, u64 mask){
        u64 result = 0;
        for(u64 bit = 1; mask; bit <<= 1){
            if(b & mask & -mask) result |= bit;
            mask &= mask-1;
        }
        return result;
    }

    u64 pdep(u64 v, u64 mask){
        u64 result = 0;
        for(u64 bit = 1; mask; bit <<= 1){
            if(v & bit) result |= mask & -mask;
            mask &= mask-1;
        }
        return result;
    }
#endif

namespace arapaimachess{

PEXT_Magic::PEXT_Magic(){
    generate_magic_sliders();
    generate_magic_knights();
    generate_magic_kings();
}

/// @brief Implementation of magic bitboard using PEXT/PDEP from https://github.com/syzygy1
int PEXT_Magic::init(int idx, i8 dir[][2], BMI2Info *info){
    int squares[12];
    int sq88, num;
    Bitboard bb, bb2;

    for(int sq = 0; sq < 64; sq++){
        info[sq].data = &attacks_table[idx];

        // calculate mask
        sq88 = sq + (sq & ~7);
        bb = 0;
        for(int i = 0; i < 4; i++){
            if((sq88 + dir[i][1]) & 0x88) continue;
            for(int d = 2; !((sq88 + d * dir[i][1]) & 0x88); d++){
                bb |= (1ULL << (sq + (d - 1) * dir[i][0]));
            }
        }
        info[sq].mask1 = bb;

        num = 0;
        while(bb){
            squares[num++] = __builtin_ctzll(bb);
            bb &= bb-1;
        }

        // loop through all possible occupations within the mask
        // and calculate the corresponding attack sets
        for(int i = 0; i < (1 << num); i++){
            bb = 0;
            for(int j = 0; j < num; j++){
                if(i & (1 << j)){
                    bb |= (1ULL << squares[j]);
                }
            }
            bb2 = 0;
            for(int j = 0; j < 4; j++){
                for(int d = 1; !((sq88 + d * dir[j][1]) & 0x88); d++){
                    bb2 |= (1ULL << (sq + d * dir[j][0]));
                    if (bb & (1ULL << (sq + d * dir[j][0]))) break;
                }
            }
            if(i == 0){
                info[sq].mask2 = bb2;
            }
            attacks_table[idx++] = pext(bb2, info[sq].mask2);
        }
    }

    return idx;
}

void PEXT_Magic::generate_magic_sliders(){
    i8 m_bishop_dir[4][2] = {
        { -9, -17 }, { -7, -15 }, { 7, 15 }, { 9, 17 }
    };
    i8 m_rook_dir[4][2] = {
        { -8, -16 }, { -1, -1 }, { 1, 1}, { 8, 16 }
    };
    int i;
    for(i = 0; i < 97264; i++){
        attacks_table[i] = 0ULL;
    }
    i = init(0, m_bishop_dir, bishop_bmi2);
    init(i, m_rook_dir, rook_bmi2);
}
constexpr void PEXT_Magic::generate_magic_knights(){
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
constexpr void PEXT_Magic::generate_magic_kings(){
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

Bitboard PEXT_Magic::get_attack_rook(int sq, Bitboard occ){
    BMI2Info *info = &rook_bmi2[sq];
    return pdep(info->data[pext(occ, info->mask1)], info->mask2);
}
Bitboard PEXT_Magic::get_attack_bishop(int sq, Bitboard occ){
    BMI2Info *info = &bishop_bmi2[sq];
    return pdep(info->data[pext(occ, info->mask1)], info->mask2);
}
Bitboard PEXT_Magic::get_attack_knight(int sq){
    return knights[sq];
}
Bitboard PEXT_Magic::get_attack_king(int sq){
    return kings[sq];
}

}