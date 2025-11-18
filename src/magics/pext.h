#ifndef PEXT_H
#define PEXT_H

#include "../types.h"
#include "../config.h"

#ifdef USE_INTEL_PEXT
    #define pext _pext_u64
    #define pdep _pdep_u64
#endif

#if defined(USE_PEXT)
    u64 pext(u64 b, u64 mask);
    u64 pdep(u64 v, u64 mask);
#endif

namespace arapaimachess{

struct BMI2Info{
    u16 *data;
    Bitboard mask1, mask2;
};

class PEXT_Magic{
    private:
        Bitboard knights[64], kings[64];
        BMI2Info rook_bmi2[64], bishop_bmi2[64];
    public:
        u16 attacks_table[107648];
        PEXT_Magic();
        ~PEXT_Magic() = default;

        int init(int idx, i8 dir[][2], BMI2Info *info);
        void generate_magic_sliders();
        constexpr void generate_magic_knights();
        constexpr void generate_magic_kings();

        Bitboard get_attack_rook(int sq, Bitboard occ);
        Bitboard get_attack_bishop(int sq, Bitboard occ);
        Bitboard get_attack_knight(int sq);
        Bitboard get_attack_king(int sq);
};

}

#endif