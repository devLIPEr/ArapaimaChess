#include <iostream>
#include <fstream>
#include <sstream>
#include <algorithm>
#include <cstring>

#include "book.h"
#include "utils.h"

namespace arapaimachess{

/// @brief Create a opening book
/// @param path file path to the opening book
/// @param seed seed for the random number generator
Book::Book(string path, u64 seed){
    this->rng = CGW64(seed);
    memset(chances, 0, (1 << 12)*sizeof(int));
    this->read_book(path);
    this->first_move = true;
}

/// @brief Reset the current position in the opening book
void Book::reset_book(){
    this->opening = this->start_book;
    this->first_move = true;
    this->valid_opening = true;
}

/// @brief Print the trie
/// @param t trie to print
/// @param s string to print before the trie
void print_trie(Trie *t, string s){
    if(t != NULL){
        for(Trie* child : t->children){
            print_trie(child, s+t->move+" ");
        }
        if(t->n == 0){
            cout << s << t->move << '\n';
        }
    }
}

/// @brief Reads and parse a book from a file
/// @param path path of the book to read
void Book::read_book(string path){
    ifstream book_file(path);
    vector<string> lines;
    if(book_file.is_open()){
        do{
            string line;
            getline(book_file, line);
            lines.push_back(line);
        }while(!book_file.eof());

        sort(lines.begin(), lines.end());

        start_book = new Trie(vector<Trie*>(), "(none)", 0);
        opening = start_book;
        for(string line : lines){
            istringstream stream(line);

            Trie *t = opening;
            string move;
            bool found;
            u8 idx;
            u8 depth = 0;
            while(stream >> move){
                if(depth == 0){
                    u16 i = get_move_idx(move);
                    while(i < 4096){
                        chances[i]++;
                        i++;
                    }
                }
                depth++;
                found = false;
                idx = 0;
                for(int i = 0; i < t->n && !found; i++){
                    if(string(t->children[i]->move) == move){
                        found = true;
                        idx = i;
                    }
                }
                if(!found){
                    Trie *t1 = new Trie(vector<Trie*>(), move, 0);
                    t->children.push_back(t1);
                    idx = t->n;
                    t->n++;
                }
                if(depth > max_depth){
                    max_depth = depth;
                }
                t = t->children[idx];
            }
        }
    }
}

/// @brief Gets a move for the current position
/// @param go_deep if true go to the next depth else stay at the current depth
/// @return move string if there is a move to be played from the current position, (none) otherwise
string Book::get_move(bool go_deep){
    if(valid_opening){
        if(first_move){
            if(opening != NULL && opening->n > 0){
                int chance = rng.next() % (chances[4095]+1);
                u8 op = 0;
                int i = 4095;
                while(i >= 0 && chance < chances[i]){
                    i--;
                }
                i++;
                while(i >= 0 && chances[i] == chances[i-1]){
                    i--;
                }
                while(op < opening->n && get_move_idx(opening->children[op]->move) != i){
                    op++;
                }
                if(op == opening->n){
                    op = rng.next() % opening->n;
                }
                string move = opening->children[op]->move;
                if(go_deep){
                    first_move = false;
                    opening = opening->children[op];
                }
                return move;
            }
        }else{
            if(opening != NULL && opening->n > 0){
                u8 op = rng.next() % opening->n;
                string move = opening->children[op]->move;
                if(go_deep){
                    opening = opening->children[op];
                }
                return move;
            }
        }
    }
    return "(none)";
}

/// @brief Make a move in the trie
/// @param move move string of the movie in uci notation
void Book::go_move(string move){
    first_move = false;
    if(opening != NULL && opening->n > 0){
        for(int i = 0; i < opening->n; i++){
            if(string(opening->children[i]->move) == move){
                opening = opening->children[i];
                return;
            }
        }
    }
    valid_opening = false;
}

}