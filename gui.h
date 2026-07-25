#pragma once

#include <string>
#include <vector>
using namespace std;

// Engine headers
#include "position.h"
#include "movegen.h"
#include "legal.h"
#include "search.h"
#include "display.h"
#include "zobrist.h"
#include "makeunmake.h"
#include "fen.h"
#include "transposition.h"
#include "book.h"

// Raylib
#include <raylib.h>

// Raylib defines WHITE and BLACK as Color macros that clash with the engine's
// Colour enum (which uses the same names for the two sides). Undefining them
// here lets the engine's own WHITE/BLACK enum values take precedence while
// still using Raylib's Color type for drawing calls.
#undef WHITE
#undef BLACK

// ============================================================
// GAME HISTORY
// ============================================================
// Stores the full sequence of moves and board positions played in the current
// game, along with a cursor (current) so the user can step backward/forward
// through history without losing the moves ahead. The positions array holds
// one extra entry (1025 vs 1024) because it also stores the initial position
// before any move is made, so positions[i] is the board state before moves[i].

struct GameHistory
{
    Move moves[1024];
    Position positions[1025];
    int count;   // total number of moves made so far
    int current; // index of the move the board is currently showing
};

// ============================================================
// ASSET MANAGEMENT
// ============================================================
// Loads all textures and fonts needed by the GUI (piece sprites, board image,
// etc.) into GPU memory. Must be called once after the Raylib window is opened
// and before any drawing takes place.
void load_assets();

// Releases all GPU memory held by textures and fonts loaded in load_assets().
// Must be called before closing the Raylib window to avoid resource leaks.
void unload_assets();

// ============================================================
// PROCEDURES & FUNCTIONS
// ============================================================

void draw_centered(const char *text, int x, int y, int size, Color col);

void draw_board(bool flipped);

void draw_last_move(Square from, Square to, bool flipped);

void draw_ponder_hint(Square sq, bool flipped);

void draw_check_highlight(const Position &pos, bool flipped);

void draw_checkmate_kings(const Position &pos, bool flipped);

void draw_highlights(const MoveList &list, Square selected, bool flipped);

void draw_pieces(const Position &pos, bool flipped);

void draw_clocks(double white_time, double black_time,
                 Colour side_to_move, Colour engine_side,
                 bool flipped, bool pondering_now);

void draw_info(int depth, double eval, const string &last_move,
               int history_current, int history_count, bool pondering);

void draw_selection_screen(int state);

void draw_game_over(const string &message, bool is_checkmate,
                    const Position &pos, bool flipped);

void run_gui(Position &pos);