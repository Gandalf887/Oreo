#pragma once
#include "position.h"
#include "movegen.h"

// ============================================================
// PROCEDURES & FUNCTIONS
// ============================================================

void init_eval();
int evaluate(const Position &pos);
int evaluate_pawns(const Position &pos, int phase);
int evaluate_king_safety(const Position &pos, int phase);
int game_phase(const Position &pos);
int evaluate_mopup(const Position &pos, int phase);
int evaluate_development(const Position &pos, int phase);