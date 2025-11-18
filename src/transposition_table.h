#ifndef TT_H
#define TT_H

#include <atomic>
#include "entry.h"

using namespace std;

namespace arapaimachess{

class TT{
    private:
        atomic<Entry> *entries = NULL;
        int table_size;
    public:
        TT();
        TT(int size);
        ~TT();
        int add(unsigned long long key, const Entry &entry);
        Entry atomic_read(unsigned long long index);
        Entry operator[](unsigned long long index); 
        const Entry operator[](unsigned long long index) const;
        void clear();
        void resize(int size);
};

}

#endif