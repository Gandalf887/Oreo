#pragma once
#include "eval.h"
#include "movegen.h"
#include "makeunmake.h"
#include "position.h"
#include "legal.h"
#include "zobrist.h"
#include <chrono>
#include <atomic>

// ============================================================
// CONSTANTS
// ============================================================
// Hard ceiling on search depth - 64 plies is deep enough that it will
// never be reached in practice; it just existes to give fixed-size arrays
// a safe upper bound (e.g. killer_moves below).
const int MAX_DEPTH = 64;

// Written by the search at the end of each iteration so the GUI can
// display the depth reached without needing to query the search thread.
extern int g_last_depth;

// Infinity. Chosen largeenough to exceed any real evaluation but small 
//enough that adding a mate-distance bonus to it won't overflow a 32-bit int.
const int INF = 99999;

// ============================================================
// MOVE ORDERING TABLES
// ============================================================
// Stores up to two "killer" quiet moves per ply — moves that caused a
// beta cutoff elsewhere at the same depth. Trying these early in sibling
// nodes often produces another cutoff without needing a costly capture.
extern Move killer_moves[MAX_DEPTH][2];

// Indexed by [from][to]; incremented whenever a quiet move causes a beta
// cutoff during search. Moves with higher history scores are tried earlier,
// improving move ordering over the course of a search.
extern int history_table[64][64];

// ============================================================
// PONDERING / THREAD CONTROL
// ============================================================
// Set to true by the GUI thread to signal the search thread to stop
// immediately. Using an atomic ensures the flag is visible across threads
// without a data race.
extern std::atomic<bool> stop_search;

// The move the engine expects the opponent to play, predicted at the end
// of the previous search. The engine thinks about this move during the
// opponent's turn (pondering) to gain extra thinking time.
extern Move ponder_move;

// Best move found so far during a ponder search. Saved incrementally so
// that if stop_search is raised mid-search, the engine still has a valid
// move to play rather than returning an empty result.
extern Move ponder_best_so_far;

// ============================================================
// PROCEDURES & FUNCTIONS
// ============================================================

void clear_killers();

void clear_history();

int quiescence(Position &pos, int alpha, int beta, int depth);

int alphaBeta(Position &pos, int depth, int alpha, int beta, uint64_t *stack, int stack_size,
              bool null_allowed);

int calculate_time(int remaining_ms, int increment_ms = 0);

Move best_move(Position &pos, int max_depth, uint64_t *game_history, int game_history_count,
               int remaining_ms, int increment_ms = 0, bool skip_book = false);