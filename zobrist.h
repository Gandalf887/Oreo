#pragma once 
#include "position.h"
#include <cstdint>

// ============================================================
// PROCEDURES & FUNCTIONS
// ============================================================

void init_zobrist();
uint64_t hash_position(const Position &pos);

//1.13.0
extern uint64_t piece_keys[PIECE_COUNT][64];
extern uint64_t side_key;
extern uint64_t castling_keys[16];
extern uint64_t en_passant_keys[8];