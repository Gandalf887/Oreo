#include "fen.h"
#include "display.h"
#include "movegen.h"
#include "legal.h"
#include "eval.h"
#include "search.h"
#include "zobrist.h"
#include "gui.h"
#include "transposition.h"
#include <iostream>
#include "uci.h"
#include "book.h"

// GUI MODE:
int main()
{
    init_zobrist();
    init_eval();
    tt.clear();
    clear_killers();
    clear_history();
    Position pos = parse_fen(starting_fen());
    init_book(pos);
    run_gui(pos);
    return 0;
}

// UCI MODE:
// int main()
// {
//     init_zobrist();
//     init_eval();
//     tt.clear();
//     clear_killers();
//     clear_history();
//     run_uci();
//     return 0;
// }