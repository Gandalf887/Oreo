
#pragma once
#include "position.h"
#include "move.h"
#include <string>

// ============================================================
// PROCEDURES & FUNCTIONS
// ============================================================

std::string sq_to_uci(Square sq);
std::string move_to_uci(const Move &move);
void parse_position(Position &pos, const std::string &line, uint64_t *game_history, int &game_history_count);
void parse_go(Position &pos, const std::string &line, uint64_t *game_history, int game_history_count);
void run_uci();
