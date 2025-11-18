#include <iostream>
#include <cstring>
#include <omp.h>

#include "evaluate.h"
#include "config.h"

using namespace std;

namespace arapaimachess{

/// @brief Evaluate a position using the material array.
/// @param board array of bitboards
/// @param material material value for each piece in each bitboard
/// @param n size of board/material array
/// @param color_change_idx index where the color change from black to white [0, n-1]
/// @return evaluation of the position
int material_evaluate(Bitboard board[], int material[], int n, int color_change_idx){
    int eval = 0;
    for(int i = 0; i < color_change_idx; i++){
        eval -= __builtin_popcountll(board[i])*material[i];
    }
    for(int i = color_change_idx; i < n; i++){
        eval += __builtin_popcountll(board[i])*material[i];
    }
    return eval;
}

/// @brief Read a binary file that contains the weights and biases of a nn
/// @param path path to nn file
void read_nn(string path){
    ifstream input(path, ios::binary);

    #if defined(NN_EVAL)
        input.read(reinterpret_cast<char*>(w1), 781*16*sizeof(float));
        input.read(reinterpret_cast<char*>(b1), 1*16*sizeof(float));
        input.read(reinterpret_cast<char*>(w2), 16*8*sizeof(float));
        input.read(reinterpret_cast<char*>(b2), 1*8*sizeof(float));
        input.read(reinterpret_cast<char*>(w3), 8*1*sizeof(float));
        input.read(reinterpret_cast<char*>(b3), 1*1*sizeof(float));
    #endif
}

/// @brief Multiply matrix m1 and m2 then stores at r
/// @param m1 matrix 1
/// @param m2 matrix 2
/// @param r result matrix
/// @param m columns of first matrix and lines of second matrix
/// @param q columns of second matrix
void mul(float *m1, float *m2, float *r, int m, int q){
    #if defined(USE_AVX_NN)
        const int VECTOR_SIZE = 8;

        for(int j = 0; j < q; j++){
            const float *W_row = m2 + j*m;

            __m256 acc_vec = _mm256_setzero_ps();

            int k = 0;
            for(; k + VECTOR_SIZE <= m; k += VECTOR_SIZE){
                __m256 x_vec = _mm256_loadu_ps(m1 + k);
                __m256 w_vec = _mm256_loadu_ps(W_row + k);
                __m256 prod_vec = _mm256_mul_ps(x_vec, w_vec);
                acc_vec = _mm256_add_ps(acc_vec, prod_vec);
            }

            __m128 lo = _mm256_castps256_ps128(acc_vec);
            __m128 hi = _mm256_extractf128_ps(acc_vec, 1);

            __m128 sum128 = _mm_add_ps(lo, hi);
            sum128 = _mm_hadd_ps(sum128, sum128);
            sum128 = _mm_hadd_ps(sum128, sum128);

            float final_sum = _mm_cvtss_f32(sum128);

            for(; k < m; k++){
                final_sum += m1[k] * W_row[k];
            }

            r[j] = final_sum;
        }
    #else
        for(int j = 0; j < q; j++){
            for(int k = 0; k < m; k++){
                r[j] += m1[k] * m2[j*m + k];
            }
        }
    #endif
}

/// @brief Sum m2 into m1
/// @param m1 matrix 1
/// @param m2 matrix 2
/// @param m number of columns
void sum(float *m1, float *m2, int m){
    for(int j = 0; j < m; j++){
        m1[j] += m2[j];
    }
}

/// @brief Apply SCReLU activation function
/// @param m1 matrix 1
/// @param m number of columns
void act(float *m1, int m){
    for(int j = 0; j < m; j++){
        if(m1[j] <= 0.0f){
            m1[j] = 0.0f;
        }else if(m1[j] >= 1.0f){
            m1[j] = 1.0f;
        }else{
            m1[j] *= m1[j];
        }
    }
}

/// @brief Evaluate a position using a simple MLP
/// @param board array of bitboards
/// @param cr castling rights
/// @param ep en passant square
/// @param player current player
/// @return evaluation of the position [-20000, 20000]
int nn_evaluate(Bitboard board[], CastlingRights cr, u8 ep, Color player){
    #if defined(NN_EVAL)
        memset(input, 0, sizeof(float)*781);
        memset(r1, 0, sizeof(float)*16);
        memset(r2, 0, sizeof(float)*8);
        memset(r3, 0, sizeof(float)*1);
        for(int i = 0; i < 12; i++){
            uint64_t b = board[i];
            int offset = i * 64;
            while(b){
                int idx = __builtin_ctzll(b);
                input[offset + idx] = 1;
                b &= b - 1; 
            }
        }
        if(cr & WHITE_OO) input[768] = 1;
        if(cr & WHITE_OOO) input[769] = 1;
        if(cr & BLACK_OO) input[770] = 1;
        if(cr & BLACK_OOO) input[771] = 1;
        if(ep != 255) input[772 + ep] = 1;
        if(player == BLACK) input[780] = 1;

        // Layer 1
        mul((float *)input, (float *)w1, (float *)r1, 781, 16);
        sum((float *)r1, (float *)b1, 16);
        act((float *)r1, 16);

        // Layer 2
        mul((float *)r1, (float *)w2, (float *)r2, 16, 8);
        sum((float *)r2, (float *)b2, 8);
        act((float *)r2, 8);

        // Layer 3
        mul((float *)r2, (float *)w3, (float *)r3, 8, 1);
        sum((float *)r3, (float *)b3, 1);

        return (int)(r3[0] * (40000.0f) - 20000.0f);
    #else
        return 0;
    #endif
}

#if defined(NN_EVAL)
    float input[781];

    float w1[16][781];
    float b1[16];
    float r1[16];

    float w2[8][16];
    float b2[8];
    float r2[8];
    
    float w3[8];
    float b3[1];
    float r3[1];
#else
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

int material_value[12] = {
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

}