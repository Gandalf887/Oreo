#pragma once
#include "types.h"
#include "position.h"
#include "move.h"
#include "makeunmake.h"

// ============================================================
// PROCEDURES & FUNCTIONS
// ============================================================

bool is_square_attacked(const Position &pos, Square square, Colour colour);
bool is_in_check(const Position &pos, Colour colour);
void filter_legal_moves(Position &pos, MoveList &list);

