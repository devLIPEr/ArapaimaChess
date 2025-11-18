#ifndef EVALUATE_H
#define EVALUATE_H

#include "types.h"
#include "config.h"
#include <fstream>

using namespace std;

namespace arapaimachess{

int material_evaluate(Bitboard board[], int material[], int n, int color_change_idx);

void read_nn(string path);
void mul(float *m1, float *m2, float *r, int m, int q);
void sum(float *m1, float *m2, int m);
void act(float *m1, int m);
int nn_evaluate(Bitboard board[], CastlingRights cr, u8 ep, Color player);

#if defined(NN_EVAL)
    extern float input[781];

    extern float w1[16][781];
    extern float b1[16];
    extern float r1[16];

    extern float w2[8][16];
    extern float b2[8];
    extern float r2[8];

    extern float w3[8];
    extern float b3[1];
    extern float r3[1];
#else
    extern int evaluation[12];
#endif

extern int material_value[12];

}

#endif