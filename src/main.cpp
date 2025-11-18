#include <iostream>

#include "engine.h"
#include "uci.h"
#include "utils.h"
#include "zobrist.h"
#include "board.h"
#include "move_generator.h"
#include "search.h"
#include "transposition_table.h"
#include "entry.h"
#include "config.h"

using namespace std;
using namespace arapaimachess;

int main(){
    u64 seed = 8428114415715405298ULL;

    read_nn("./chess.nn");
    TT tt = TT(MB_to_TT(64));
    Zobrist zobrist_table = Zobrist(seed);
    MAGIC magic = MAGIC();
    MoveGenerator<MAGIC> move_generator = MoveGenerator<MAGIC>(&zobrist_table, &magic, 3);
    Search search = Search(&move_generator, &zobrist_table);
    Board<MAGIC> board = Board<MAGIC>(&zobrist_table, &move_generator);
    Engine engine = Engine(&tt, &zobrist_table, &move_generator, &search, &board);
    UCI uci = UCI(&engine);

    uci.read();

    return 0;
}