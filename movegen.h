#pragma once
#include "move.h"
#include "position.h"
#include "makeunmake.h"
#include "search.h"

// ============================================================
// PROCEDURES & FUNCTIONS
// ============================================================

void generate_moves(const Position &pos, MoveList &list);
void generate_king_moves(const Position &pos, MoveList &list);
void generate_knight_moves(const Position &pos, MoveList &list);
void generate_sliding_moves(const Position &pos, MoveList &list);
void generate_pawn_moves(const Position &pos, MoveList &list);

Bitboard rook_attacks(Square sq, Bitboard occupied);
Bitboard bishop_attacks(Square sq, Bitboard occupied);

// performance upgrades 1.1.0
int score_move(const Position &pos, const Move &move, int depth); // performance upgrade 1.7.0 add depth int
void order_moves(const Position &pos, MoveList &list, int depth); // performance upgrade 1.7.0 add depth int

inline Bitboard knight_attacks_bb(int sq)
{
    Bitboard knight = 1ULL << sq;
    Bitboard attacks = 0ULL;
    attacks |= (knight << 17) & ~0x0101010101010101ULL;
    attacks |= (knight << 15) & ~0x8080808080808080ULL;
    attacks |= (knight << 10) & ~0x0303030303030303ULL;
    attacks |= (knight << 6)  & ~0xC0C0C0C0C0C0C0C0ULL;
    attacks |= (knight >> 15) & ~0x0101010101010101ULL;
    attacks |= (knight >> 17) & ~0x8080808080808080ULL;
    attacks |= (knight >> 6)  & ~0x0303030303030303ULL;
    attacks |= (knight >> 10) & ~0xC0C0C0C0C0C0C0C0ULL;
    return attacks;
}