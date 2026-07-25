#pragma once
#include "position.h"
#include "movegen.h"
#include "legal.h"
#include "uci.h"
#include <unordered_map>
#include <string>
#include <vector>

// ============================================================
// BOOK MOVE ENTRY
// ============================================================
// Represents a single candidate move stored in the opening book for a given
// position. weight controls how often the move is chosen when multiple book
// moves exist (higher = more likely), and priority allows certain moves to be
// forced over others regardless of weight (e.g. a theoretically critical reply).
struct BookMove
{
    std::string uci;
    int weight;
    int priority;
};

// ============================================================
// BOOK TABLE
// ============================================================
// Maps Zobrist hashes to a list of BookMove candidates for that position.
// Using the hash as the key means lookups are O(1) and the book integrates
// naturally with the engine's existing hashing infrastructure — no separate
// position encoding is needed.
extern std::unordered_map<uint64_t, std::vector<BookMove>> opening_book;

// Set to true once the engine plays a move not found in the book, or when
// the book has no entry for the current position. Prevents the engine from
// re-entering the book after diverging from theory.
extern bool out_of_book;

// ============================================================
// PROCEDURES & FUNCTIONS
// ============================================================

void init_book(const Position &start_pos);
Move probe_book(const Position &pos);