#pragma once
#include "position.h"
#include <string>

// ============================================================
// PROCEDURES & FUNCTIONS
// ============================================================

void print_board(const Position &pos);
std::string square_to_string(Square sq);
std::string piece_to_string(PieceType pt);