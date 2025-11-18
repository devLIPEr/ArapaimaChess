#ifndef CGW64_H
#define CGW64_H

namespace arapaimachess{

/// @brief Random Number Generator based on Collatz-Weyl from Tomasz R. Dziala
/// @author Tomasz R. Dziala
/// @link https://arxiv.org/pdf/2312.17043
class CGW64{
    private:
        unsigned long long x, a, weyl, s;
    public:
        CGW64();
        CGW64(unsigned long long seed);
        ~CGW64();
        void seed(unsigned long long seed);
        unsigned long long next();
        unsigned long long splitmix64(unsigned long long seed);
        unsigned long long splitmix63(unsigned long long seed);
};

}

#endif