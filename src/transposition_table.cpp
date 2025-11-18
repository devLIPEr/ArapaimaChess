#include <cstring>
#include <cstdlib>
#include <atomic>

#include "transposition_table.h"
#include "entry.h"

using namespace std;

namespace arapaimachess{

TT::TT(){
    this->table_size = 1048573;
    this->entries = new atomic<Entry>[this->table_size];
}

TT::TT(int size){
    this->table_size = size;
    this->entries = new atomic<Entry>[this->table_size];
}

TT::~TT(){
    delete[] this->entries;
    this->entries = NULL;
}

/// @brief Add an entry to the table
/// @param key zobrist key
/// @param entry entry object
/// @return
int TT::add(unsigned long long key, const Entry &entry){
    int pos = key % this->table_size;

    Entry curr_entry = this->entries[pos].load(memory_order_acquire);

    if(curr_entry.count < entry.count){
        while(true){
            if(this->entries[pos].compare_exchange_strong(curr_entry, entry, memory_order_acquire)){
                return 0; 
            }
        }
    }
    return 0;
}

/// @brief Atomically read a value from the table
/// @param index zobrist key
/// @return entry from the table
Entry TT::atomic_read(unsigned long long index){ return this->entries[index % this->table_size].load(memory_order_acquire); }

/// @brief Reads a value from the table
/// @param index zobrist key
/// @return entry from the table
Entry TT::operator[](unsigned long long index){ return this->entries[index % this->table_size]; }

/// @brief Reads a value from the table
/// @param index zobrist key
/// @return entry from the table
const Entry TT::operator[](unsigned long long index) const { return this->entries[index % this->table_size]; }

/// @brief Clear the entire table
void TT::clear(){
    for(int i = 0; i < table_size; i++){
        this->entries[i] = Entry();
    }
}

/// @brief Resize the table
/// @param size new size in number of entries
void TT::resize(int size){
    delete[] this->entries;
    this->table_size = size;
    this->entries = new atomic<Entry>[this->table_size];
}

}