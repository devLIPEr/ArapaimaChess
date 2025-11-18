#ifndef CONFIG_H
#define CONFIG_H

#define NULL_MOVE_DEPTH 2
#define NULL_REDUCTION 2

#define MAX_LATE_REDUCTION 4

#define MAX_HISTORY (1 << 24)

/// Use only one of the MAGIC's define instruction

/// @brief --- Use intel's pext instruction
#define USE_INTEL_PEXT
/// @brief --- Use a software pext implementation
// #define USE_PEXT
/// @brief --- Use fixed magic to generate moves
// #define USE_FIXED

/// @brief --- Use simple material evaluation
// #define MATERIAL_EVAL
/// @brief --- Use neural network evaluation
#define NN_EVAL
/// @brief --- Use AVX instructions to make neural network evaluation
#define USE_AVX_NN

#if defined(USE_AVX_NN) || defined(USE_INTEL_PEXT)
    #include <immintrin.h>
#endif

#if defined(USE_PEXT) || defined(USE_INTEL_PEXT)
    #include "./magics/pext.h"
    #define MAGIC PEXT_Magic
#else
    #if defined(USE_FIXED)
        #include "./magics/fixed.h"
        #define MAGIC FIXED_Magic
    #else
        #include "./magics/fixed.h"
        #define MAGIC FIXED_Magic
    #endif
#endif

#include "evaluate.h"

#if defined(MATERIAL_EVAL)
    #define evaluate material_evaluate
#elif defined(NN_EVAL)
    #define evaluate nn_evaluate
#else
    #define evaluate material_evaluate
#endif

#endif