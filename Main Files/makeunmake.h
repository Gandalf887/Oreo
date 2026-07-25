#pragma once
#include "types.h"
#include "position.h"
#include "move.h"

// ============================================================
// PROCEDURES & FUNCTIONS
// ============================================================

void make_move(Position &pos, const Move &move, UndoInfo &undo);
void unmake_move(Position &pos, const Move &move, const UndoInfo &undo);
Piece piece_on(const Position &pos, Square sq);

