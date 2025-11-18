#include "cgw64.h"

using namespace std;

namespace arapaimachess{

CGW64::CGW64(){ this->seed(8428114415715405298ULL); }
CGW64::CGW64(unsigned long long seed){ this->seed(seed); }
CGW64::~CGW64(){}

unsigned long long CGW64::splitmix64(unsigned long long seed){
    unsigned long long z = (seed += 0x9e3779b97f4a7c15);
    z = (z ^ (z >> 30)) * 0xbf58476d1ce4e5b9;
    z = (z ^ (z >> 27)) * 0x94d049bb133111eb;
    return z ^ (z >> 31);
}
unsigned long long CGW64::splitmix63(unsigned long long seed){
    unsigned long long z = (seed += 0x9e3779b97f4a7c15) & 0x7fffffffffffffff;
    z = ((z ^ (z >> 30)) * 0xbf58476d1ce4e5b9) & 0x7fffffffffffffff;
    z = ((z ^ (z >> 27)) * 0x94d049bb133111eb) & 0x7fffffffffffffff;
    return z ^ (z >> 31);
}

void CGW64::seed(unsigned long long seed){
    this->a = 0;
    this->weyl = 0;
    this->x = this->splitmix64(seed);
    this->s = this->splitmix63(seed) | 1;
}

unsigned long long CGW64::next(){
    this->x = (this->x >> 1) * ((this->a += this->x) | 1) ^ (this->weyl += this->s);
    return this->a >> 48 ^ this->x;
}

}