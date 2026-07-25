// ============================================================
// THE FOLLOWING IS THE COMBINATION OF 28 FILES. TO GET THE FULLY SEPERATED ONES THAT WORK & 
// MY PROGRAM USES ACCESS THE GOOGLE DRIVE LINK IN MY H2 SUBMISSION. I WILL START BELOW WITH
// THE HEADER FILES. THIS WILL LIKELY NOT BE FUNCTIONAL AS THE INCLUDES AND SUCH WILL BE BROKEN BUT FOR THE
// REQUIREMENT OF SUBMISSION ALL THE CODE HAS BEEN CONCATENATED HERE AS REQUIRED.
// ============================================================

// ============================================================
// HEADER FILES:
// ============================================================

// ============================================================
// book.h:
// ============================================================

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

// ============================================================
// display.h:
// ============================================================

#pragma once
#include "position.h"
#include <string>

// ============================================================
// PROCEDURES & FUNCTIONS
// ============================================================

void print_board(const Position &pos);
std::string square_to_string(Square sq);
std::string piece_to_string(PieceType pt);

// ============================================================
// eval.h:
// ============================================================

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

// ============================================================
// fen.h:
// ============================================================

#pragma once
#include "position.h"
#include <string>

// ============================================================
// PROCEDURES & FUNCTIONS
// ============================================================

Position parse_fen(const std::string &fen);
std::string starting_fen();

// ============================================================
// gui.h:
// ============================================================

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

// ============================================================
// legal.h:
// ============================================================

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

// ============================================================
// makeunmake.h:
// ============================================================

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

// ============================================================
// move.h:
// ============================================================

#pragma once
#include <cstdint>
#include <string>
#include "types.h"

enum MoveFlag
{
    NORMAL,
    EN_PASSANT,
    CASTLING,
    PROMOTION,
};

struct Move
{
    Square from_square;

    Square to_square;

    PieceType piecetype;

    MoveFlag flag;
};

struct MoveList
{
    Move moves[256];
    int count;
};

inline void add_move(MoveList &list, Move move)
{
    list.moves[list.count] = move;
    list.count++;
}

struct UndoInfo
{
    Piece captured;
    // check piece captured

    Colour side_to_move;

    int castling_rights;

    Square en_passant_square;

    int halfmove_clock;

    int fullmove_clock;

    uint64_t hash;
};

// ============================================================
// movegen.h:
// ============================================================

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
    attacks |= (knight << 6) & ~0xC0C0C0C0C0C0C0C0ULL;
    attacks |= (knight >> 15) & ~0x0101010101010101ULL;
    attacks |= (knight >> 17) & ~0x8080808080808080ULL;
    attacks |= (knight >> 6) & ~0x0303030303030303ULL;
    attacks |= (knight >> 10) & ~0xC0C0C0C0C0C0C0C0ULL;
    return attacks;
}

// ============================================================
// position.h:
// ============================================================

#pragma once
#include <cstdint>
#include <string>
#include "types.h"

struct Position
{
    Bitboard bitboards[PIECE_COUNT];
    // assign one bitboard per piece start with pawns

    Colour side_to_move;

    int castling_rights;

    Square en_passant_square;

    int halfmove_clock;

    int fullmove_clock;

    uint64_t hash; // 1.13.0 hashing
};

inline Bitboard white_pieces(const Position &pos)
{
    return pos.bitboards[WP] | pos.bitboards[WN] | pos.bitboards[WB] | pos.bitboards[WR] | pos.bitboards[WQ] | pos.bitboards[WK];
}

inline Bitboard black_pieces(const Position &pos)
{
    return pos.bitboards[BP] | pos.bitboards[BN] | pos.bitboards[BB] | pos.bitboards[BR] | pos.bitboards[BQ] | pos.bitboards[BK];
}

inline Bitboard all_pieces(const Position &pos)
{
    return white_pieces(pos) | black_pieces(pos);
}

inline Bitboard empty_squares(const Position &pos)
{
    return ~all_pieces(pos);
}

inline Bitboard friendly_pieces(const Position &pos) // check friendly
{
    return pos.side_to_move == WHITE ? white_pieces(pos) : black_pieces(pos);
}

inline Bitboard enemy_pieces(const Position &pos) // check enemy
{
    return pos.side_to_move == WHITE ? black_pieces(pos) : white_pieces(pos);
}

inline Position empty_position()
{
    Position pos;
    for (int i = 0; i < PIECE_COUNT; i++)
        pos.bitboards[i] = 0ULL;
    pos.side_to_move = WHITE;
    pos.castling_rights = NO_CASTLING;
    pos.en_passant_square = NO_SQUARE;
    pos.halfmove_clock = 0;
    pos.fullmove_clock = 1;
    pos.hash = 0ULL; // 1.13.0
    return pos;
}

// ============================================================
// search.h:
// ============================================================
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
// enough that adding a mate-distance bonus to it won't overflow a 32-bit int.
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

// ============================================================
// transposition.h
// ============================================================

#pragma once
#include "types.h"
#include "move.h"

// the type of score stored in the table
enum TTFlag
{
    TT_EXACT, // exact score
    TT_ALPHA, // upper bound (score <= alpha)
    TT_BETA   // lower bound (score >= beta)
};

// one entry in the transposition table
struct TTEntry
{
    uint64_t hash; // zobrist hash of the posi
    int depth;     // depth this was searched to4
    int score;     // the score
    Move best;     // best move found
    TTFlag flag;   // type of score
    bool valid;    // is this entry occupied
};

// table size - it think 256MB should be enough can upgrade since 32gb ram
const int TT_SIZE = 1 << 22; // 4 millionn entries makjes 256MB

struct TranspositionTable
{
    TTEntry entries[TT_SIZE];

    void clear() // clear the table
    {
        for (int i = 0; i < TT_SIZE; i++)
        {
            entries[i].valid = false;
        }
    }

    void store(uint64_t hash, int depth, int score, Move best, TTFlag flag)
    {
        int index = hash % TT_SIZE;
        entries[index] = {hash,
                          depth,
                          score,
                          best,
                          flag,
                          true};
    }

    TTEntry *probe(uint64_t hash)
    {
        int index = hash % TT_SIZE;
        if (entries[index].valid && entries[index].hash == hash)
            return &entries[index];
        return nullptr;
    }
};

// gloabl transposition table
extern TranspositionTable tt;

// ============================================================
// types.h:
// ============================================================

#pragma once
#include <cstdint>
#include <string>

using Bitboard = uint64_t;

enum Square
{
    A1,
    B1,
    C1,
    D1,
    E1,
    F1,
    G1,
    H1,
    A2,
    B2,
    C2,
    D2,
    E2,
    F2,
    G2,
    H2,
    A3,
    B3,
    C3,
    D3,
    E3,
    F3,
    G3,
    H3,
    A4,
    B4,
    C4,
    D4,
    E4,
    F4,
    G4,
    H4,
    A5,
    B5,
    C5,
    D5,
    E5,
    F5,
    G5,
    H5,
    A6,
    B6,
    C6,
    D6,
    E6,
    F6,
    G6,
    H6,
    A7,
    B7,
    C7,
    D7,
    E7,
    F7,
    G7,
    H7,
    A8,
    B8,
    C8,
    D8,
    E8,
    F8,
    G8,
    H8,
    NO_SQUARE = -1
};

enum PieceType
{
    PAWN,
    KNIGHT,
    BISHOP,
    ROOK,
    QUEEN,
    KING,
    NO_PIECE_TYPE
};

enum Colour
{
    WHITE,
    BLACK,
    NO_COLOUR
};

enum Piece
{
    WP,
    WN,
    WB,
    WR,
    WQ,
    WK,
    BP,
    BN,
    BB,
    BR,
    BQ,
    BK,
    NO_PIECE,
    PIECE_COUNT = 12
};

enum CastlingRights
{
    NO_CASTLING = 0,
    WHITE_00 = 1,  // WHITE <KING>SIDE
    WHITE_000 = 2, // WHITE <QUEEN>SIDE
    BLACK_00 = 4,  // BLACK <KING>SIDE
    BLACK_000 = 8, // BLACK <QUEEN>SIDE
    ALL_CASTLING = 15
};

inline Bitboard square_bb(Square s)
{
    return 1ULL << s;
}

inline bool square_set(Bitboard b, Square s)
{
    return (b >> s) & 1ULL;
}

// ============================================================
// uci.h:
// ============================================================

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

// ============================================================
// zobrist.h:
// ============================================================

#pragma once
#include "position.h"
#include <cstdint>

// ============================================================
// PROCEDURES & FUNCTIONS
// ============================================================

void init_zobrist();
uint64_t hash_position(const Position &pos);

// 1.13.0
extern uint64_t piece_keys[PIECE_COUNT][64];
extern uint64_t side_key;
extern uint64_t castling_keys[16];
extern uint64_t en_passant_keys[8];

// ============================================================
// .CPP FILES. THIS IS THE ACTUAL CODE NOW.
// ============================================================

// ============================================================
// book.cpp
// ============================================================

#include "book.h"
#include "uci.h"
#include "movegen.h"
#include "legal.h"
#include "makeunmake.h"
#include <cstdlib>
#include <ctime>
#include <vector>
#include <unordered_map>
#include <string>
#include <algorithm>
#include <random>

// ============================================================
// PRIORITY TIERS
//   PRIORITY_FALLBACK  (0) -> flank/sideline responses, last resort
//   PRIORITY_STANDARD  (1) -> normal mainline responses as Black
//   PRIORITY_PREFERRED (2) -> engine's primary repertoire as White
//
// probe_book() always picks the highest available tier at a
// given position, then does weighted-random within that tier.
// This means:
//   - Black response lines never fire when engine is White
//   - Preferred White lines always beat standard ones
//   - Weights let you bias toward sharper or safer variations
// ============================================================

static const int PRIORITY_FALLBACK = 0;
static const int PRIORITY_STANDARD = 1;
static const int PRIORITY_PREFERRED = 2;

// ============================================================
// GLOBALS
// ============================================================

static std::mt19937 rng(std::random_device{}());
bool out_of_book = false;
std::unordered_map<uint64_t, std::vector<BookMove>> opening_book;

// ============================================================
// ADD LINE
// Walks the position move by move, recording hash -> BookMove
// at every step except the last (the last move IS the response,
// so it gets stored at the second-to-last position's hash).
//
// If a hash already has a BookMove with the same UCI string,
// we keep the highest priority / weight rather than duplicating.
// ============================================================

void add_line(Position pos,
              const std::vector<std::string> &moves,
              int priority,
              int weight)
{
    for (int i = 0; i < (int)moves.size() - 1; i++)
    {
        // Check if this hash already has this UCI move recorded
        bool already_recorded = false;
        auto it = opening_book.find(pos.hash);
        if (it != opening_book.end())
        {
            for (auto &bm : it->second)
            {
                if (bm.uci == moves[i])
                {
                    // Keep the highest priority/weight seen for this move
                    bm.priority = std::max(bm.priority, priority);
                    bm.weight = std::max(bm.weight, weight);
                    already_recorded = true;
                    break;
                }
            }
        }

        if (!already_recorded)
            opening_book[pos.hash].push_back({moves[i], weight, priority});

        // Advance the position by making the move
        MoveList list;
        generate_moves(pos, list);
        filter_legal_moves(pos, list);

        bool found = false;
        for (int j = 0; j < list.count; j++)
        {
            Move m = list.moves[j];
            std::string uci = sq_to_uci(m.from_square) +
                              sq_to_uci(m.to_square);
            if (uci == moves[i])
            {
                UndoInfo undo;
                make_move(pos, m, undo);
                found = true;
                break;
            }
        }
        if (!found)
            break; // bad move string — stop processing this line
    }
}

// ============================================================
// INIT BOOK
// Lines are grouped by priority tier. Within each tier, weight
// controls how often a move is chosen vs its siblings at the
// same position. Higher weight = chosen more often.
//
// PRIORITY_PREFERRED (2) — White's primary repertoire
// PRIORITY_STANDARD  (1) — Black responses to common White setups
// PRIORITY_FALLBACK  (0) — responses to rare/flank openings
// ============================================================

void init_book(const Position &start_pos)
{
    static std::mt19937 rng(std::random_device{}());

    // ============================================================
    // PRIORITY_PREFERRED — White's primary repertoire
    // These always fire when the engine is White because no Black
    // response line can reach priority 2.
    // ============================================================

    // ---------------------------------------------------------------
    // e4 — Ruy Lopez (main preferred system after 1.e4)
    // ---------------------------------------------------------------

    // Ruy Lopez — Closed (weight 15 — main workhorse)
    add_line(start_pos, {"e2e4", "e7e5", "g1f3", "b8c6", "f1b5", "a7a6", "b5a4", "g8f6", "e1g1", "f8e7", "f1e1", "b7b5", "a4b3", "d7d6", "c2c3", "e8g8"},
             PRIORITY_PREFERRED, 15);

    // Ruy Lopez — Closed, Chigorin (…Na5 …c5)
    add_line(start_pos, {"e2e4", "e7e5", "g1f3", "b8c6", "f1b5", "a7a6", "b5a4", "g8f6", "e1g1", "f8e7", "f1e1", "b7b5", "a4b3", "d7d6", "c2c3", "e8g8", "h2h3", "c6a5", "b3c2", "c7c5", "d2d4"},
             PRIORITY_PREFERRED, 15);

    // Ruy Lopez — Exchange (vs …a6, trade on c6)
    add_line(start_pos, {"e2e4", "e7e5", "g1f3", "b8c6", "f1b5", "a7a6", "b5c6", "d7c6", "e1g1", "f7f6", "d2d4", "e5d4", "f3d4", "c8d7", "b1c3"},
             PRIORITY_PREFERRED, 8);

    // Ruy Lopez — Berlin (solid, endgame-oriented)
    add_line(start_pos, {"e2e4", "e7e5", "g1f3", "b8c6", "f1b5", "g8f6", "e1g1", "f6e4", "d2d4", "e4d6", "b5c6", "d7c6", "d4e5", "d6f5", "d1d8", "e8d8", "b1c3"},
             PRIORITY_PREFERRED, 10);

    // Ruy Lopez — Archangel (…Bb4 aggressive)
    add_line(start_pos, {"e2e4", "e7e5", "g1f3", "b8c6", "f1b5", "a7a6", "b5a4", "g8f6", "e1g1", "f8b4", "f1e1", "b7b5", "a4b3", "e8g8", "c2c3", "d7d5"},
             PRIORITY_PREFERRED, 8);

    // ---------------------------------------------------------------
    // e4 — Italian (secondary e4 system)
    // ---------------------------------------------------------------

    // Italian — Giuoco Piano
    add_line(start_pos, {"e2e4", "e7e5", "g1f3", "b8c6", "f1c4", "f8c5", "c2c3", "g8f6", "d2d4", "e5d4", "c3d4", "c5b4", "c1d2", "b4d2", "b1d2", "d7d5"},
             PRIORITY_PREFERRED, 10);

    // Italian — Two Knights solid
    add_line(start_pos, {"e2e4", "e7e5", "g1f3", "b8c6", "f1c4", "g8f6", "d2d3", "f8c5", "c2c3", "d7d6", "e1g1", "e8g8", "b1d2"},
             PRIORITY_PREFERRED, 8);

    // ---------------------------------------------------------------
    // e4 — Scotch (tertiary e4 system for variety)
    // ---------------------------------------------------------------

    // Scotch — Classical (…Bc5)
    add_line(start_pos, {"e2e4", "e7e5", "g1f3", "b8c6", "d2d4", "e5d4", "f3d4", "f8c5", "c1e3", "d8f6", "c2c3", "g8e7", "f1c4", "e8g8", "e1g1"},
             PRIORITY_PREFERRED, 7);

    // Scotch — Mieses (…Nf6)
    add_line(start_pos, {"e2e4", "e7e5", "g1f3", "b8c6", "d2d4", "e5d4", "f3d4", "g8f6", "d4c6", "b7c6", "e4e5", "d8e7", "d1e2", "f6d5", "c2c4", "d5b6", "b1c3"},
             PRIORITY_PREFERRED, 7);

    // ---------------------------------------------------------------
    // e4 — Four Knights
    // ---------------------------------------------------------------

    add_line(start_pos, {"e2e4", "e7e5", "g1f3", "b8c6", "b1c3", "g8f6", "f1b5", "f8b4", "e1g1", "e8g8", "d2d3", "d7d6", "c1g5", "b4c3", "b2c3", "d8e7"},
             PRIORITY_PREFERRED, 8);

    // ---------------------------------------------------------------
    // d4 — London System (main d4 preferred system)
    // ---------------------------------------------------------------

    // London — vs …d5 …Nf6
    add_line(start_pos, {"d2d4", "d7d5", "g1f3", "g8f6", "c1f4", "e7e6", "e2e3", "f8d6", "f4d6", "d8d6", "f1d3", "e8g8", "e1g1", "b8d7", "b1d2", "b7b6", "c2c3"},
             PRIORITY_PREFERRED, 15);

    // London — vs …c5
    add_line(start_pos, {"d2d4", "d7d5", "g1f3", "g8f6", "c1f4", "c7c5", "e2e3", "b8c6", "c2c3", "d8b6", "d1b3", "b6b3", "a2b3", "c5d4", "c3d4", "e7e6", "b1c3"},
             PRIORITY_PREFERRED, 12);

    // London — vs KID setup (…g6)
    add_line(start_pos, {"d2d4", "g8f6", "g1f3", "g7g6", "c1f4", "f8g7", "e2e3", "d7d6", "h2h3", "e8g8", "f1e2", "b8d7", "e1g1", "c7c5", "c2c3"},
             PRIORITY_PREFERRED, 10);

    // ---------------------------------------------------------------
    // d4 — Queen's Gambit
    // ---------------------------------------------------------------

    // QGA — Classical
    add_line(start_pos, {"d2d4", "d7d5", "c2c4", "d5c4", "g1f3", "g8f6", "e2e3", "e7e6", "f1c4", "c7c5", "e1g1", "a7a6", "d1e2", "b7b5", "c4d3", "c5d4", "e3d4"},
             PRIORITY_PREFERRED, 12);

    // QGD — Orthodox
    add_line(start_pos, {"d2d4", "d7d5", "c2c4", "e7e6", "b1c3", "g8f6", "c1g5", "f8e7", "e2e3", "e8g8", "g1f3", "h7h6", "g5h4", "b7b6", "f1d3", "c8b7", "e1g1"},
             PRIORITY_PREFERRED, 12);

    // QGD — Exchange
    add_line(start_pos, {"d2d4", "d7d5", "c2c4", "e7e6", "b1c3", "g8f6", "c4d5", "e6d5", "c1g5", "f8e7", "e2e3", "c7c6", "g1f3", "b8d7", "f1d3", "f6h5", "g5e7", "d8e7", "e1g1"},
             PRIORITY_PREFERRED, 8);

    // Semi-Slav — Meran
    add_line(start_pos, {"d2d4", "d7d5", "c2c4", "e7e6", "b1c3", "g8f6", "g1f3", "c7c6", "e2e3", "b8d7", "f1d3", "d5c4", "d3c4", "b7b5", "c4d3", "c8b7", "e1g1", "b5b4"},
             PRIORITY_PREFERRED, 10);

    // ---------------------------------------------------------------
    // d4 — Catalan
    // ---------------------------------------------------------------

    // Catalan — Open (…dxc4)
    add_line(start_pos, {"d2d4", "d7d5", "c2c4", "e7e6", "g1f3", "g8f6", "g2g3", "d5c4", "f1g2", "f8e7", "e1g1", "e8g8", "d1c2", "a7a6", "a2a4", "b8d7"},
             PRIORITY_PREFERRED, 10);

    // Catalan — Closed
    add_line(start_pos, {"d2d4", "d7d5", "c2c4", "e7e6", "g1f3", "g8f6", "g2g3", "f8e7", "f1g2", "e8g8", "e1g1", "b8d7", "d1c2", "c7c6", "f1d1", "a7a6", "b1c3"},
             PRIORITY_PREFERRED, 10);

    // ---------------------------------------------------------------
    // d4 — Colle System
    // ---------------------------------------------------------------

    // Colle — Zukertort
    add_line(start_pos, {"d2d4", "d7d5", "g1f3", "g8f6", "e2e3", "e7e6", "f1d3", "f8d6", "e1g1", "e8g8", "b1d2", "b8d7", "f1e1", "c7c6", "e3e4"},
             PRIORITY_PREFERRED, 8);

    // Colle — vs …b6 fianchetto
    add_line(start_pos, {"d2d4", "g8f6", "g1f3", "e7e6", "e2e3", "b7b6", "f1d3", "c8b7", "e1g1", "f8e7", "b1d2", "d7d5", "c2c3", "e8g8", "d1e2"},
             PRIORITY_PREFERRED, 7);

    // ---------------------------------------------------------------
    // d4 — Slav Defence responses
    // ---------------------------------------------------------------

    // Slav — Main Line (…dxc4 …Bf5)
    add_line(start_pos, {"d2d4", "d7d5", "c2c4", "c7c6", "g1f3", "g8f6", "b1c3", "d5c4", "a2a4", "c8f5", "e2e3", "e7e6", "f1c4", "f8b4", "e1g1", "e8g8", "d1e2"},
             PRIORITY_PREFERRED, 10);

    // Slav — Chebanenko (…a6)
    add_line(start_pos, {"d2d4", "d7d5", "c2c4", "c7c6", "g1f3", "g8f6", "b1c3", "a7a6", "e2e3", "e7e6", "f1d3", "d5c4", "d3c4", "b7b5", "c4d3"},
             PRIORITY_PREFERRED, 8);

    // ---------------------------------------------------------------
    // c4 — English Opening
    // ---------------------------------------------------------------

    // English — Symmetrical (…c5)
    add_line(start_pos, {"c2c4", "c7c5", "b1c3", "b8c6", "g1f3", "g8f6", "g2g3", "g7g6", "f1g2", "f8g7", "e1g1", "e8g8", "d2d4", "c5d4", "f3d4"},
             PRIORITY_PREFERRED, 10);

    // English — reversed Sicilian (…e5)
    add_line(start_pos, {"c2c4", "e7e5", "b1c3", "g8f6", "g1f3", "b8c6", "g2g3", "f8b4", "f1g2", "e8g8", "e1g1", "e5e4", "f3g5", "b4c3", "d2c3", "h7h6"},
             PRIORITY_PREFERRED, 8);

    // English — vs Hedgehog (…Nf6 …e6 …b6)
    add_line(start_pos, {"c2c4", "g8f6", "b1c3", "e7e6", "g1f3", "b7b6", "g2g3", "c8b7", "f1g2", "f8e7", "e1g1", "e8g8", "d2d4", "d7d6", "b2b3"},
             PRIORITY_PREFERRED, 7);

    // ---------------------------------------------------------------
    // Nf3 — King's Indian Attack
    // ---------------------------------------------------------------

    add_line(start_pos, {"g1f3", "d7d5", "g2g3", "g8f6", "f1g2", "e7e6", "e1g1", "f8e7", "d2d3", "e8g8", "b1d2", "c7c5", "e2e4", "b8c6", "f1e1"},
             PRIORITY_PREFERRED, 8);

    // ============================================================
    // PRIORITY_STANDARD — Black responses to common White setups
    // These fire when the engine is Black. Priority 1 means they
    // never conflict with preferred White lines (priority 2).
    // ============================================================

    // ---------------------------------------------------------------
    // vs 1.e4 — Sicilian Defence
    // ---------------------------------------------------------------

    // Sicilian — Najdorf (main Black weapon vs e4)
    add_line(start_pos, {"e2e4", "c7c5", "g1f3", "d7d6", "d2d4", "c5d4", "f3d4", "g8f6", "b1c3", "a7a6", "f1e2", "e7e5", "d4b3", "f8e7", "e1g1", "e8g8", "c1e3"},
             PRIORITY_STANDARD, 15);

    // Sicilian — Najdorf, English Attack
    add_line(start_pos, {"e2e4", "c7c5", "g1f3", "d7d6", "d2d4", "c5d4", "f3d4", "g8f6", "b1c3", "a7a6", "c1e3", "c8g4", "f2f3", "g4h5", "d1d2", "e7e5", "d4b3", "f8e7", "e1g1"},
             PRIORITY_STANDARD, 12);

    // Sicilian — Dragon
    add_line(start_pos, {"e2e4", "c7c5", "g1f3", "d7d6", "d2d4", "c5d4", "f3d4", "g8f6", "b1c3", "g7g6", "c1e3", "f8g7", "f2f3", "e8g8", "d1d2", "b8c6", "e1c1"},
             PRIORITY_STANDARD, 10);

    // Sicilian — Scheveningen
    add_line(start_pos, {"e2e4", "c7c5", "g1f3", "d7d6", "d2d4", "c5d4", "f3d4", "g8f6", "b1c3", "e7e6", "f1e2", "f8e7", "e1g1", "e8g8", "f2f4", "b8c6", "c1e3"},
             PRIORITY_STANDARD, 10);

    // Sicilian — Classical (…Nc6)
    add_line(start_pos, {"e2e4", "c7c5", "g1f3", "b8c6", "d2d4", "c5d4", "f3d4", "g8f6", "b1c3", "d7d6", "f1e2", "e7e5", "d4b3", "f8e7", "e1g1", "e8g8"},
             PRIORITY_STANDARD, 10);

    // Sicilian — Kan / Taimanov
    add_line(start_pos, {"e2e4", "c7c5", "g1f3", "e7e6", "d2d4", "c5d4", "f3d4", "a7a6", "b1c3", "d8c7", "f1d3", "b8c6", "d4b3", "g8f6", "e1g1", "f8e7", "c1e3"},
             PRIORITY_STANDARD, 10);

    // ---------------------------------------------------------------
    // vs 1.e4 — French Defence
    // ---------------------------------------------------------------

    // French — Classical (…Nf6)
    add_line(start_pos, {"e2e4", "e7e6", "d2d4", "d7d5", "b1c3", "g8f6", "c1g5", "f8e7", "e4e5", "f6d7", "g5e7", "d8e7", "f2f4", "a7a5", "d1d2", "c7c5"},
             PRIORITY_STANDARD, 10);

    // French — Winawer (…Bb4)
    add_line(start_pos, {"e2e4", "e7e6", "d2d4", "d7d5", "b1c3", "f8b4", "e4e5", "c7c5", "a2a3", "b4c3", "b2c3", "g8e7", "d1g4", "e8g8", "g4d1", "b8c6", "g1f3"},
             PRIORITY_STANDARD, 10);

    // French — Tarrasch (vs 3.Nd2)
    add_line(start_pos, {"e2e4", "e7e6", "d2d4", "d7d5", "b1d2", "g8f6", "e4e5", "f6d7", "f1d3", "c7c5", "c2c3", "b8c6", "g1e2", "c5d4", "c3d4", "f7f6", "e5f6", "d7f6"},
             PRIORITY_STANDARD, 8);

    // French — Exchange (3.exd5)
    add_line(start_pos, {"e2e4", "e7e6", "d2d4", "d7d5", "e4d5", "e6d5", "g1f3", "g8f6", "f1d3", "f8d6", "e1g1", "e8g8", "h2h3", "b8c6", "c1f4", "d6f4", "d1f4"},
             PRIORITY_STANDARD, 6);

    // ---------------------------------------------------------------
    // vs 1.e4 — Caro-Kann Defence
    // ---------------------------------------------------------------

    // Caro-Kann — Classical (…Bf5)
    add_line(start_pos, {"e2e4", "c7c6", "d2d4", "d7d5", "b1c3", "d5e4", "c3e4", "c8f5", "e4g3", "f5g6", "h2h4", "h7h6", "g1f3", "g8f6", "h4h5", "g6h7", "f1d3", "h7d3", "d1d3"},
             PRIORITY_STANDARD, 10);

    // Caro-Kann — Advance (3.e5)
    add_line(start_pos, {"e2e4", "c7c6", "d2d4", "d7d5", "e4e5", "c8f5", "g1f3", "e7e6", "f1e2", "g8e7", "e1g1", "b8d7", "b1d2", "h7h6", "d1b3", "d8c7", "c1e3"},
             PRIORITY_STANDARD, 10);

    // Caro-Kann — Panov Attack (4.c4)
    add_line(start_pos, {"e2e4", "c7c6", "d2d4", "d7d5", "e4d5", "c6d5", "c2c4", "g8f6", "b1c3", "e7e6", "g1f3", "f8e7", "c4d5", "e6d5", "f1b5", "b8c6", "e1g1"},
             PRIORITY_STANDARD, 8);

    // ---------------------------------------------------------------
    // vs 1.e4 — Pirc / Modern
    // ---------------------------------------------------------------

    // Pirc — Classical System
    add_line(start_pos, {"e2e4", "d7d6", "d2d4", "g8f6", "b1c3", "g7g6", "g1f3", "f8g7", "f1e2", "e8g8", "e1g1", "c7c6", "a2a4", "b8d7", "h2h3"},
             PRIORITY_STANDARD, 8);

    // Pirc — Austrian Attack (f4)
    add_line(start_pos, {"e2e4", "d7d6", "d2d4", "g8f6", "b1c3", "g7g6", "f2f4", "f8g7", "g1f3", "c7c5", "d4d5", "e8g8", "f1e2", "e7e6", "e1g1", "e6d5", "e4d5"},
             PRIORITY_STANDARD, 6);

    // ---------------------------------------------------------------
    // vs 1.e4 e5 — King's Gambit Declined (solid response)
    // ---------------------------------------------------------------

    add_line(start_pos, {"e2e4", "e7e5", "f2f4", "d7d5", "e4d5", "e5e4", "d2d3", "g8f6", "d3e4", "f6e4", "g1f3", "f8c5", "d1e2", "d8e7", "c1e3"},
             PRIORITY_STANDARD, 8);

    // ---------------------------------------------------------------
    // vs 1.d4 — King's Indian Defence
    // ---------------------------------------------------------------

    // KID — Classical (…e5)
    add_line(start_pos, {"d2d4", "g8f6", "c2c4", "g7g6", "b1c3", "f8g7", "e2e4", "d7d6", "g1f3", "e8g8", "f1e2", "e7e5", "e1g1", "b8c6", "d4d5", "c6e7", "f3e1", "f6d7", "b2b4"},
             PRIORITY_STANDARD, 12);

    // KID — Samisch
    add_line(start_pos, {"d2d4", "g8f6", "c2c4", "g7g6", "b1c3", "f8g7", "e2e4", "d7d6", "f2f3", "e8g8", "c1e3", "e7e5", "d4d5", "b8d7", "d1d2", "f6h5", "e1c1", "f7f5"},
             PRIORITY_STANDARD, 8);

    // KID — Four Pawns Attack
    add_line(start_pos, {"d2d4", "g8f6", "c2c4", "g7g6", "b1c3", "f8g7", "e2e4", "d7d6", "f2f4", "e8g8", "g1f3", "c7c5", "d4d5", "e7e6", "f1e2", "e6d5", "c4d5"},
             PRIORITY_STANDARD, 6);

    // ---------------------------------------------------------------
    // vs 1.d4 — Nimzo-Indian Defence
    // ---------------------------------------------------------------

    // Nimzo — Rubinstein (4.e3)
    add_line(start_pos, {"d2d4", "g8f6", "c2c4", "e7e6", "b1c3", "f8b4", "e2e3", "e8g8", "f1d3", "d7d5", "g1f3", "c7c5", "e1g1", "d5c4", "d3c4", "b8d7", "d1d3"},
             PRIORITY_STANDARD, 12);

    // Nimzo — Classical (4.Qc2)
    add_line(start_pos, {"d2d4", "g8f6", "c2c4", "e7e6", "b1c3", "f8b4", "d1c2", "e8g8", "g1f3", "c7c5", "d4d5", "e6d5", "c4d5", "b8a6", "e2e4", "b4a5", "f1e2"},
             PRIORITY_STANDARD, 10);

    // Nimzo — Leningrad (4.Bg5)
    add_line(start_pos, {"d2d4", "g8f6", "c2c4", "e7e6", "b1c3", "f8b4", "c1g5", "h7h6", "g5h4", "c7c5", "d4d5", "d7d6", "e2e3", "b4c3", "b2c3", "e6d5", "c4d5"},
             PRIORITY_STANDARD, 8);

    // ---------------------------------------------------------------
    // vs 1.d4 — Queen's Indian Defence
    // ---------------------------------------------------------------

    // QID — Main Line (4.g3 …Ba6)
    add_line(start_pos, {"d2d4", "g8f6", "c2c4", "e7e6", "g1f3", "b7b6", "g2g3", "c8a6", "b2b3", "f8b4", "c1d2", "b4e7", "f1g2", "c7c6", "e1g1", "d7d5", "d1c2"},
             PRIORITY_STANDARD, 10);

    // QID — Petrosian (4.a3)
    add_line(start_pos, {"d2d4", "g8f6", "c2c4", "e7e6", "g1f3", "b7b6", "a2a3", "c8a6", "d1c2", "a6b7", "b1c3", "c7c5", "e2e4", "c5d4", "f3d4", "b8c6", "d4c6"},
             PRIORITY_STANDARD, 8);

    // ---------------------------------------------------------------
    // vs 1.d4 — Grunfeld Defence
    // ---------------------------------------------------------------

    // Grunfeld — Exchange (5.e4)
    add_line(start_pos, {"d2d4", "g8f6", "c2c4", "g7g6", "b1c3", "d7d5", "c4d5", "f6d5", "e2e4", "d5c3", "b2c3", "f8g7", "g1f3", "c7c5", "f1e2", "e8g8", "e1g1", "c5d4", "c3d4"},
             PRIORITY_STANDARD, 10);

    // Grunfeld — Russian System (7.Qb3)
    add_line(start_pos, {"d2d4", "g8f6", "c2c4", "g7g6", "b1c3", "d7d5", "g1f3", "f8g7", "d1b3", "d5c4", "b3c4", "e8g8", "e2e4", "c8g4", "c1e3", "b8d7", "f1e2"},
             PRIORITY_STANDARD, 8);

    // ---------------------------------------------------------------
    // vs 1.d4 — Dutch Defence
    // ---------------------------------------------------------------

    // Dutch — Leningrad (…f5 …g6)
    add_line(start_pos, {"d2d4", "f7f5", "g1f3", "g8f6", "g2g3", "g7g6", "f1g2", "f8g7", "e1g1", "e8g8", "c2c4", "d7d6", "b1c3", "d8e8", "d1b3", "c7c6", "c1f4"},
             PRIORITY_STANDARD, 8);

    // Dutch — Stonewall (…d5 …e6 …f5)
    add_line(start_pos, {"d2d4", "f7f5", "g1f3", "g8f6", "g2g3", "e7e6", "f1g2", "d7d5", "e1g1", "f8d6", "c2c4", "c7c6", "b2b3", "d8e7", "c1a3", "d6a3", "b1a3"},
             PRIORITY_STANDARD, 6);

    // ============================================================
    // PRIORITY_FALLBACK — responses to flank/rare openings
    // Only fires if no standard or preferred move exists at that hash.
    // ============================================================

    // vs English (1.c4) — …e5 reversed Sicilian
    add_line(start_pos, {"c2c4", "e7e5", "b1c3", "g8f6", "g1f3", "b8c6", "g2g3", "f8b4", "f1g2", "e8g8", "e1g1", "d7d6", "d2d3", "c8f5", "e2e4"},
             PRIORITY_FALLBACK, 10);

    // vs English (1.c4) — …c5 symmetrical
    add_line(start_pos, {"c2c4", "c7c5", "b1c3", "b8c6", "g2g3", "g7g6", "f1g2", "f8g7", "g1f3", "g8f6", "e1g1", "e8g8", "d2d4", "c5d4", "f3d4", "d7d6"},
             PRIORITY_FALLBACK, 10);

    // vs KIA (1.Nf3 2.g3) — French-like setup
    add_line(start_pos, {"g1f3", "d7d5", "g2g3", "c7c5", "f1g2", "b8c6", "e1g1", "e7e5", "d2d3", "g8f6", "b1d2", "f8e7", "e2e4", "d5d4", "a2a3"},
             PRIORITY_FALLBACK, 8);

    // vs Bird's (1.f4) — solid …d5 …Nf6
    add_line(start_pos, {"f2f4", "d7d5", "g1f3", "g8f6", "e2e3", "g7g6", "f1e2", "f8g7", "e1g1", "e8g8", "d2d3", "c7c5", "d1e1", "b8c6", "c2c3"},
             PRIORITY_FALLBACK, 8);
}

// ============================================================
// PROBE BOOK
// 1. Find all BookMoves at this position hash.
// 2. Select only those at the highest available priority tier.
// 3. Do a weighted-random draw within that tier.
//
// This guarantees:
//   - Preferred White lines always beat standard Black responses
//   - Fallback lines only fire when nothing better exists
//   - Variety within a tier prevents trivial preparation by opponents
// ============================================================

Move probe_book(const Position &pos)
{
    auto it = opening_book.find(pos.hash);
    if (it == opening_book.end())
        return {NO_SQUARE, NO_SQUARE, NO_PIECE_TYPE, NORMAL};

    const auto &entries = it->second;

    // Step 1 — find highest priority tier available
    int best_priority = -1;
    for (const auto &bm : entries)
        best_priority = std::max(best_priority, bm.priority);

    // Step 2 — collect candidates at that tier and sum weights
    int total_weight = 0;
    for (const auto &bm : entries)
        if (bm.priority == best_priority)
            total_weight += bm.weight;

    if (total_weight <= 0)
        return {NO_SQUARE, NO_SQUARE, NO_PIECE_TYPE, NORMAL};

    // Step 3 — weighted random draw
    int roll = std::uniform_int_distribution<int>(0, total_weight - 1)(rng);
    int cumulative = 0;
    std::string chosen_uci;

    for (const auto &bm : entries)
    {
        if (bm.priority != best_priority)
            continue;
        cumulative += bm.weight;
        if (roll < cumulative)
        {
            chosen_uci = bm.uci;
            break;
        }
    }

    // Step 4 — convert UCI string to a legal Move object
    MoveList list;
    generate_moves(const_cast<Position &>(pos), list);
    filter_legal_moves(const_cast<Position &>(pos), list);

    for (int i = 0; i < list.count; i++)
    {
        Move m = list.moves[i];
        std::string m_uci = sq_to_uci(m.from_square) +
                            sq_to_uci(m.to_square);
        if (m_uci == chosen_uci)
            return m;
    }

    // Move was in book but not legal — fail safe
    return {NO_SQUARE, NO_SQUARE, NO_PIECE_TYPE, NORMAL};
}

// ============================================================
// display.cpp
// ============================================================

#include "display.h"
#include <iostream>

// ============================================================
// PIECE IDENTIFICATION
// ============================================================
// Scans all 12 bitboards to find which piece (if any) occupies the given
// square, then returns its standard FEN letter — uppercase for White, lowercase
// for Black. Returns '.' for an empty square, matching the convention used
// when printing the board to the terminal.
char piece_on_square(const Position &pos, Square sq)
{
    for (int i = 0; i < PIECE_COUNT; i++)
    {
        if (square_set(pos.bitboards[i], sq))
        {
            switch (i)
            {
            // white pieces:
            case WP:
                return 'P';
            case WN:
                return 'N';
            case WB:
                return 'B';
            case WR:
                return 'R';
            case WQ:
                return 'Q';
            case WK:
                return 'K';
            // black pieces:
            case BP:
                return 'p';
            case BN:
                return 'n';
            case BB:
                return 'b';
            case BR:
                return 'r';
            case BQ:
                return 'q';
            case BK:
                return 'k';
            }
        }
    }
    return '.';
}

// ============================================================
// TERMINAL BOARD DISPLAY
// ============================================================
// Prints the board to stdout from White's perspective (rank 8 at the top),
// followed by file labels and a summary of side to move, castling rights,
// and en passant availability. Primarily used during development and
// debugging to inspect positions without the GUI.
void print_board(const Position &pos)
{
    std::cout << "\n";

    // Iterate ranks 8 down to 1 so the board reads top-to-bottom as a
    // human would expect, with rank 8 printed on the first line.
    for (int rank = 7; rank >= 0; rank--)
    {
        std::cout << (rank + 1) << " ";
        for (int file = 0; file < 8; file++)
        {
            Square sq = static_cast<Square>(rank * 8 + file);
            std::cout << piece_on_square(pos, sq) << "  ";
        }
        std::cout << "\n";
    }

    // File labels along the bottom, aligned with the three-character column width above.
    std::cout << "  a  b  c  d  e  f  g  h\n";

    // ── Position metadata ─────────────────────────────────────
    std::cout << "\nSide to move is: " << (pos.side_to_move == WHITE ? "White" : "Black") << "\n";

    // Castling rights are printed in standard FEN order (KQkq); "-" means
    // neither side has any castling rights remaining.
    std::cout << "Castling: ";
    if (pos.castling_rights == NO_CASTLING)
    {
        std::cout << "-";
    }
    else
    {
        if (pos.castling_rights & WHITE_00)
            std::cout << "K";
        if (pos.castling_rights & WHITE_000)
            std::cout << "Q";
        if (pos.castling_rights & BLACK_00)
            std::cout << "k";
        if (pos.castling_rights & BLACK_000)
            std::cout << "q";
    }
    std::cout << "\n";

    // En passant square is shown in algebraic notation (e.g. "e6"), or "-"
    // if no pawn just made a double push. The square index is converted back
    // to file/rank by taking modulo and integer division by 8 respectively.
    std::cout << "En passant: ";
    if (pos.en_passant_square == NO_SQUARE)
    {
        std::cout << "-";
    }
    else
    {
        int file = pos.en_passant_square % 8;
        int rank = pos.en_passant_square / 8;
        std::cout << (char)('a' + file) << (rank + 1);
    }
    std::cout << "\n";
}

// ============================================================
// NOTATION HELPERS
// ============================================================
// Converts a square index (0–63) to its two-character algebraic name
// (e.g. 0 → "a1", 63 → "h8"). Used when formatting moves and position
// info for display and UCI output.
std::string square_to_string(Square sq)
{
    std::string result = "";
    result += (char)('a' + sq % 8);
    result += (char)('1' + sq / 8);
    return result;
}

// Converts a PieceType enum value to its full English name for use in
// debug output and GUI labels where single-letter abbreviations would
// be ambiguous (e.g. distinguishing Bishop from Black in log messages).
std::string piece_to_string(PieceType pt)
{
    switch (pt)
    {
    case PAWN:
        return "Pawn";
    case KNIGHT:
        return "Knight";
    case BISHOP:
        return "Bishop";
    case ROOK:
        return "Rook";
    case QUEEN:
        return "Queen";
    case KING:
        return "King";
    default:
        return "Unknown";
    }
}

// ============================================================
// eval.cpp
// ============================================================
// 1.14.0 overhaul starts below
#include "eval.h"

inline int iabs(int x) { return x < 0 ? -x : x; }; // 1.14.0.1 technically faster then include <cstlib> so this will do

// ============================================================
// FILE MASKS — initialised once via init_eval()
// ============================================================
Bitboard FILE_MASKS[8];
Bitboard RANK_MASKS[8];
Bitboard ADJACENT_FILES[8];

// Pre-computes bitmasks for each file, rank, and the two files adjacent to each file.
// Called once at startup; these masks are used throughout pawn and king evaluation.
void init_eval()
{
    for (int f = 0; f < 8; f++)
    {
        FILE_MASKS[f] = 0ULL;
        for (int r = 0; r < 8; r++)
            FILE_MASKS[f] |= 1ULL << (r * 8 + f);

        ADJACENT_FILES[f] = 0ULL;
        if (f > 0)
            ADJACENT_FILES[f] |= FILE_MASKS[f - 1];
        if (f < 7)
            ADJACENT_FILES[f] |= FILE_MASKS[f + 1];
    }
    for (int r = 0; r < 8; r++)
    {
        RANK_MASKS[r] = 0ULL;
        for (int f = 0; f < 8; f++)
            RANK_MASKS[r] |= 1ULL << (r * 8 + f);
    }
}

// ============================================================
// MATERIAL VALUES
// ============================================================
const int PIECE_VALUES[PIECE_COUNT] = {
    100, 320, 330, 500, 900, 20000,      // white
    -100, -320, -330, -500, -900, -20000 // black
};

// ============================================================
// PIECE-SQUARE TABLES (from White's perspective, a1=0, h8=63)
// BUG FIX: Tables are indexed [sq] for White, [mirror(sq)] for Black.
// 1.13.x had this backwards - White used mirror(sq), Black used sq.
// ============================================================

const int MG_PAWN_TABLE[64] = {
    0, 0, 0, 0, 0, 0, 0, 0,
    98, 134, 61, 95, 68, 126, 34, -11,
    -6, 7, 26, 31, 65, 56, 25, -20,
    -14, 13, 6, 21, 23, 12, 17, -23,
    -27, -2, -5, 12, 17, 6, 10, -25,
    -26, -4, -4, -10, 3, 3, 33, -12,
    -35, -1, -20, -23, -15, 24, 38, -22,
    0, 0, 0, 0, 0, 0, 0, 0};

const int EG_PAWN_TABLE[64] = {
    0, 0, 0, 0, 0, 0, 0, 0,
    178, 173, 158, 134, 147, 132, 165, 187,
    94, 100, 85, 67, 56, 53, 82, 84,
    32, 24, 13, 5, -2, 4, 17, 17,
    13, 9, -3, -7, -7, -8, 3, -1,
    4, 7, -6, 1, 0, -5, -1, -8,
    13, 8, 8, 10, 13, 0, 2, -7,
    0, 0, 0, 0, 0, 0, 0, 0};

const int MG_KNIGHT_TABLE[64] = {
    -50, -40, -30, -30, -30, -30, -40, -50,
    -40, -20, 0, 0, 0, 0, -20, -40,
    -30, 0, 10, 15, 15, 10, 0, -30,
    -30, 5, 15, 20, 20, 15, 5, -30,
    -30, 0, 15, 20, 20, 15, 0, -30,
    -30, 5, 10, 15, 15, 10, 5, -30,
    -40, -20, 0, 5, 5, 0, -20, -40,
    -50, -40, -30, -30, -30, -30, -40, -50};

const int EG_KNIGHT_TABLE[64] = {
    -58, -38, -13, -28, -31, -27, -63, -99,
    -25, -8, -25, -2, -9, -25, -24, -52,
    -24, -20, 10, 9, -1, -9, -19, -41,
    -17, 3, 22, 22, 22, 11, 8, -18,
    -18, -6, 16, 25, 16, 17, 4, -18,
    -23, -3, -1, 15, 10, -3, -20, -22,
    -42, -20, -10, -5, -2, -20, -23, -44,
    -29, -51, -23, -15, -22, -18, -50, -64};

const int MG_BISHOP_TABLE[64] = {
    -29, 4, -82, -37, -25, -42, 7, -8,
    -26, 16, -18, -13, 30, 59, 18, -47,
    -16, 37, 43, 40, 35, 50, 37, -2,
    -4, 5, 19, 50, 37, 37, 7, -2,
    -6, 13, 13, 26, 34, 12, 10, 4,
    0, 15, 15, 15, 14, 27, 18, 10,
    4, 15, 16, 0, 7, 21, 33, 1,
    -33, -3, -14, -21, -13, -12, -39, -21};

const int EG_BISHOP_TABLE[64] = {
    -14, -21, -11, -8, -7, -9, -17, -24,
    -8, -4, 7, -12, -3, -13, -4, -14,
    2, -8, 0, -1, -2, 6, 0, 4,
    -3, 9, 12, 9, 14, 10, 3, 2,
    -6, 3, 13, 19, 7, 10, -3, -9,
    -12, -3, 8, 10, 13, 3, -7, -15,
    -14, -18, -7, -1, 4, -9, -15, -27,
    -23, -9, -23, -5, -9, -16, -5, -17};

const int MG_ROOK_TABLE[64] = {
    32, 42, 32, 51, 63, 9, 31, 43,
    27, 32, 58, 62, 80, 67, 26, 44,
    -5, 19, 26, 36, 17, 45, 61, 16,
    -24, -11, 7, 26, 24, 35, -8, -20,
    -36, -26, -12, -1, 9, -7, 6, -23,
    -45, -25, -16, -17, 3, 0, -5, -33,
    -44, -16, -20, -9, -1, 11, -6, -71,
    -19, -13, 1, 17, 16, 7, -37, -26};

const int EG_ROOK_TABLE[64] = {
    13, 10, 18, 15, 12, 12, 8, 5,
    11, 13, 13, 11, -3, 3, 8, 3,
    7, 7, 7, 5, 4, -3, -5, -3,
    4, 3, 13, 1, 2, 1, -1, 2,
    3, 5, 8, 4, -5, -6, -8, -11,
    -4, 0, -5, -1, -7, -12, -8, -16,
    -6, -6, 0, 2, -9, -9, -11, -3,
    -9, 2, 3, -1, -5, -13, 4, -20};

const int MG_QUEEN_TABLE[64] = {
    -28, 0, 29, 12, 59, 44, 43, 45,
    -24, -39, -5, 1, -16, 57, 28, 54,
    -13, -17, 7, 8, 29, 56, 47, 57,
    -27, -27, -16, -16, -1, 17, -2, 1,
    -9, -26, -9, -10, -2, -4, 3, -3,
    -14, 2, -11, -2, -5, 2, 14, 5,
    -35, -8, 11, 2, 8, 15, -3, 1,
    -1, -18, -9, 10, -15, -25, -31, -50};

const int EG_QUEEN_TABLE[64] = {
    -9, 22, 22, 27, 27, 19, 10, 20,
    -17, 20, 32, 41, 58, 25, 30, 0,
    -20, 6, 9, 49, 47, 35, 19, 9,
    3, 22, 24, 45, 57, 40, 57, 36,
    -18, 28, 19, 47, 31, 34, 39, 23,
    -16, -27, 15, 6, 9, 17, 10, 5,
    -22, -23, -30, -16, -16, -23, -36, -32,
    -33, -28, -22, -43, -5, -32, -20, -41};

const int MG_KING_TABLE[64] = {
    -65, 23, 16, -15, -56, -34, 2, 13,
    29, -1, -20, -7, -8, -4, -38, -29,
    -9, 24, 2, -16, -20, 6, 22, -22,
    -17, -20, -12, -27, -30, -25, -14, -36,
    -49, -1, -27, -39, -46, -44, -33, -51,
    -14, -14, -22, -46, -44, -30, -15, -27,
    1, 7, -8, -64, -43, -16, 9, 8,
    -15, 36, 12, -54, 8, -28, 24, 14};

const int EG_KING_TABLE[64] = {
    -74, -35, -18, -18, -11, 15, 4, -17,
    -12, 17, 14, 17, 17, 38, 23, 11,
    10, 17, 23, 15, 20, 45, 44, 13,
    -8, 22, 24, 27, 26, 33, 26, 3,
    -18, -4, 21, 24, 27, 23, 9, -11,
    -19, -3, 11, 21, 23, 16, 7, -9,
    -27, -11, 4, 13, 14, 4, -5, -17,
    -53, -34, -21, -11, -28, -14, -24, -43};

// ============================================================
// HELPERS
// ============================================================

// BUG FIX: mirror flips rank so Black's tables read correctly.
// sq 0 (a1, White's back rank) maps to 56 (a8, Black's back rank).
inline int mirror(int sq) { return sq ^ 56; }

inline int popcount(Bitboard b) { return __builtin_popcountll(b); }

// Game phase: 24 = full middlegame, 0 = pure endgame.
// Returns a phase value from 0 (pure endgame) to 24 (full middlegame) based on
// remaining piece material. Knights/bishops = 1, rooks = 2, queens = 4.
// Used to blend middlegame and endgame scores via tapered_bonus().
int game_phase(const Position &pos)
{
    int phase = 0;
    phase += popcount(pos.bitboards[WN] | pos.bitboards[BN]) * 1;
    phase += popcount(pos.bitboards[WB] | pos.bitboards[BB]) * 1;
    phase += popcount(pos.bitboards[WR] | pos.bitboards[BR]) * 2;
    phase += popcount(pos.bitboards[WQ] | pos.bitboards[BQ]) * 4;
    return phase;
}

// Linearly interpolates between a middlegame (mg) and endgame (eg) score using
// the current game phase. At phase=24 returns mg; at phase=0 returns eg.
inline int tapered_bonus(int mg, int eg, int phase)
{
    return (mg * phase + eg * (24 - phase)) / 24;
}

// ============================================================
// PIECE-SQUARE TABLE LOOKUP
// BUG FIX: White uses sq directly; Black mirrors.
// 1.13.x had White mirroring and Black using sq — completely backwards.
// ============================================================

// Returns the middlegame piece-square bonus for a given piece on a given square.
// White pieces index the table directly; Black pieces use mirror(sq) and negate,
// so both sides are evaluated from White's perspective.
int mg_table(Piece piece, int sq)
{
    switch (piece)
    {
    case WP:
        return MG_PAWN_TABLE[sq];
    case WN:
        return MG_KNIGHT_TABLE[sq];
    case WB:
        return MG_BISHOP_TABLE[sq];
    case WR:
        return MG_ROOK_TABLE[sq];
    case WQ:
        return MG_QUEEN_TABLE[sq];
    case WK:
        return MG_KING_TABLE[sq];
    case BP:
        return -MG_PAWN_TABLE[mirror(sq)];
    case BN:
        return -MG_KNIGHT_TABLE[mirror(sq)];
    case BB:
        return -MG_BISHOP_TABLE[mirror(sq)];
    case BR:
        return -MG_ROOK_TABLE[mirror(sq)];
    case BQ:
        return -MG_QUEEN_TABLE[mirror(sq)];
    case BK:
        return -MG_KING_TABLE[mirror(sq)];
    default:
        return 0;
    }
}

// Same as mg_table but uses the endgame piece-square tables.
int eg_table(Piece piece, int sq)
{
    switch (piece)
    {
    case WP:
        return EG_PAWN_TABLE[sq];
    case WN:
        return EG_KNIGHT_TABLE[sq];
    case WB:
        return EG_BISHOP_TABLE[sq];
    case WR:
        return EG_ROOK_TABLE[sq];
    case WQ:
        return EG_QUEEN_TABLE[sq];
    case WK:
        return EG_KING_TABLE[sq];
    case BP:
        return -EG_PAWN_TABLE[mirror(sq)];
    case BN:
        return -EG_KNIGHT_TABLE[mirror(sq)];
    case BB:
        return -EG_BISHOP_TABLE[mirror(sq)];
    case BR:
        return -EG_ROOK_TABLE[mirror(sq)];
    case BQ:
        return -EG_QUEEN_TABLE[mirror(sq)];
    case BK:
        return -EG_KING_TABLE[mirror(sq)];
    default:
        return 0;
    }
}

// ============================================================
// PAWN STRUCTURE EVALUATION
// BUG FIX: Passed pawn bonus was being added TWICE — once as a
// flat bonus and again as an endgame extra. This made a rank-6
// passed pawn worth ~293cp, warping the entire eval. Now we use
// tapered_bonus() to blend a single MG and EG value.
// ============================================================

// Scores pawn structure: doubled pawn penalties, isolated pawn penalties, and
// passed pawn bonuses (tapered MG/EG). Passed pawn bonuses include king proximity
// terms — in the endgame, the defending king's distance from the passer and the
// attacking king's closeness to it both matter significantly.
int evaluate_pawns(const Position &pos, int phase)
{
    int score = 0;
    Bitboard wp = pos.bitboards[WP];
    Bitboard bp = pos.bitboards[BP];

    // King squares for proximity bonus
    int wk_sq = __builtin_ctzll(pos.bitboards[WK]);
    int wk_file = wk_sq % 8;
    int wk_rank = wk_sq / 8;
    int bk_sq = __builtin_ctzll(pos.bitboards[BK]);
    int bk_file = bk_sq % 8;
    int bk_rank = bk_sq / 8;

    // Passed pawn bonus tables (mg and eg separately)
    // Indexed by rank 0-7. rank 0 and 7 should never fire.
    static const int MG_PASSED[8] = {0, 10, 20, 35, 55, 80, 110, 0};
    static const int EG_PASSED[8] = {0, 20, 40, 65, 100, 145, 200, 0};

    for (int f = 0; f < 8; f++)
    {
        Bitboard wp_file = wp & FILE_MASKS[f];
        Bitboard bp_file = bp & FILE_MASKS[f];

        // Doubled pawn penalty
        if (popcount(wp_file) > 1)
            score -= 20;
        if (popcount(bp_file) > 1)
            score += 20;

        // Isolated pawn penalty
        if (wp_file && !(wp & ADJACENT_FILES[f]))
            score -= 15;
        if (bp_file && !(bp & ADJACENT_FILES[f]))
            score += 15;

        // ---- White passed pawns ----
        Bitboard wp_iter = wp_file;
        while (wp_iter)
        {
            int sq = __builtin_ctzll(wp_iter);
            int rank = sq / 8; // 0 = rank 1 (White's starting side)

            // Build a mask of all squares ahead of this pawn on this
            // and adjacent files (i.e. the pawn's "cone of concern").
            Bitboard ahead = 0ULL;
            for (int r = rank + 1; r < 8; r++)
            {
                ahead |= 1ULL << (r * 8 + f);
                if (f > 0)
                    ahead |= 1ULL << (r * 8 + f - 1);
                if (f < 7)
                    ahead |= 1ULL << (r * 8 + f + 1);
            }

            if (!(bp & ahead))
            {
                score += tapered_bonus(MG_PASSED[rank], EG_PASSED[rank], phase);
                // King proximity — only meaningful in endgame
                // Enemy king far from passer = more dangerous
                int bk_dist = iabs(bk_file - f) + iabs(bk_rank - rank);
                score += tapered_bonus(0, bk_dist * 5, phase);

                // Our king close to passer = better support
                int wk_dist = iabs(wk_file - f) + iabs(wk_rank - rank);
                score -= tapered_bonus(0, wk_dist * 3, phase);
            }
            wp_iter &= wp_iter - 1;
        }

        // ---- Black passed pawns ----
        Bitboard bp_iter = bp_file;
        while (bp_iter)
        {
            int sq = __builtin_ctzll(bp_iter);
            int rank = sq / 8; // 7 = rank 8 (Black's starting side)

            Bitboard ahead = 0ULL;
            for (int r = rank - 1; r >= 0; r--)
            {
                ahead |= 1ULL << (r * 8 + f);
                if (f > 0)
                    ahead |= 1ULL << (r * 8 + f - 1);
                if (f < 7)
                    ahead |= 1ULL << (r * 8 + f + 1);
            }

            // BUG FIX: Black's passed pawn rank bonus should reference
            // the *mirrored* rank (distance from promotion), not the raw rank.
            // rank 7 = Black's back rank (0 from promo), rank 0 = almost promoting.
            int mirrored_rank = 7 - rank;
            if (!(wp & ahead))
            {
                score -= tapered_bonus(MG_PASSED[mirrored_rank], EG_PASSED[mirrored_rank], phase);

                // Enemy king far from passer = more dangerous for White
                int wk_dist = iabs(wk_file - f) + iabs(wk_rank - rank);
                score -= tapered_bonus(0, wk_dist * 5, phase);

                // Black king close to passer = better support
                int bk_dist = iabs(bk_file - f) + iabs(bk_rank - rank);
                score += tapered_bonus(0, bk_dist * 3, phase);
            }

            bp_iter &= bp_iter - 1;
        }
    }

    return score;
}

// ============================================================
// KING SAFETY
// BUG FIX 1: King safety now only fires after castling (king on
// a/b/c or f/g/h file). Applying it from move 1 penalises
// normal development since e1/e8 have no pawn shield.
// BUG FIX 2: The open-file penalty for Black's king was checking
// White pawn absence but labelling it as a penalty on Black — the
// logic was the same sign for both sides, causing no differential.
// ============================================================

// Scores king shelter (pawn shield on the three files in front of the king) and
// open/semi-open file penalties near the king. Only applied after castling
// (king on a/b/c or f/g/h file) since the starting e-file has no natural shield.
// The total is scaled by game phase — king safety is far less important in the endgame.
int evaluate_king_safety(const Position &pos, int phase)
{
    Bitboard white_pawns = pos.bitboards[WP];
    Bitboard black_pawns = pos.bitboards[BP];

    int w_score = 0;
    int b_score = 0;

    // ---- White king ----
    int wk_sq = __builtin_ctzll(pos.bitboards[WK]);
    int wk_file = wk_sq % 8;
    int wk_rank = wk_sq / 8;

    // Only penalise/reward shield when king has actually castled.
    bool w_castled = (wk_file < 3 || wk_file > 4);
    if (w_castled && wk_rank < 6)
    {
        for (int df = -1; df <= 1; df++)
        {
            int f = wk_file + df;
            if (f < 0 || f > 7)
                continue;
            if (white_pawns & (1ULL << ((wk_rank + 1) * 8 + f)))
                w_score += 10;
            else if (wk_rank + 2 < 8 && (white_pawns & (1ULL << ((wk_rank + 2) * 8 + f))))
                w_score += 5;
            else
                w_score -= 15;
        }
    }

    // Open-file penalty near king (semi/fully open = attackers can rush in)
    for (int df = -1; df <= 1; df++)
    {
        int f = wk_file + df;
        if (f < 0 || f > 7)
            continue;
        bool no_wp = !(white_pawns & FILE_MASKS[f]);
        bool no_bp = !(black_pawns & FILE_MASKS[f]);
        if (no_wp && no_bp)
            w_score -= 20; // fully open
        else if (no_wp)
            w_score -= 10; // semi-open (White pawn gone)
    }

    // ---- Black king ----
    int bk_sq = __builtin_ctzll(pos.bitboards[BK]);
    int bk_file = bk_sq % 8;
    int bk_rank = bk_sq / 8;

    bool b_castled = (bk_file < 3 || bk_file > 4);
    if (b_castled && bk_rank > 1)
    {
        for (int df = -1; df <= 1; df++)
        {
            int f = bk_file + df;
            if (f < 0 || f > 7)
                continue;
            if (black_pawns & (1ULL << ((bk_rank - 1) * 8 + f)))
                b_score += 10;
            else if (bk_rank - 2 >= 0 && (black_pawns & (1ULL << ((bk_rank - 2) * 8 + f))))
                b_score += 5;
            else
                b_score -= 15;
        }
    }

    // BUG FIX: was checking White pawn absence for Black's open file penalty.
    for (int df = -1; df <= 1; df++)
    {
        int f = bk_file + df;
        if (f < 0 || f > 7)
            continue;
        bool no_wp = !(white_pawns & FILE_MASKS[f]);
        bool no_bp = !(black_pawns & FILE_MASKS[f]);
        if (no_wp && no_bp)
            b_score -= 20;
        else if (no_bp)
            b_score -= 10; // BUG FIX: was (no_wp) before
    }

    // Scale by phase — king safety matters far more in the middlegame.
    return ((w_score - b_score) * phase) / 24;
}

// ============================================================
// MOBILITY
// ============================================================

// Counts the number of legal destination squares for each piece type, weighted by
// piece: knights x3, bishops x2, rooks x2, queens x1. A higher weight on knights
// reflects that a short-range piece with few squares available is significantly
// worse than a long-range piece in the same situation.
int evaluate_mobility(const Position &pos)
{
    int score = 0;
    Bitboard occupied = all_pieces(pos);
    Bitboard white = white_pieces(pos);
    Bitboard black = black_pieces(pos);

    // Knights (weight 3 — lots of squares matters a lot for a short-range piece)
    Bitboard wn = pos.bitboards[WN];
    while (wn)
    {
        int sq = __builtin_ctzll(wn);
        score += popcount(knight_attacks_bb(sq) & ~white) * 3;
        wn &= wn - 1;
    }
    Bitboard bn = pos.bitboards[BN];
    while (bn)
    {
        int sq = __builtin_ctzll(bn);
        score -= popcount(knight_attacks_bb(sq) & ~black) * 3;
        bn &= bn - 1;
    }

    // Bishops (weight 2)
    Bitboard wb = pos.bitboards[WB];
    while (wb)
    {
        int sq = __builtin_ctzll(wb);
        score += popcount(bishop_attacks(static_cast<Square>(sq), occupied) & ~white) * 2;
        wb &= wb - 1;
    }
    Bitboard bb_pieces = pos.bitboards[BB];
    while (bb_pieces)
    {
        int sq = __builtin_ctzll(bb_pieces);
        score -= popcount(bishop_attacks(static_cast<Square>(sq), occupied) & ~black) * 2;
        bb_pieces &= bb_pieces - 1;
    }

    // Rooks (weight 2 — a trapped rook is a serious problem)
    Bitboard wr = pos.bitboards[WR];
    while (wr)
    {
        int sq = __builtin_ctzll(wr);
        score += popcount(rook_attacks(static_cast<Square>(sq), occupied) & ~white) * 2;
        wr &= wr - 1;
    }
    Bitboard br = pos.bitboards[BR];
    while (br)
    {
        int sq = __builtin_ctzll(br);
        score -= popcount(rook_attacks(static_cast<Square>(sq), occupied) & ~black) * 2;
        br &= br - 1;
    }

    // Queens (weight 1 — queens have very high raw mobility, so a small
    // weight still produces a meaningful bonus without dominating everything)
    Bitboard wq = pos.bitboards[WQ];
    while (wq)
    {
        int sq = __builtin_ctzll(wq);
        score += popcount((bishop_attacks(static_cast<Square>(sq), occupied) |
                           rook_attacks(static_cast<Square>(sq), occupied)) &
                          ~white) *
                 1;
        wq &= wq - 1;
    }
    Bitboard bq = pos.bitboards[BQ];
    while (bq)
    {
        int sq = __builtin_ctzll(bq);
        score -= popcount((bishop_attacks(static_cast<Square>(sq), occupied) |
                           rook_attacks(static_cast<Square>(sq), occupied)) &
                          ~black) *
                 1;
        bq &= bq - 1;
    }

    return score;
}

// ============================================================
// ROOK EVALUATION
// ============================================================

// Scores rook placement: +20 for an open file (no pawns of either colour),
// +10/−10 for a semi-open file (own pawns gone), and a bonus for occupying
// the 7th rank (rank 6 for White, rank 1 for Black) which traps the enemy king
// and attacks unadvanced pawns.
int evaluate_rooks(const Position &pos)
{
    int score = 0;
    Bitboard wp = pos.bitboards[WP];
    Bitboard bp = pos.bitboards[BP];

    Bitboard wr = pos.bitboards[WR];
    while (wr)
    {
        int sq = __builtin_ctzll(wr);
        int f = sq % 8;
        int r = sq / 8;
        bool no_wp = !(wp & FILE_MASKS[f]);
        bool no_bp = !(bp & FILE_MASKS[f]);

        if (no_wp && no_bp)
            score += 20; // open file
        else if (no_wp)
            score += 10; // semi-open (White's pawn gone)

        if (r == 6)
            score += 25; // 7th rank (0-indexed rank 6)

        wr &= wr - 1;
    }

    Bitboard br = pos.bitboards[BR];
    while (br)
    {
        int sq = __builtin_ctzll(br);
        int f = sq % 8;
        int r = sq / 8;
        bool no_wp = !(wp & FILE_MASKS[f]);
        bool no_bp = !(bp & FILE_MASKS[f]);

        if (no_wp && no_bp)
            score -= 20;
        else if (no_bp)
            score -= 10; // BUG FIX: was (no_wp) — wrong side

        if (r == 1)
            score -= 25; // 7th rank from Black's side (0-indexed rank 1)

        br &= br - 1;
    }

    return score;
}

//============================================================
// EVALUATE ENGDGAME MOPUP
//============================================================
// Add inside evaluate(), only when one side has no pawns and minimal material
// i.e. pure K+R or K+Q vs K endings 1.14.0.1
// In near-pure endgames (phase <= 4) with a large material imbalance, rewards
// pushing the losing king to a corner and closing the distance between kings.
// This guides the engine to convert won K+R/K+Q vs K positions that pure
// material/PST evaluation alone cannot reliably finish.
int evaluate_mopup(const Position &pos, int phase)
{
    // Only kick in when very close to pure endgame
    if (phase > 4)
        return 0;

    int score = 0;

    // If White has a large material advantage, push Black king to corner
    int material_diff = 0;
    for (int i = 0; i < PIECE_COUNT; i++)
        material_diff += PIECE_VALUES[i] * popcount(pos.bitboards[i]);

    if (material_diff > 400) // White is up a rook or more
    {
        int bk_sq = __builtin_ctzll(pos.bitboards[BK]);
        int bk_file = bk_sq % 8;
        int bk_rank = bk_sq / 8;

        // Manhattan distance from centre — corner = 6, centre = 0
        int centre_dist = (iabs(bk_file - 3) + iabs(bk_rank - 3));
        score += centre_dist * 10;

        // Bonus for White king being close to Black king (to assist mate)
        int wk_sq = __builtin_ctzll(pos.bitboards[WK]);
        int wk_file = wk_sq % 8;
        int wk_rank = wk_sq / 8;
        int king_dist = iabs(wk_file - bk_file) + iabs(wk_rank - bk_rank);
        score += (14 - king_dist) * 5;
    }

    if (material_diff < -400) // Black is up
    {
        int wk_sq = __builtin_ctzll(pos.bitboards[WK]);
        int wk_file = wk_sq % 8;
        int wk_rank = wk_sq / 8;
        int centre_dist = (iabs(wk_file - 3) + iabs(wk_rank - 3));
        score -= centre_dist * 10;

        int bk_sq = __builtin_ctzll(pos.bitboards[BK]);
        int bk_file = bk_sq % 8;
        int bk_rank = bk_sq / 8;
        int king_dist = iabs(wk_file - bk_file) + iabs(wk_rank - bk_rank);
        score -= (14 - king_dist) * 5;
    }

    return score;
}

//============================================================
// EVALUATE DEVELOPMENT
//============================================================
// Only applies in the opening (high phase = lots of pieces = early game) 1.14.0.2
// Applies opening-specific bonuses and penalties: rewards central pawn presence,
// penalises unmoved minor pieces and central pawns, penalises early queen
// development, and rewards castling. Fades out entirely below phase 10 so it
// has no effect once the opening is over.
int evaluate_development(const Position &pos, int phase)
{
    // Fade out completely by move ~10 (phase drops as pieces trade)
    if (phase < 10)
        return 0;

    int score = 0;

    // ---- Pawn centre control ----
    // Bonus for pawns on e4/d4 (White) and e5/d5 (Black)
    if (pos.bitboards[WP] & (1ULL << 27))
        score += 30; // d4
    if (pos.bitboards[WP] & (1ULL << 28))
        score += 30; // e4
    if (pos.bitboards[BP] & (1ULL << 35))
        score -= 30; // d5
    if (pos.bitboards[BP] & (1ULL << 36))
        score -= 30; // e5

    // ---- Penalty for unmoved central pawns ----
    // d2/e2 pawns still on starting square = not developing
    if (pos.bitboards[WP] & (1ULL << 11))
        score -= 20; // d2
    if (pos.bitboards[WP] & (1ULL << 12))
        score -= 20; // e2
    if (pos.bitboards[BP] & (1ULL << 51))
        score += 20; // d7
    if (pos.bitboards[BP] & (1ULL << 52))
        score += 20; // e7

    // ---- Penalty for unmoved minor pieces ----
    // Knights and bishops still on back rank
    if (pos.bitboards[WN] & (1ULL << 1))
        score -= 80; // b1
    if (pos.bitboards[WN] & (1ULL << 6))
        score -= 80; // g1
    if (pos.bitboards[WB] & (1ULL << 2))
        score -= 60; // c1
    if (pos.bitboards[WB] & (1ULL << 5))
        score -= 60; // f1
    if (pos.bitboards[BN] & (1ULL << 57))
        score += 80; // b8
    if (pos.bitboards[BN] & (1ULL << 62))
        score += 80; // g8
    if (pos.bitboards[BB] & (1ULL << 58))
        score += 60; // c8
    if (pos.bitboards[BB] & (1ULL << 61))
        score += 60; // f8

    // ---- Queen early development penalty ----
    // Queen moved before move ~6 = bad
    if (!(pos.bitboards[WQ] & (1ULL << 3)))
        score -= 20; // not on d1
    if (!(pos.bitboards[BQ] & (1ULL << 59)))
        score += 20; // not on d8

    // ---- Castling reward ----
    // Approximated: king off e1/e8 and not in centre = probably castled
    int wk_sq = __builtin_ctzll(pos.bitboards[WK]);
    int bk_sq = __builtin_ctzll(pos.bitboards[BK]);
    if (wk_sq == 6 || wk_sq == 2)
        score += 40; // g1 or c1
    if (bk_sq == 62 || bk_sq == 58)
        score -= 40; // g8 or c8

    // Scale by phase — only meaningful in the opening
    return score;
}

// ============================================================
// MAIN EVALUATE FUNCTION
// ============================================================

// Top-level evaluation function. Sums material, tapered piece-square tables,
// bishop pair bonus, and all structural sub-evaluations. Returns the score from
// the perspective of the side to move (positive = good for the mover), which is
// the convention required by the negamax search framework.
int evaluate(const Position &pos)
{
    int score = 0;
    int phase = game_phase(pos);

    // Material + tapered piece-square tables
    for (int i = 0; i < PIECE_COUNT; i++)
    {
        Bitboard bb = pos.bitboards[i];
        while (bb)
        {
            int sq = __builtin_ctzll(bb);
            score += PIECE_VALUES[i];
            score += tapered_bonus(
                mg_table(static_cast<Piece>(i), sq),
                eg_table(static_cast<Piece>(i), sq),
                phase);
            bb &= bb - 1;
        }
    }

    // Bishop pair bonus
    if (popcount(pos.bitboards[WB]) >= 2)
        score += 30;
    if (popcount(pos.bitboards[BB]) >= 2)
        score -= 30;

    // Structural terms
    score += evaluate_pawns(pos, phase);
    score += evaluate_king_safety(pos, phase);
    score += evaluate_mobility(pos);
    score += evaluate_rooks(pos);
    score += evaluate_mopup(pos, phase);
    score += evaluate_development(pos, phase);

    // Return from the side-to-move's perspective (negamax convention)
    return pos.side_to_move == WHITE ? score : -score;
}

// ============================================================
// fen.cpp
// ============================================================

#include "fen.h"
#include <sstream>
#include <stdexcept>
#include "zobrist.h"

// ============================================================
// STARTING POSITION
// ============================================================
// Returns the standard chess starting position as a FEN string.
// Kept as a function rather than a constant so callers always get
// a fresh copy and can't accidentally mutate a shared string.
std::string starting_fen()
{
    return "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1";
}

// ============================================================
// FEN PARSER
// ============================================================
// Converts a FEN string into a fully populated Position struct.
// FEN encodes six space-separated fields: piece placement, side to move,
// castling rights, en passant square, halfmove clock, and fullmove counter.
// The Zobrist hash is computed from scratch at the end so the position is
// immediately ready for use in the transposition table without a separate
// init call.
Position parse_fen(const std::string &fen)
{
    Position pos = empty_position();
    std::istringstream ss(fen);
    std::string piece_placement, side, castling, en_passant;
    int halfmove, fullmove;

    // Split all six FEN fields in one pass using istringstream's whitespace tokenisation.
    ss >> piece_placement >> side >> castling >> en_passant >> halfmove >> fullmove;

    // ── Field 1: piece placement ──────────────────────────────
    // FEN describes the board from rank 8 down to rank 1, left to right.
    // We start at rank index 7 (rank 8) and decrement on each '/' separator.
    int rank = 7;
    int file = 0;

    for (char c : piece_placement)
    {
        if (c == '/')
        {
            // Slash signals end of a rank — move down one rank and reset to the A file.
            rank--;
            file = 0;
        }
        else if (c >= '1' && c <= '8')
        {
            // A digit means that many consecutive empty squares — advance the file cursor.
            file += (c - '0');
        }
        else
        {
            // Any other character is a piece letter. Convert rank+file to a square index
            // (0 = a1, 63 = h8) and set the corresponding bit in the right bitboard.
            Square sq = static_cast<Square>(rank * 8 + file);
            Piece piece;
            switch (c)
            {
            case 'P':
                piece = WP;
                break;
            case 'N':
                piece = WN;
                break;
            case 'B':
                piece = WB;
                break;
            case 'R':
                piece = WR;
                break;
            case 'Q':
                piece = WQ;
                break;
            case 'K':
                piece = WK;
                break;
            case 'p':
                piece = BP;
                break;
            case 'n':
                piece = BN;
                break;
            case 'b':
                piece = BB;
                break;
            case 'r':
                piece = BR;
                break;
            case 'q':
                piece = BQ;
                break;
            case 'k':
                piece = BK;
                break;
            default:
                throw std::runtime_error("Invalid piece character in FEN.");
            }
            pos.bitboards[piece] |= square_bb(sq);
            file++;
        }
    }

    // ── Field 2: side to move ─────────────────────────────────
    // "w" means White moves next, anything else is treated as Black.
    pos.side_to_move = (side == "w") ? WHITE : BLACK;

    // ── Field 3: castling rights ──────────────────────────────
    // Each letter grants one castling right; "-" means none are available.
    // Rights are stored as a bitmask so make_move can update them cheaply
    // with a single AND operation rather than four separate flag checks.
    pos.castling_rights = NO_CASTLING;
    if (castling != "-")
    {
        for (char c : castling)
        {
            switch (c)
            {
            case 'K':
                pos.castling_rights |= WHITE_00;
                break;
            case 'Q':
                pos.castling_rights |= WHITE_000;
                break;
            case 'k':
                pos.castling_rights |= BLACK_00;
                break;
            case 'q':
                pos.castling_rights |= BLACK_000;
                break;
            }
        }
    }

    // ── Field 4: en passant square ────────────────────────────
    // "-" means no en passant is possible this move. Otherwise the square
    // is given in algebraic notation (e.g. "e6") and converted to an index
    // by treating the file letter as an offset from 'a' and the rank digit
    // as an offset from '1', matching the engine's 0-63 square numbering.
    if (en_passant == "-")
    {
        pos.en_passant_square = NO_SQUARE;
    }
    else
    {
        int ep_file = en_passant[0] - 'a';
        int ep_rank = en_passant[1] - '1';
        pos.en_passant_square = static_cast<Square>(ep_rank * 8 + ep_file);
    }

    // ── Fields 5 & 6: move clocks ─────────────────────────────
    // halfmove_clock counts plies since the last capture or pawn move and
    // is used to enforce the 50-move draw rule. fullmove_clock counts
    // complete moves (incremented after Black's reply) and is used for
    // display and PGN export only — the search doesn't depend on it.
    pos.halfmove_clock = halfmove;
    pos.fullmove_clock = fullmove;

    // Compute the Zobrist hash from scratch so this position can be looked
    // up in the transposition table immediately. During normal play the hash
    // is maintained incrementally by make_move/unmake_move, but when loading
    // from FEN there is no prior hash to update from.
    pos.hash = hash_position(pos);

    return pos;
}

// ============================================================
// gui.cpp
// ============================================================

#include "gui.h"
#include "search.h"
#include "legal.h"
#include "transposition.h"
#include "zobrist.h"
#include "book.h"
#include <sstream>
#include <iomanip>
#include <string>
#include <cmath>
#include <thread>
#include <atomic>
#include <mutex>
#include <iostream>
#include <math.h>

using namespace std;

// ============================================================
// LAYOUT
// ============================================================
const int WINDOW_WIDTH = 1200;
const int WINDOW_HEIGHT = 820;
const int BOARD_OFFSET_X = 280;
const int BOARD_OFFSET_Y = 60;
const int SQUARE_SIZE = 86;
const int BOARD_SIZE = SQUARE_SIZE * 8;

// ============================================================
// COLOURS
// ============================================================
const Color BG_COLOR = {28, 27, 23, 255};
const Color LIGHT_SQ = {235, 209, 166, 255};
const Color DARK_SQ = {165, 117, 81, 255};
const Color HIGHLIGHT_SEL = {186, 202, 43, 180};
const Color HIGHLIGHT_MOV = {247, 247, 105, 140};
const Color HIGHLIGHT_LAST_FROM = {205, 210, 60, 160};
const Color HIGHLIGHT_LAST_TO = {205, 210, 60, 200};
const Color HIGHLIGHT_PONDER = {100, 180, 255, 100};
const Color HIGHLIGHT_CHECK = {220, 50, 50, 190};
const Color HIGHLIGHT_MATE_WIN = {60, 200, 80, 210};
const Color HIGHLIGHT_MATE_LOSE = {200, 40, 40, 210};
const Color TEXT_GRAY = {150, 150, 150, 255};
const Color TEXT_WHITE = {220, 220, 220, 255};
const Color TEXT_YELLOW = {200, 160, 80, 255};
const Color TEXT_GREEN = {100, 200, 100, 255};
const Color TEXT_PURPLE = {160, 100, 200, 255};
const Color TEXT_RED = {180, 80, 80, 255};
const Color TEXT_BLUE = {100, 160, 220, 255};
const Color RAY_WHITE = {255, 255, 255, 255};
const Color RAY_BLACK = {0, 0, 0, 255};

// ============================================================
// PIECE PNG'S & FONTS
// ============================================================
Texture2D piece_textures[12];
const char *PIECE_FILES[12] = {
    "resources/images/wP.png", "resources/images/wN.png",
    "resources/images/wB.png", "resources/images/wR.png",
    "resources/images/wQ.png", "resources/images/wK.png",
    "resources/images/bP.png", "resources/images/bN.png",
    "resources/images/bB.png", "resources/images/bR.png",
    "resources/images/bQ.png", "resources/images/bK.png"};

enum GameState
{
    SELECTING_SIDE,
    SELECTING_TIME,
    PLAYING
};

Font main_font;

static GameHistory gh;
static uint64_t game_history[1024];

// ── Externals from search.cpp / uci.cpp ───────────────────────────────────
extern int g_last_depth;
extern Move ponder_best_so_far;
extern bool out_of_book;
// stop_search : std::atomic<bool> declared in search.h

// ── GUI pondering state ───────────────────────────────────────────────────
static std::thread gui_ponder_thread;
static std::atomic<bool> gui_pondering{false};
static Move gui_ponder_move;    // move we are pondering over
static Position gui_ponder_pos; // board after engine move + ponder move
static uint64_t gui_ponder_history[1024];
static int gui_ponder_history_count = 0;

// Signals the background ponder thread to stop via stop_search, then joins it
// on a detached thread to avoid blocking the GUI. Resets stop_search to false
// once the ponder thread has exited so future searches are not immediately halted.
static void stop_ponder()
{
    if (gui_ponder_thread.joinable())
    {
        stop_search = true;
        std::thread([t = std::move(gui_ponder_thread)]() mutable
                    {
            t.join();
            stop_search = false; })
            .detach();
    }
    else
    {
        stop_search = false;
    }
    gui_pondering = false;
}

// Starts a background search on the position after the engine's move plus the
// expected reply (ponder move). Searches up to depth 24 with 30 seconds so it
// runs indefinitely until stop_ponder() is called. If the human plays the
// predicted move (a "ponder hit"), the search result can be used immediately.
static void start_ponder(Position cap_pos, uint64_t *cap_hist, int cap_count)
{
    stop_ponder();
    uint64_t hist_copy[1024];
    for (int i = 0; i < cap_count; i++)
        hist_copy[i] = cap_hist[i];
    stop_search = false;
    gui_pondering = true;
    gui_ponder_thread = std::thread([cap_pos, hist_copy, cap_count]() mutable
                                    { best_move(cap_pos, 24, hist_copy, cap_count, 30000, 0, true); });
}

// ── Small helpers ─────────────────────────────────────────────────────────
// Converts a board square index to pixel coordinates of the top-left corner of
// that square, accounting for board orientation (flipped = Black's perspective).
static void sq_to_screen(Square sq, bool flipped, int &sx, int &sy)
{
    int file = sq % 8, rank = sq / 8;
    sx = BOARD_OFFSET_X + (flipped ? 7 - file : file) * SQUARE_SIZE;
    sy = BOARD_OFFSET_Y + (flipped ? rank : 7 - rank) * SQUARE_SIZE;
}

// Converts a mouse pixel position to a board square index, returning NO_SQUARE
// if the click is outside the board area.
static Square screen_to_square(int mx, int my, bool flipped)
{
    int file = (mx - BOARD_OFFSET_X) / SQUARE_SIZE;
    int rank = (my - BOARD_OFFSET_Y) / SQUARE_SIZE;
    if (file < 0 || file >= 8 || rank < 0 || rank >= 8)
        return NO_SQUARE;
    return static_cast<Square>((flipped ? rank : 7 - rank) * 8 + (flipped ? 7 - file : file));
}

// Formats a time in seconds as "MM:SS", clamped to zero for negative values.
static string format_time(int s)
{
    if (s < 0)
        s = 0;
    char buf[16];
    snprintf(buf, sizeof(buf), "%02d:%02d", s / 60, s % 60);
    return buf;
}

// Draws text horizontally centred on x at the given y position using the global font.
void draw_centered(const char *text, int x, int y, int size, Color col)
{
    DrawTextEx(main_font, text,
               {(float)(x - MeasureText(text, size) / 2), (float)y},
               size, 1, col);
}

// Loads the font and all 12 piece textures (6 white, 6 black) from disk into GPU memory.
void load_assets()
{
    main_font = LoadFontEx("resources/fonts/centurygothic.ttf", 128, nullptr, 0);
    for (int i = 0; i < 12; i++)
        piece_textures[i] = LoadTexture(PIECE_FILES[i]);
}

// Releases all GPU texture and font resources. Called on shutdown.
void unload_assets()
{
    for (int i = 0; i < 12; i++)
        UnloadTexture(piece_textures[i]);
    UnloadFont(main_font);
}

// ── Draw helpers ──────────────────────────────────────────────────────────
// Draws the 64 board squares in alternating light/dark colours and renders rank
// numbers and file letters along the edges, respecting the flipped orientation.
void draw_board(bool flipped)
{
    for (int r = 0; r < 8; r++)
        for (int f = 0; f < 8; f++)
        {
            DrawRectangle(BOARD_OFFSET_X + (flipped ? 7 - f : f) * SQUARE_SIZE,
                          BOARD_OFFSET_Y + (flipped ? r : 7 - r) * SQUARE_SIZE,
                          SQUARE_SIZE, SQUARE_SIZE,
                          (r + f) % 2 != 0 ? LIGHT_SQ : DARK_SQ);
        }
    for (int i = 0; i < 8; i++)
    {
        char buf[4];
        snprintf(buf, sizeof(buf), "%d", flipped ? i + 1 : 8 - i);
        DrawTextEx(main_font, buf,
                   {(float)(BOARD_OFFSET_X - 20),
                    (float)(BOARD_OFFSET_Y + i * SQUARE_SIZE + SQUARE_SIZE / 2 - 10)},
                   18, 1, TEXT_GRAY);
        snprintf(buf, sizeof(buf), "%c", 'a' + (flipped ? 7 - i : i));
        DrawTextEx(main_font, buf,
                   {(float)(BOARD_OFFSET_X + i * SQUARE_SIZE + SQUARE_SIZE / 2 - 6),
                    (float)(BOARD_OFFSET_Y + BOARD_SIZE + 8)},
                   18, 1, TEXT_GRAY);
    }
}

// Highlights the from- and to-squares of the most recently played move in yellow.
void draw_last_move(Square from, Square to, bool flipped)
{
    if (from == NO_SQUARE || to == NO_SQUARE)
        return;
    int sx, sy;
    sq_to_screen(from, flipped, sx, sy);
    DrawRectangle(sx, sy, SQUARE_SIZE, SQUARE_SIZE, HIGHLIGHT_LAST_FROM);
    sq_to_screen(to, flipped, sx, sy);
    DrawRectangle(sx, sy, SQUARE_SIZE, SQUARE_SIZE, HIGHLIGHT_LAST_TO);
}

// Draws a translucent blue overlay on the square the engine expects the human to
// play to (the ponder move destination), giving a visual hint during pondering.
void draw_ponder_hint(Square sq, bool flipped)
{
    if (sq == NO_SQUARE)
        return;
    int sx, sy;
    sq_to_screen(sq, flipped, sx, sy);
    DrawRectangle(sx, sy, SQUARE_SIZE, SQUARE_SIZE, HIGHLIGHT_PONDER);
}

// Draws a red overlay on the king's square when the side to move is in check.
void draw_check_highlight(const Position &pos, bool flipped)
{
    if (!is_in_check(pos, pos.side_to_move))
        return;
    Bitboard kb = pos.side_to_move == WHITE ? pos.bitboards[WK] : pos.bitboards[BK];
    if (!kb)
        return;
    int sx, sy;
    sq_to_screen(static_cast<Square>(__builtin_ctzll(kb)), flipped, sx, sy);
    DrawRectangle(sx, sy, SQUARE_SIZE, SQUARE_SIZE, HIGHLIGHT_CHECK);
}

// At the end of the game, colours the losing king red and the winning king green
// to make the result visually clear on the board.
void draw_checkmate_kings(const Position &pos, bool flipped)
{
    Bitboard lb = pos.side_to_move == WHITE ? pos.bitboards[WK] : pos.bitboards[BK];
    Bitboard wb = pos.side_to_move == WHITE ? pos.bitboards[BK] : pos.bitboards[WK];
    int sx, sy;
    if (lb)
    {
        sq_to_screen(static_cast<Square>(__builtin_ctzll(lb)), flipped, sx, sy);
        DrawRectangle(sx, sy, SQUARE_SIZE, SQUARE_SIZE, HIGHLIGHT_MATE_LOSE);
    }
    if (wb)
    {
        sq_to_screen(static_cast<Square>(__builtin_ctzll(wb)), flipped, sx, sy);
        DrawRectangle(sx, sy, SQUARE_SIZE, SQUARE_SIZE, HIGHLIGHT_MATE_WIN);
    }
}

// Highlights the selected square in green-yellow and all legal destination squares
// for that piece with a semi-transparent yellow overlay and a small dot indicator.
void draw_highlights(const MoveList &list, Square selected, bool flipped)
{
    if (selected != NO_SQUARE)
    {
        int sx, sy;
        sq_to_screen(selected, flipped, sx, sy);
        DrawRectangle(sx, sy, SQUARE_SIZE, SQUARE_SIZE, HIGHLIGHT_SEL);
    }
    for (int i = 0; i < list.count; i++)
        if (list.moves[i].from_square == selected)
        {
            int sx, sy;
            sq_to_screen(list.moves[i].to_square, flipped, sx, sy);
            DrawRectangle(sx, sy, SQUARE_SIZE, SQUARE_SIZE, HIGHLIGHT_MOV);
            DrawCircle(sx + SQUARE_SIZE / 2, sy + SQUARE_SIZE / 2, 12, {0, 0, 0, 80});
        }
}

// Iterates all 64 squares, looks up which piece occupies each via the bitboards,
// and draws the corresponding texture scaled to SQUARE_SIZE.
void draw_pieces(const Position &pos, bool flipped)
{
    for (int r = 0; r < 8; r++)
        for (int f = 0; f < 8; f++)
        {
            Square sq = static_cast<Square>(r * 8 + f);
            int sx = BOARD_OFFSET_X + (flipped ? 7 - f : f) * SQUARE_SIZE;
            int sy = BOARD_OFFSET_Y + (flipped ? r : 7 - r) * SQUARE_SIZE;
            for (int i = 0; i < 12; i++)
                if (square_set(pos.bitboards[i], sq))
                {
                    float scale = (float)SQUARE_SIZE / piece_textures[i].width;
                    DrawTextureEx(piece_textures[i], {(float)sx, (float)sy}, 0.0f, scale, RAY_WHITE);
                    break;
                }
        }
}

// Draws the two player clock panels to the right of the board. The active clock's
// time pulses when below 30s and turns red below 10s. A thin progress bar shows
// the fraction of 30s remaining. Labels change to "(pondering)" while the engine
// thinks on the opponent's time.
void draw_clocks(double white_time, double black_time,
                 Colour side_to_move, Colour engine_side,
                 bool flipped, bool pondering_now)
{
    const int cx = BOARD_OFFSET_X + BOARD_SIZE + 30;
    const int cw = 240;
    const int mid_y = WINDOW_HEIGHT / 2;

    bool top_white = flipped;
    double top_time = top_white ? white_time : black_time;
    double bot_time = top_white ? black_time : white_time;
    Colour top_col = top_white ? WHITE : BLACK;
    Colour bot_col = top_white ? BLACK : WHITE;
    bool top_active = (side_to_move == top_col);

    auto label_for = [&](Colour c) -> string
    {
        if (c == engine_side && pondering_now)
            return string(c == WHITE ? "White" : "Black") + " (pondering)";
        return string(c == WHITE ? "White" : "Black") + (c == engine_side ? " (Engine)" : " (You)");
    };
    auto pulse = [](double t, bool active) -> float
    {
        if (!active || t >= 10.0)
            return 1.0f;
        return 1.0f + 0.04f * (1.0f - (float)(t / 10.0)) * sinf((float)(GetTime() * 6.0));
    };
    auto tcol = [](double t, bool active) -> Color
    {
        if (!active)
            return TEXT_GRAY;
        if (t < 10.0)
            return TEXT_RED;
        if (t < 30.0)
            return TEXT_YELLOW;
        return TEXT_WHITE;
    };
    auto draw_panel = [&](int py, double t, bool active, Colour c)
    {
        Color lc = (c == engine_side && pondering_now) ? TEXT_BLUE : TEXT_GRAY;
        DrawTextEx(main_font, label_for(c).c_str(), {(float)(cx + 8), (float)py}, 20, 1, lc);
        string ts = format_time((int)t);
        int fsz = (int)(68 * pulse(t, active));
        Vector2 ts_size = MeasureTextEx(main_font, ts.c_str(), fsz, 1);
        DrawTextEx(main_font, ts.c_str(),
                   {cx + 8 + (cw - 16 - ts_size.x) / 2.0f, (float)(py + 28)},
                   fsz, 1, tcol(t, active));
        if (active && t < 30.0)
        {
            Color bar = t < 10.0 ? TEXT_RED : TEXT_YELLOW;
            bar.a = 160;
            DrawRectangle(cx, py + 108, (int)(cw * (t / 30.0)), 4, bar);
            DrawRectangle(cx, py + 108, cw, 4, {80, 80, 80, 80});
        }
    };

    draw_panel(mid_y - 190, top_time, top_active, top_col);
    DrawLine(cx + 10, mid_y - 10, cx + cw - 10, mid_y - 10, {60, 60, 60, 200});
    draw_panel(mid_y + 20, bot_time, !top_active, bot_col);
}

// Renders the sidebar info panel: search depth, centipawn evaluation, last move
// played, pondering indicator, move history position, and keyboard shortcut hints.
void draw_info(int depth, double eval, const string &last_move,
               int history_current, int history_count, bool pondering)
{
    int px = 20, py = 40;
    DrawTextEx(main_font, ("Depth: " + to_string(depth)).c_str(), {(float)px, (float)py}, 26, 1, TEXT_RED);
    string es = (eval >= 0 ? "+" : "") + to_string(eval).substr(0, to_string(eval).find('.') + 3);
    DrawTextEx(main_font, ("Eval:  " + es).c_str(), {(float)px, (float)(py + 45)}, 26, 1, TEXT_YELLOW);
    DrawTextEx(main_font, ("Move:  " + last_move).c_str(), {(float)px, (float)(py + 90)}, 26, 1, TEXT_PURPLE);
    if (pondering)
        DrawTextEx(main_font, "Pondering...", {(float)px, (float)(py + 125)}, 20, 1, TEXT_BLUE);
    DrawTextEx(main_font, (to_string(history_current) + " / " + to_string(history_count)).c_str(),
               {(float)px, (float)(py + 155)}, 22, 1, TEXT_GRAY);
    DrawTextEx(main_font, "< > review moves", {(float)px, (float)(WINDOW_HEIGHT - 100)}, 18, 1, TEXT_GRAY);
    DrawTextEx(main_font, "F   flip board", {(float)px, (float)(WINDOW_HEIGHT - 74)}, 18, 1, TEXT_GRAY);
    DrawTextEx(main_font, "R   play again", {(float)px, (float)(WINDOW_HEIGHT - 48)}, 18, 1, TEXT_GRAY);
}

// Draws either the side-selection screen (White/Black buttons) or the time-control
// selection screen (1/3/5/10/15 minute buttons), depending on the current GameState.
void draw_selection_screen(GameState state)
{
    int cx = WINDOW_WIDTH / 2;
    if (state == SELECTING_SIDE)
    {
        draw_centered("Oreo 1.16.1.9", cx, 120, 52, TEXT_WHITE);
        draw_centered("Choose your side", cx, 240, 28, TEXT_GRAY);
        DrawRectangle(cx - 180, 310, 150, 65, LIGHT_SQ);
        draw_centered("Play White", cx - 105, 330, 24, RAY_BLACK);
        DrawRectangle(cx + 30, 310, 150, 65, DARK_SQ);
        draw_centered("Play Black", cx + 105, 330, 24, TEXT_WHITE);
    }
    else
    {
        draw_centered("Select Time Control", cx, 220, 32, TEXT_WHITE);
        vector<int> times = {1, 3, 5, 10, 15};
        for (int i = 0; i < (int)times.size(); i++)
        {
            int bx = cx - 290 + i * 130;
            DrawRectangle(bx, 310, 110, 65, {70, 70, 70, 255});
            DrawRectangleLines(bx, 310, 110, 65, TEXT_GRAY);
            draw_centered((to_string(times[i]) + " min").c_str(), bx + 55, 330, 22, TEXT_WHITE);
        }
    }
}

// Draws a semi-transparent overlay on the board with the game result message.
void draw_game_over(const string &message, bool is_checkmate,
                    const Position &pos, bool flipped)
{
    DrawRectangle(BOARD_OFFSET_X, BOARD_OFFSET_Y, BOARD_SIZE, BOARD_SIZE, {0, 0, 0, 120});
    int bx = BOARD_OFFSET_X + BOARD_SIZE / 2 - 180;
    int by = BOARD_OFFSET_Y + BOARD_SIZE / 2 - 60;
    DrawRectangle(bx, by, 360, 120, {40, 40, 40, 240});
    DrawRectangleLines(bx, by, 360, 120, TEXT_GRAY);
    draw_centered(message.c_str(), BOARD_OFFSET_X + BOARD_SIZE / 2, by + 20, 26, TEXT_WHITE);
    draw_centered("Press R to play again", BOARD_OFFSET_X + BOARD_SIZE / 2, by + 70, 20, TEXT_GRAY);
}

// Main GUI entry point. Opens the Raylib window and runs the game loop at 60 FPS.
// Manages all game state: menu flow, clock ticking, human input, engine thread
// dispatch, pondering, move history browsing (arrow keys), board flip (F), and
// game reset (R). The engine runs on a detached thread and signals completion via
// the engine_move_ready atomic flag so the GUI thread is never blocked.
void run_gui(Position &pos)
{
    InitWindow(WINDOW_WIDTH, WINDOW_HEIGHT, "Oreo 1.16.1.9");
    SetTargetFPS(60);
    load_assets();

    GameState state = SELECTING_SIDE;
    Colour engine_side = WHITE;
    bool flipped = false;

    Square selected = NO_SQUARE;
    Square last_from = NO_SQUARE;
    Square last_to = NO_SQUARE;
    Square ponder_hint_sq = NO_SQUARE;

    string last_move_str = "-";
    double last_eval = 0.0;
    int last_depth = 0;

    bool game_over = false;
    bool game_over_is_checkmate = false;
    string game_over_msg = "";
    bool human_turn = false;

    std::atomic<bool> engine_thinking{false};
    std::atomic<bool> engine_move_ready{false};
    Move engine_best_move;
    double engine_search_eval = 0.0;
    int engine_search_depth = 0;

    double white_time = 180.0;
    double black_time = 180.0;
    double selected_time = 180.0;

    int game_history_count = 0;
    game_history[game_history_count++] = hash_position(pos);

    gh.count = gh.current = 0;
    gh.positions[0] = pos;

    // ── Cached move list - only regenerated when position changes ──────────
    MoveList all_moves;
    bool moves_dirty = true;
    bool in_check = false;

    while (!WindowShouldClose())
    {
        double delta = GetFrameTime();
        if (delta > 0.5)
            delta = 0.0;

        BeginDrawing();
        ClearBackground(BG_COLOR);

        // ── MENUES ─────────────────────────────────────────────────────────
        if (state != PLAYING)
        {
            draw_selection_screen(state);
            if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON))
            {
                int mx = GetMouseX(), my = GetMouseY(), cx = WINDOW_WIDTH / 2;
                if (state == SELECTING_SIDE)
                {
                    if (mx >= cx - 180 && mx <= cx - 30 && my >= 310 && my <= 375)
                    {
                        engine_side = BLACK;
                        flipped = false;
                        state = SELECTING_TIME;
                    }
                    else if (mx >= cx + 30 && mx <= cx + 180 && my >= 310 && my <= 375)
                    {
                        engine_side = WHITE;
                        flipped = true;
                        state = SELECTING_TIME;
                    }
                }
                else
                {
                    vector<int> times = {1, 3, 5, 10, 15};
                    for (int i = 0; i < (int)times.size(); i++)
                    {
                        int bx = cx - 290 + i * 130;
                        if (mx >= bx && mx <= bx + 110 && my >= 310 && my <= 375)
                        {
                            selected_time = times[i] * 60.0;
                            white_time = black_time = selected_time;
                            human_turn = (engine_side == BLACK);
                            moves_dirty = true;
                            state = PLAYING;
                        }
                    }
                }
            }
            EndDrawing();
            continue;
        }

        // ── PLAYING ───────────────────────────────────────────────────────
        bool browsing = (gh.current < gh.count);

        // Rebuild move list only when position changed
        if (moves_dirty)
        {
            generate_moves(pos, all_moves);
            filter_legal_moves(pos, all_moves);
            in_check = is_in_check(pos, pos.side_to_move);
            moves_dirty = false;

            if (all_moves.count == 0 && !game_over && !browsing)
            {
                stop_ponder();
                game_over = true;
                if (in_check)
                {
                    game_over_is_checkmate = true;
                    game_over_msg = string(pos.side_to_move == WHITE ? "Black" : "White") + " wins by checkmate!";
                }
                else
                {
                    game_over_is_checkmate = false;
                    game_over_msg = "Stalemate — Draw!";
                }
            }
            else if (pos.halfmove_clock >= 100)
            {
                stop_ponder();
                game_over = true;
                game_over_is_checkmate = false;
                game_over_msg = "Draw by 50-move rule!";
            }
        }

        // Clocks
        if (!game_over && !browsing)
        {
            if (human_turn)
            {
                if (engine_side == BLACK)
                    white_time -= delta;
                else
                    black_time -= delta;
            }
            else if (engine_thinking || gui_pondering)
            {
                if (engine_side == WHITE)
                    white_time -= delta;
                else
                    black_time -= delta;
            }

            if (white_time <= 0)
            {
                game_over_msg = "Black wins on time!";
                game_over = true;
                game_over_is_checkmate = false;
                stop_ponder();
            }
            if (black_time <= 0)
            {
                game_over_msg = "White wins on time!";
                game_over = true;
                game_over_is_checkmate = false;
                stop_ponder();
            }
        }

        //====Keys====
        if (IsKeyPressed(KEY_F))
            flipped = !flipped;

        if (IsKeyPressed(KEY_R))
        {
            stop_ponder();
            stop_search = true;
            if (engine_thinking)
            {
                std::this_thread::sleep_for(std::chrono::milliseconds(10));
            }
            engine_thinking = engine_move_ready = false;
            stop_search = false;

            pos = parse_fen(starting_fen());
            gh.count = gh.current = 0;
            gh.positions[0] = pos;
            white_time = black_time = selected_time;
            last_move_str = "-";
            last_eval = 0.0;
            last_depth = 0;
            game_over = game_over_is_checkmate = false;
            selected = last_from = last_to = ponder_hint_sq = NO_SQUARE;
            game_history_count = 0;
            game_history[game_history_count++] = hash_position(pos);

            tt.clear();
            clear_killers();
            clear_history();
            out_of_book = false;
            ponder_best_so_far = {NO_SQUARE, NO_SQUARE, NO_PIECE_TYPE, NORMAL};

            moves_dirty = true;
            state = SELECTING_SIDE;
            EndDrawing();
            continue;
        }

        if (IsKeyPressed(KEY_LEFT) && gh.current > 0)
        {
            gh.current--;
            pos = gh.positions[gh.current];
            selected = NO_SQUARE;
            moves_dirty = true;
        }
        if (IsKeyPressed(KEY_RIGHT) && gh.current < gh.count)
        {
            gh.current++;
            pos = gh.positions[gh.current];
            selected = NO_SQUARE;
            moves_dirty = true;
        }

        browsing = (gh.current < gh.count);

        // ── DRAW ──────────────────────────────────────────────────────────
        draw_board(flipped);
        if (!browsing)
        {
            draw_last_move(last_from, last_to, flipped);
            if (gui_pondering && ponder_hint_sq != NO_SQUARE)
                draw_ponder_hint(ponder_hint_sq, flipped);
            if (!game_over && in_check)
            {
                // draw check highlight manually — no is_in_check() call here
                Bitboard kb = pos.side_to_move == WHITE ? pos.bitboards[WK] : pos.bitboards[BK];
                if (kb)
                {
                    int sx, sy;
                    sq_to_screen(static_cast<Square>(__builtin_ctzll(kb)), flipped, sx, sy);
                    DrawRectangle(sx, sy, SQUARE_SIZE, SQUARE_SIZE, HIGHLIGHT_CHECK);
                }
            }
            draw_highlights(all_moves, selected, flipped);
            if (game_over && !browsing && game_over_is_checkmate)
                draw_checkmate_kings(pos, flipped);
            draw_pieces(pos, flipped);
        }
        draw_pieces(pos, flipped);
        draw_clocks(white_time, black_time, pos.side_to_move, engine_side, flipped, (bool)gui_pondering);
        draw_info(last_depth, last_eval, last_move_str, gh.current, gh.count, (bool)gui_pondering);
        if (game_over && !browsing)
            draw_game_over(game_over_msg, game_over_is_checkmate, pos, flipped);

        // ── ENGINE THINK TRIGGER ──────────────────────────────────────────
        // Spawns the engine search on a detached thread when it is the engine's
        // turn. The thread writes the result to engine_best_move and sets
        // engine_move_ready; the main loop picks it up on the next frame without
        // blocking. A safety check re-runs legal move generation in case the
        // engine returns an illegal move (should never happen, guards against
        // rare TT corruption).
        if (!browsing && !game_over &&
            pos.side_to_move == engine_side &&
            !engine_thinking && !engine_move_ready)
        {
            engine_thinking = true;
            ponder_hint_sq = NO_SQUARE;

            double rem_ms = (engine_side == WHITE ? white_time : black_time) * 1000.0;
            Position tpos = pos;
            int tcount = game_history_count;
            uint64_t thist[1024];
            for (int i = 0; i < tcount; i++)
                thist[i] = game_history[i];

            stop_search = false;
            std::thread([&, tpos, tcount, thist, rem_ms]() mutable
                        {
                Move mv = best_move(tpos, 64, thist, tcount, (int)rem_ms, 0, false);

                if (mv.from_square != NO_SQUARE)
                {
                    UndoInfo v; make_move(tpos, mv, v);
                    bool illegal = is_in_check(tpos, tpos.side_to_move == WHITE ? BLACK : WHITE);
                    unmake_move(tpos, mv, v);
                    if (illegal)
                    {
                        MoveList safe; generate_moves(tpos, safe); filter_legal_moves(tpos, safe);
                        if (safe.count > 0) mv = safe.moves[0];
                    }
                }

                engine_best_move    = mv;
                engine_search_eval  = (evaluate(tpos) / 100.0) * (tpos.side_to_move == WHITE ? 1 : -1);
                engine_search_depth = g_last_depth;
                engine_move_ready   = true; })
                .detach();
        }

        //====ENGINE MOVE====
        if (engine_move_ready && !browsing)
        {
            engine_move_ready = false;
            engine_thinking = false;

            last_from = engine_best_move.from_square;
            last_to = engine_best_move.to_square;
            last_depth = engine_search_depth;
            last_eval = engine_search_eval;
            last_move_str = piece_to_string(engine_best_move.piecetype) + " " +
                            square_to_string(engine_best_move.from_square) +
                            square_to_string(engine_best_move.to_square);

            UndoInfo undo;
            make_move(pos, engine_best_move, undo);
            game_history[game_history_count++] = hash_position(pos);
            gh.moves[gh.count] = engine_best_move;
            gh.count++;
            gh.current = gh.count;
            gh.positions[gh.current] = pos;
            selected = NO_SQUARE;
            human_turn = true;
            moves_dirty = true; // position changed

            // After applying the engine's move, pick a ponder move (the expected
            // human reply) from the TT if available, otherwise fall back to the
            // first legal move. Apply it to a copy of the position and start the
            // background search on that copy.
            {
                Move pm = {NO_SQUARE, NO_SQUARE, NO_PIECE_TYPE, NORMAL};
                TTEntry *entry = tt.probe(pos.hash);
                if (entry && entry->best.from_square != NO_SQUARE)
                    pm = entry->best;
                else
                {
                    MoveList reply;
                    generate_moves(pos, reply);
                    filter_legal_moves(pos, reply);
                    if (reply.count > 0)
                        pm = reply.moves[0];
                }

                if (pm.from_square != NO_SQUARE)
                {
                    gui_ponder_move = pm;
                    ponder_hint_sq = pm.to_square;

                    gui_ponder_pos = pos;
                    gui_ponder_history_count = game_history_count;
                    for (int i = 0; i < game_history_count; i++)
                        gui_ponder_history[i] = game_history[i];

                    UndoInfo pundo;
                    make_move(gui_ponder_pos, pm, pundo);
                    gui_ponder_history[gui_ponder_history_count++] = hash_position(gui_ponder_pos);

                    start_ponder(gui_ponder_pos, gui_ponder_history, gui_ponder_history_count);
                }
            }
        }

        //====HUMAN MOVES====
        Colour human_side = engine_side == WHITE ? BLACK : WHITE;
        if (!browsing && !game_over &&
            pos.side_to_move == human_side && !engine_thinking)
        {
            if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON))
            {
                Square clicked = screen_to_square(GetMouseX(), GetMouseY(), flipped);
                if (clicked != NO_SQUARE)
                {
                    Bitboard hpieces = human_side == WHITE ? white_pieces(pos) : black_pieces(pos);
                    if (selected == NO_SQUARE)
                    {
                        if (square_set(hpieces, clicked))
                            selected = clicked;
                    }
                    else
                    {
                        bool moved = false;
                        for (int i = 0; i < all_moves.count; i++)
                        {
                            if (all_moves.moves[i].from_square != selected ||
                                all_moves.moves[i].to_square != clicked)
                                continue;

                            bool ponderhit = gui_pondering &&
                                             gui_ponder_move.from_square != NO_SQUARE &&
                                             all_moves.moves[i].from_square == gui_ponder_move.from_square &&
                                             all_moves.moves[i].to_square == gui_ponder_move.to_square;

                            stop_ponder();
                            ponder_hint_sq = NO_SQUARE;

                            if (ponderhit)
                            {
                                fprintf(stderr, "[GUI] ponderhit!\n");
                                fflush(stderr);
                            }

                            last_from = selected;
                            last_to = clicked;

                            UndoInfo undo;
                            make_move(pos, all_moves.moves[i], undo);
                            game_history[game_history_count++] = hash_position(pos);
                            gh.moves[gh.count] = all_moves.moves[i];
                            gh.count++;
                            gh.current = gh.count;
                            gh.positions[gh.current] = pos;

                            last_move_str = square_to_string(selected) + square_to_string(clicked);
                            selected = NO_SQUARE;
                            moved = true;
                            human_turn = false;
                            moves_dirty = true;
                            break;
                        }
                        if (!moved)
                        {
                            selected = NO_SQUARE;
                            Bitboard hp2 = human_side == WHITE ? white_pieces(pos) : black_pieces(pos);
                            if (square_set(hp2, clicked))
                                selected = clicked;
                        }
                    }
                }
            }
        }

        EndDrawing();
    }

    stop_ponder();
    unload_assets();
    CloseWindow();
}

// ============================================================
// legal.cpp
// ============================================================

#include "legal.h"
#include "movegen.h"

// ============================================================
// ATTACK DETECTION
// ============================================================

// Returns true if sq is attacked by any piece of the given colour.
// Uses a "reverse attack" technique: instead of enumerating attacker moves,
// we place a virtual piece of each type on sq and check whether its attack
// pattern intersects the real attacker bitboard. This is correct because
// attack patterns are symmetric (a knight on A attacks B iff a knight on B
// attacks A). Pawn attacks are not symmetric, so White and Black are handled
// separately: a square attacked by a White pawn is below-diagonal from that
// pawn, so we look at squares diagonally behind sq relative to the attacker's
// direction. Sliding piece attacks (rook/bishop) reuse the ray-casting
// generators from movegen.cpp, which already handle blockers correctly.
bool is_square_attacked(const Position &pos, Square sq, Colour attacker)
{
    // figure out right pieces to attack
    Bitboard pawns = attacker == WHITE ? pos.bitboards[WP] : pos.bitboards[BP];
    Bitboard knights = attacker == WHITE ? pos.bitboards[WN] : pos.bitboards[BN];
    Bitboard bishops = attacker == WHITE ? pos.bitboards[WB] : pos.bitboards[BB];
    Bitboard rooks = attacker == WHITE ? pos.bitboards[WR] : pos.bitboards[BR];
    Bitboard queens = attacker == WHITE ? pos.bitboards[WQ] : pos.bitboards[BQ];
    Bitboard kings = attacker == WHITE ? pos.bitboards[WK] : pos.bitboards[BK];
    Bitboard occupied = all_pieces(pos);
    Bitboard sq_bb = square_bb(sq);

    // knights
    Bitboard knight_attacks = 0ULL;
    knight_attacks |= (sq_bb << 17) & ~0x0101010101010101ULL;
    knight_attacks |= (sq_bb << 15) & ~0x8080808080808080ULL;
    knight_attacks |= (sq_bb << 10) & ~0x0303030303030303ULL;
    knight_attacks |= (sq_bb << 6) & ~0xC0C0C0C0C0C0C0C0ULL;
    knight_attacks |= (sq_bb >> 6) & ~0x0303030303030303ULL;
    knight_attacks |= (sq_bb >> 10) & ~0xC0C0C0C0C0C0C0C0ULL;
    knight_attacks |= (sq_bb >> 15) & ~0x0101010101010101ULL;
    knight_attacks |= (sq_bb >> 17) & ~0x8080808080808080ULL;
    if (knight_attacks & knights)
        return true;

    // king attacks
    Bitboard king_attacks = 0ULL;
    king_attacks |= ((sq_bb & ~0x8080808080808080ULL) << 1); // east
    king_attacks |= ((sq_bb & ~0x0101010101010101ULL) >> 1); // west
    king_attacks |= ((sq_bb & ~0x8080808080808080ULL) << 9); // NE
    king_attacks |= ((sq_bb & ~0x0101010101010101ULL) >> 9); // SW
    king_attacks |= ((sq_bb & ~0x0101010101010101ULL) << 7); // NW
    king_attacks |= ((sq_bb & ~0x8080808080808080ULL) >> 7); // SE
    king_attacks |= (sq_bb << 8);                            // north
    king_attacks |= (sq_bb >> 8);                            // south
    if (king_attacks & kings)
        return true;

    // pawn attacks
    if (attacker == WHITE)
    {
        // white pawns will attack up so check diagnoally below?
        Bitboard pawn_attacks = 0ULL;
        pawn_attacks |= (sq_bb >> 9) & ~0x8080808080808080ULL;
        pawn_attacks |= (sq_bb >> 7) & ~0x0101010101010101ULL;
        if (pawn_attacks & pawns)
            return true;
    }
    else
    {
        // black pawns downward then
        Bitboard pawn_attacks = 0ULL;
        pawn_attacks |= (sq_bb << 9) & ~0x0101010101010101ULL;
        pawn_attacks |= (sq_bb << 7) & ~0x8080808080808080ULL;
        if (pawn_attacks & pawns)
            return true;
    }

    // check the rook and queen (straights)
    Bitboard rook_queen = rooks | queens;
    if (rook_attacks(sq, occupied) & rook_queen)
        return true;

    // check the bishop and queen (diagnoal)
    Bitboard bishop_queen = bishops | queens;
    if (bishop_attacks(sq, occupied) & bishop_queen)
        return true;

    return false;
}
// ============================================================
// CHECK DETECTION
// ============================================================

// Returns true if the given colour's king is currently in check.
// Locates the king square via __builtin_ctzll (index of lowest set bit)
// then delegates to is_square_attacked() with the opposite colour as attacker.
bool is_in_check(const Position &pos, Colour colour)
{
    // find the kings square
    Bitboard king = colour == WHITE ? pos.bitboards[WK] : pos.bitboards[BK];
    Square king_sq = static_cast<Square>(__builtin_ctzll(king));

    // check the square attacked by enemey
    Colour attacker = colour == WHITE ? BLACK : WHITE;
    return is_square_attacked(pos, king_sq, attacker);
}

// ============================================================
// LEGALITY FILTERING
// ============================================================

// Reduces a pseudo-legal move list to only fully legal moves by making each
// move, checking whether the moving side's king is left in check, then
// unmaking it. Castling requires two extra checks before make_move: the king
// must not be in check on its origin square, and must not pass through an
// attacked square (the destination is already verified by generate_king_moves).
// The result overwrites the input list in place.
void filter_legal_moves(Position &pos, MoveList &list)
{
    MoveList legal;
    legal.count = 0;

    for (int i = 0; i < list.count; i++)
    {
        // special castling check — king cannot pass through attacked square
        if (list.moves[i].flag == CASTLING)
        {
            Colour enemy = pos.side_to_move == WHITE ? BLACK : WHITE;

            // check the square the king passes through
            Square pass_through;
            if (list.moves[i].to_square == G1)
                pass_through = F1;
            else if (list.moves[i].to_square == C1)
                pass_through = D1;
            else if (list.moves[i].to_square == G8)
                pass_through = F8;
            else
                pass_through = D8;

            if (is_square_attacked(pos, pass_through, enemy))
                continue;

            // also check king is not currently in check
            if (is_square_attacked(pos, list.moves[i].from_square, enemy))
                continue;
        }

        UndoInfo undo;
        make_move(pos, list.moves[i], undo);

        // check once moved still not in check
        if (!is_in_check(pos, undo.side_to_move))
        {
            add_move(legal, list.moves[i]);
        }

        unmake_move(pos, list.moves[i], undo);
    }

    list = legal;
}

// ============================================================
// makeunmake.cpp
// ============================================================

#include "makeunmake.h"
#include "zobrist.h"

// ============================================================
// PIECE LOOKUPS
// ============================================================

// Scans all 12 bitboards to find which piece occupies sq.
// Returns NO_PIECE if the square is empty. Called frequently during
// make/unmake so results are used immediately rather than cached.
Piece piece_on(const Position &pos, Square sq)
{
    for (int i = 0; i < PIECE_COUNT; i++)
    {
        if (square_set(pos.bitboards[i], sq))
            return static_cast<Piece>(i);
    }
    return NO_PIECE;
}

// ============================================================
// MAKE MOVES
// ============================================================

// Applies a move to the position and incrementally updates the Zobrist hash.
// All state needed to reverse the move (captured piece, castling rights,
// en passant square, clocks, hash) is saved into undo before anything changes.
// Hash updates follow the XOR-in/XOR-out pattern: remove the old value, apply
// the change, XOR in the new value. Special cases handled in order:
//   EN_PASSANT  — the captured pawn sits one rank behind the destination, not on it.
//   CASTLING    — the rook is also relocated; all four cases are handled explicitly.
//   PROMOTION   — the pawn on to_square is replaced by the promoted piece type;
//                 both the pawn and promoted piece are XOR'd into the hash separately.
// En passant rights are reset each move and only re-set for double pawn pushes
// (|diff| == 16). Castling rights are stripped whenever a king or corner rook moves
// or is captured, then the new rights value is XOR'd into the hashs
void make_move(Position &pos, const Move &move, UndoInfo &undo)
{
    // save current state
    undo.captured = piece_on(pos, move.to_square);
    undo.side_to_move = pos.side_to_move;
    undo.castling_rights = pos.castling_rights;
    undo.en_passant_square = pos.en_passant_square;
    undo.fullmove_clock = pos.fullmove_clock;
    undo.halfmove_clock = pos.halfmove_clock;
    undo.hash = pos.hash; // 1.14.0 bug fixes and overhaul updates

    uint64_t h = pos.hash;

    // figure out which piece is moving
    Piece piece = piece_on(pos, move.from_square);

    // XOR out old en passant if any
    if (pos.en_passant_square != NO_SQUARE)
        h ^= en_passant_keys[pos.en_passant_square % 8];

    // XOR out old castling rights
    h ^= castling_keys[pos.castling_rights];

    // remove captured piece from hash
    if (undo.captured != NO_PIECE)
    {
        pos.bitboards[undo.captured] &= ~square_bb(move.to_square);
        h ^= piece_keys[undo.captured][move.to_square];
    }

    // move the piece
    pos.bitboards[piece] &= ~square_bb(move.from_square);
    pos.bitboards[piece] |= square_bb(move.to_square);
    h ^= piece_keys[piece][move.from_square]; // remove from old square
    h ^= piece_keys[piece][move.to_square];   // add to new square

    // special moves
    if (move.flag == EN_PASSANT)
    {
        Square ep_pawn = static_cast<Square>(move.to_square + (pos.side_to_move == WHITE ? -8 : 8));
        Piece ep_piece = pos.side_to_move == WHITE ? BP : WP;
        pos.bitboards[ep_piece] &= ~square_bb(ep_pawn);
        h ^= piece_keys[ep_piece][ep_pawn];
    }

    if (move.flag == CASTLING)
    {
        if (move.to_square == G1)
        {
            pos.bitboards[WR] &= ~square_bb(H1);
            pos.bitboards[WR] |= square_bb(F1);
            h ^= piece_keys[WR][H1];
            h ^= piece_keys[WR][F1];
        }
        else if (move.to_square == C1)
        {
            pos.bitboards[WR] &= ~square_bb(A1);
            pos.bitboards[WR] |= square_bb(D1);
            h ^= piece_keys[WR][A1];
            h ^= piece_keys[WR][D1];
        }
        else if (move.to_square == G8)
        {
            pos.bitboards[BR] &= ~square_bb(H8);
            pos.bitboards[BR] |= square_bb(F8);
            h ^= piece_keys[BR][H8];
            h ^= piece_keys[BR][F8];
        }
        else if (move.to_square == C8)
        {
            pos.bitboards[BR] &= ~square_bb(A8);
            pos.bitboards[BR] |= square_bb(D8);
            h ^= piece_keys[BR][A8];
            h ^= piece_keys[BR][D8];
        }
    }

    if (move.flag == PROMOTION)
    {
        Piece promoted;
        if (pos.side_to_move == WHITE)
        {
            switch (move.piecetype)
            {
            case QUEEN:
                promoted = WQ;
                break;
            case ROOK:
                promoted = WR;
                break;
            case BISHOP:
                promoted = WB;
                break;
            case KNIGHT:
                promoted = WN;
                break;
            default:
                promoted = WQ;
                break;
            }
        }
        else
        {
            switch (move.piecetype)
            {
            case QUEEN:
                promoted = BQ;
                break;
            case ROOK:
                promoted = BR;
                break;
            case BISHOP:
                promoted = BB;
                break;
            case KNIGHT:
                promoted = BN;
                break;
            default:
                promoted = BQ;
                break;
            }
        }
        // remove pawn, add promoted piece
        h ^= piece_keys[piece][move.to_square]; // remove pawn from to_square
        pos.bitboards[piece] &= ~square_bb(move.to_square);
        pos.bitboards[promoted] |= square_bb(move.to_square);
        h ^= piece_keys[promoted][move.to_square]; // add promoted piece
    }

    // update en passant hash
    pos.en_passant_square = NO_SQUARE;
    if (move.piecetype == PAWN)
    {
        int diff = move.to_square - move.from_square;
        if (diff == 16)
        {
            pos.en_passant_square = static_cast<Square>(move.from_square + 8);
            h ^= en_passant_keys[pos.en_passant_square % 8];
        }
        else if (diff == -16)
        {
            pos.en_passant_square = static_cast<Square>(move.from_square - 8);
            h ^= en_passant_keys[pos.en_passant_square % 8];
        }
    }

    // update castling rights
    if (piece == WK)
        pos.castling_rights &= ~(WHITE_00 | WHITE_000);
    if (piece == BK)
        pos.castling_rights &= ~(BLACK_00 | BLACK_000);
    if (move.from_square == A1 || move.to_square == A1)
        pos.castling_rights &= ~WHITE_000;
    if (move.from_square == A8 || move.to_square == A8)
        pos.castling_rights &= ~BLACK_000;
    if (move.from_square == H1 || move.to_square == H1)
        pos.castling_rights &= ~WHITE_00;
    if (move.from_square == H8 || move.to_square == H8)
        pos.castling_rights &= ~BLACK_00;

    // XOR in new castling rights
    h ^= castling_keys[pos.castling_rights];

    // flip side to move
    h ^= side_key;

    // update clocks
    pos.halfmove_clock++;
    if (undo.captured != NO_PIECE || move.piecetype == PAWN)
        pos.halfmove_clock = 0;
    if (pos.side_to_move == BLACK)
        pos.fullmove_clock++;
    pos.side_to_move = (pos.side_to_move == WHITE) ? BLACK : WHITE;

    pos.hash = h;
}

// ============================================================
// UNMAKE MOVE
// ============================================================

// Reverses a move by restoring all position state from the UndoInfo saved during
// make_move. The hash is restored directly from undo rather than being
// recomputed incrementally — this is simpler and equally correct since the saved
// hash is guaranteed to be consistent with the restored state. Promotion is
// handled first: the promoted piece on to_square is the removed and the original pawn
// is restored on from_square. En passant restores the captured pawn on the square
// behind the destination. Castling moves the rook back to its original corner.
void unmake_move(Position &pos, const Move &move, const UndoInfo &undo)
{
    // just restore the full state - hash restores automatically
    pos.side_to_move = undo.side_to_move;

    Piece piece = piece_on(pos, move.to_square);

    if (move.flag == PROMOTION)
    {
        Piece promoted;
        if (pos.side_to_move == WHITE)
        {
            switch (move.piecetype)
            {
            case QUEEN:
                promoted = WQ;
                break;
            case ROOK:
                promoted = WR;
                break;
            case BISHOP:
                promoted = WB;
                break;
            case KNIGHT:
                promoted = WN;
                break;
            default:
                promoted = WQ;
                break;
            }
        }
        else
        {
            switch (move.piecetype)
            {
            case QUEEN:
                promoted = BQ;
                break;
            case ROOK:
                promoted = BR;
                break;
            case BISHOP:
                promoted = BB;
                break;
            case KNIGHT:
                promoted = BN;
                break;
            default:
                promoted = BQ;
                break;
            }
        }
        pos.bitboards[promoted] &= ~square_bb(move.to_square);
        Piece pawn = pos.side_to_move == WHITE ? WP : BP;
        pos.bitboards[pawn] |= square_bb(move.from_square);
    }
    else
    {
        pos.bitboards[piece] &= ~square_bb(move.to_square);
        pos.bitboards[piece] |= square_bb(move.from_square);
    }

    if (undo.captured != NO_PIECE)
        pos.bitboards[undo.captured] |= square_bb(move.to_square);

    if (move.flag == EN_PASSANT)
    {
        Square ep_pawn = static_cast<Square>(move.to_square + (pos.side_to_move == WHITE ? -8 : 8));
        Piece ep_piece = pos.side_to_move == WHITE ? BP : WP;
        pos.bitboards[ep_piece] |= square_bb(ep_pawn);
    }

    if (move.flag == CASTLING)
    {
        if (move.to_square == G1)
        {
            pos.bitboards[WR] &= ~square_bb(F1);
            pos.bitboards[WR] |= square_bb(H1);
        }
        else if (move.to_square == C1)
        {
            pos.bitboards[WR] &= ~square_bb(D1);
            pos.bitboards[WR] |= square_bb(A1);
        }
        else if (move.to_square == G8)
        {
            pos.bitboards[BR] &= ~square_bb(F8);
            pos.bitboards[BR] |= square_bb(H8);
        }
        else if (move.to_square == C8)
        {
            pos.bitboards[BR] &= ~square_bb(D8);
            pos.bitboards[BR] |= square_bb(A8);
        }
    }

    pos.castling_rights = undo.castling_rights;
    pos.en_passant_square = undo.en_passant_square;
    pos.halfmove_clock = undo.halfmove_clock;
    pos.fullmove_clock = undo.fullmove_clock;

    // restore hash from undo - recompute since we restored all state
    // simplest correct approach: just recompute
    pos.hash = undo.hash; // 1.14.0 fixes
}

// ============================================================
// movegen.cpp
// ============================================================
#include "movegen.h"
#include "search.h"
#include "legal.h"
#include <iostream>

// ============================================================
// KING MOVES
// ============================================================

// Generates all pseudo-legal king moves: one step in all eight directions with
// friendly-piece blocking, plus castling. Each diagonal/orthogonal shift is masked
// against the A-file (0x0101...ULL) or H-file (0x8080...ULL) to prevent the king
// from wrapping around the board edge. Castling checks that both the transit and
// destination squares are unoccupied and not attacked, as required by the rules.
void generate_king_moves(const Position &pos, MoveList &list)
{

    Bitboard king = pos.side_to_move == WHITE ? pos.bitboards[WK] : pos.bitboards[BK];
    Bitboard friendly = friendly_pieces(pos);

    int from = __builtin_ctzll(king);

    Bitboard attacks = 0ULL;
    attacks |= (king << 8);                          // north
    attacks |= (king >> 8);                          // south
    attacks |= (king << 1) & ~0x0101010101010101ULL; // east
    attacks |= (king >> 1) & ~0x8080808080808080ULL; // westa
    attacks |= (king << 9) & ~0x0101010101010101ULL; // north east
    attacks |= (king << 7) & ~0x8080808080808080ULL; // north west
    attacks |= (king >> 7) & ~0x0101010101010101ULL; // south east
    attacks |= (king >> 9) & ~0x8080808080808080ULL; // south west

    attacks &= ~friendly;

    while (attacks)
    {
        int to = __builtin_ctzll(attacks);
        Move move;
        move.from_square = static_cast<Square>(from);
        move.to_square = static_cast<Square>(to);
        move.flag = NORMAL;
        move.piecetype = KING;
        add_move(list, move);
        // clear squares by friendly pieces
        attacks &= attacks - 1;
    }

    // castling lol dugongs dugongs dugongs

    if (pos.side_to_move == WHITE)
    {
        Bitboard occ = all_pieces(pos);

        // kingside - just check squares are empty
        if ((pos.castling_rights & WHITE_00) &&
            !(occ & square_bb(F1)) &&
            !(occ & square_bb(G1)) &&
            !is_square_attacked(pos, E1, BLACK) &&
            !is_square_attacked(pos, F1, BLACK) &&
            !is_square_attacked(pos, G1, BLACK))
        {
            Move move;
            move.from_square = E1;
            move.to_square = G1;
            move.flag = CASTLING;
            move.piecetype = KING;
            add_move(list, move);
        }

        // queenside
        if ((pos.castling_rights & WHITE_000) &&
            !(occ & square_bb(B1)) &&
            !(occ & square_bb(C1)) &&
            !(occ & square_bb(D1)) &&
            !is_square_attacked(pos, E1, BLACK) &&
            !is_square_attacked(pos, D1, BLACK) &&
            !is_square_attacked(pos, C1, BLACK))
        {
            Move move;
            move.from_square = E1;
            move.to_square = C1;
            move.flag = CASTLING;
            move.piecetype = KING;
            add_move(list, move);
        }
    }
    else
    {
        Bitboard occ = all_pieces(pos);

        // BLACK kingside
        if ((pos.castling_rights & BLACK_00) &&
            !(occ & square_bb(F8)) &&
            !(occ & square_bb(G8)) &&
            !is_square_attacked(pos, E8, WHITE) &&
            !is_square_attacked(pos, F8, WHITE) &&
            !is_square_attacked(pos, G8, WHITE))
        {
            Move move;
            move.from_square = E8;
            move.to_square = G8;
            move.flag = CASTLING;
            move.piecetype = KING;
            add_move(list, move);
        }

        // queenside black
        if ((pos.castling_rights & BLACK_000) &&
            !(occ & square_bb(B8)) &&
            !(occ & square_bb(C8)) &&
            !(occ & square_bb(D8)) &&
            !is_square_attacked(pos, E8, WHITE) &&
            !is_square_attacked(pos, D8, WHITE) &&
            !is_square_attacked(pos, C8, WHITE))
        {
            Move move;
            move.from_square = E8;
            move.to_square = C8;
            move.flag = CASTLING;
            move.piecetype = KING;
            add_move(list, move);
        }
    }
}

// ============================================================
// KNIGHT MOVES
// ============================================================

// Generates all pseudo-legal knight moves for every knight of the side to move.
// Each of the 8 L-shaped leaps is computed by shifting the knight's bitboard and
// masking off wrap-around: jumps going 2 east mask the A+B files (0x0303...ULL),
// jumps going 2 west mask the G+H files (0xC0C0...ULL), and 1-file jumps mask
// only the single edge file, preventing the knight from appearing on the opposite
// side of the board.
void generate_knight_moves(const Position &pos, MoveList &list)
{
    Bitboard knights = pos.side_to_move == WHITE ? pos.bitboards[WN] : pos.bitboards[BN];
    Bitboard friendly = friendly_pieces(pos);

    while (knights)
    {
        int from = __builtin_ctzll(knights);
        Bitboard knight = 1ULL << from;

        Bitboard attacks = 0ULL;
        attacks |= (knight << 17) & ~0x0101010101010101ULL; // 2N 1E — dest can't be A file
        attacks |= (knight << 15) & ~0x8080808080808080ULL; // 2N 1W — dest can't be H file
        attacks |= (knight << 10) & ~0x0303030303030303ULL; // 1N 2E — dest can't be A or B file
        attacks |= (knight << 6) & ~0xC0C0C0C0C0C0C0C0ULL;  // 1N 2W — dest can't be G or H file
        attacks |= (knight >> 6) & ~0x0303030303030303ULL;  // 1S 2E — dest can't be A or B file
        attacks |= (knight >> 10) & ~0xC0C0C0C0C0C0C0C0ULL; // 1S 2W — dest can't be G or H file
        attacks |= (knight >> 15) & ~0x0101010101010101ULL; // 2S 1E — dest can't be A file
        attacks |= (knight >> 17) & ~0x8080808080808080ULL; // 2S 1W — dest can't be H file

        attacks &= ~friendly;

        while (attacks)
        {
            int to = __builtin_ctzll(attacks);
            Move move;
            move.from_square = static_cast<Square>(from);
            move.to_square = static_cast<Square>(to);
            move.flag = NORMAL;
            move.piecetype = KNIGHT;
            add_move(list, move);
            attacks &= attacks - 1;
        }

        knights &= knights - 1;
    }
}

// ============================================================
// SLIDING PIECE ATTACK GENERATORS
// ============================================================

// Returns a bitboard of all squares a rook on sq can attack given the occupied
// squares. Rays are cast in all four orthogonal directions (north, south, east,
// west); each ray includes the first occupied square it hits (a capture target)
// but stops there so pieces behind it are not falsely attacked.
Bitboard rook_attacks(Square sq, Bitboard occupied)
{
    Bitboard attacks = 0ULL;
    int r = sq / 8, f = sq % 8;

    // north movement up
    for (int i = r + 1; i < 8; i++)
    {
        attacks |= 1ULL << (i * 8 + f);
        if (occupied & (1ULL << (i * 8 + f)))
            break;
    }
    // south movement down
    for (int i = r - 1; i >= 0; i--)
    {
        attacks |= 1ULL << (i * 8 + f);
        if (occupied & (1ULL << (i * 8 + f)))
            break;
    }
    // move east left
    for (int i = f + 1; i < 8; i++)
    {
        attacks |= 1ULL << (r * 8 + i);
        if (occupied & (1ULL << (r * 8 + i)))
            break;
    }
    // move west right
    for (int i = f - 1; i >= 0; i--)
    {
        attacks |= 1ULL << (r * 8 + i);
        if (occupied & (1ULL << (r * 8 + i)))
            break;
    }

    return attacks;
}

// Returns a bitboard of all squares a bishop on sq can attack given the occupied
// squares. Identical ray-casting logic to rook_attacks but along the four diagonal
// directions (NE, NW, SE, SW). Both i (rank) and j (file) are stepped together
// and the loop terminates when either board edge or a blocker is reached.
// bishop sliding
Bitboard bishop_attacks(Square sq, Bitboard occupied)
{
    Bitboard attacks = 0ULL;
    int r = sq / 8, f = sq % 8;

    // north east
    for (int i = r + 1, j = f + 1; i < 8 && j < 8; i++, j++)
    {
        attacks |= 1ULL << (i * 8 + j);
        if (occupied & (1ULL << (i * 8 + j)))
            break;
    }

    // nort west
    for (int i = r + 1, j = f - 1; i < 8 && j >= 0; i++, j--)
    {
        attacks |= 1ULL << (i * 8 + j);
        if (occupied & (1ULL << (i * 8 + j)))
            break;
    }

    // south east
    for (int i = r - 1, j = f + 1; i >= 0 && j < 8; i--, j++)
    {
        attacks |= 1ULL << (i * 8 + j);
        if (occupied & (1ULL << (i * 8 + j)))
            break;
    }

    // south west
    for (int i = r - 1, j = f - 1; i >= 0 && j >= 0; i--, j--)
    {
        attacks |= 1ULL << (i * 8 + j);
        if (occupied & (1ULL << (i * 8 + j)))
            break;
    }

    return attacks;
}

// Generates pseudo-legal moves for rooks, bishops, and queens by calling the
// corresponding attack generators and iterating destination squares. Queen moves
// are the union of rook and bishop attacks — no separate ray logic is needed.
// rooks + bishops + queens
void generate_sliding_moves(const Position &pos, MoveList &list)
{
    Bitboard friendly = friendly_pieces(pos);
    Bitboard occupied = all_pieces(pos);

    // rooks
    Bitboard rooks = pos.side_to_move == WHITE ? pos.bitboards[WR] : pos.bitboards[BR];
    while (rooks)
    {
        int from = __builtin_ctzll(rooks);
        Bitboard attacks = rook_attacks(static_cast<Square>(from), occupied) & ~friendly;

        while (attacks)
        {
            int to = __builtin_ctzll(attacks);
            Move move;
            move.from_square = static_cast<Square>(from);
            move.to_square = static_cast<Square>(to);
            move.flag = NORMAL;
            move.piecetype = ROOK;
            add_move(list, move);
            attacks &= attacks - 1;
        }
        rooks &= rooks - 1;
    }

    // bishops
    Bitboard bishops = pos.side_to_move == WHITE ? pos.bitboards[WB] : pos.bitboards[BB];
    while (bishops)
    {
        int from = __builtin_ctzll(bishops);
        Bitboard attacks = bishop_attacks(static_cast<Square>(from), occupied) & ~friendly;

        while (attacks)
        {
            int to = __builtin_ctzll(attacks);
            Move move;
            move.from_square = static_cast<Square>(from);
            move.to_square = static_cast<Square>(to);
            move.flag = NORMAL;
            move.piecetype = BISHOP;
            add_move(list, move);
            attacks &= attacks - 1;
        }
        bishops &= bishops - 1;
    }

    // queens = rook + bishop
    Bitboard queens = pos.side_to_move == WHITE ? pos.bitboards[WQ] : pos.bitboards[BQ];
    while (queens)
    {
        int from = __builtin_ctzll(queens);
        Bitboard attacks = (rook_attacks(static_cast<Square>(from), occupied) | bishop_attacks(static_cast<Square>(from), occupied)) & ~friendly;

        while (attacks)
        {
            int to = __builtin_ctzll(attacks);
            Move move;
            move.from_square = static_cast<Square>(from);
            move.to_square = static_cast<Square>(to);
            move.flag = NORMAL;
            move.piecetype = QUEEN;
            add_move(list, move);
            attacks &= attacks - 1;
        }
        queens &= queens - 1;
    }
}

// ============================================================
// PAWN MOVES
// ============================================================

// Generates all pseudo-legal pawn moves: single push, double push from the
// starting rank, diagonal captures, and en passant. The double-push mask
// (0x0000000000FF0000ULL for White, 0x0000FF0000000000ULL for Black) ensures
// the pawn is still on its starting rank before the second step is allowed.
// Diagonal captures are masked against the A-file or H-file to prevent
// wrap-around. All four promotions (Q/R/B/N) are generated whenever a pawn
// reaches the back rank, including promotion captures.
// pawn moves
void generate_pawn_moves(const Position &pos, MoveList &list)
{
    Bitboard pawns = pos.side_to_move == WHITE ? pos.bitboards[WP] : pos.bitboards[BP];
    Bitboard empty = empty_squares(pos);
    Bitboard enemy = enemy_pieces(pos);

    while (pawns)
    {
        int from = __builtin_ctzll(pawns);
        Bitboard pawn = 1ULL << from;

        if (pos.side_to_move == WHITE)
        {
            Bitboard single_push = (pawn << 8) & empty;
            Bitboard double_push = ((single_push & 0x0000000000FF0000ULL) << 8) & empty;
            Bitboard captures = 0ULL;
            captures |= (pawn << 9) & ~0x0101010101010101ULL & enemy;
            captures |= (pawn << 7) & ~0x8080808080808080ULL & enemy;

            if (pos.en_passant_square != NO_SQUARE)
            {
                Bitboard ep = square_bb(pos.en_passant_square);
                captures |= (pawn << 9) & ~0x0101010101010101ULL & ep;
                captures |= (pawn << 7) & ~0x8080808080808080ULL & ep;
            }

            while (single_push)
            {
                int to = __builtin_ctzll(single_push);
                Move move;
                move.from_square = static_cast<Square>(from);
                move.to_square = static_cast<Square>(to);
                move.piecetype = PAWN;

                if (to >= 56)
                {
                    // generate all 4 promots 1.5.6 debugging
                    for (PieceType pt : {QUEEN, ROOK, BISHOP, KNIGHT})
                    {
                        move.flag = PROMOTION;
                        move.piecetype = pt;
                        add_move(list, move);
                    }
                }

                else
                {
                    move.flag = NORMAL;
                    add_move(list, move);
                }
                single_push &= single_push - 1;
            }

            while (double_push)
            {
                int to = __builtin_ctzll(double_push);
                Move move;
                move.from_square = static_cast<Square>(from);
                move.to_square = static_cast<Square>(to);
                move.piecetype = PAWN;
                move.flag = NORMAL;
                add_move(list, move);
                double_push &= double_push - 1;
            }

            while (captures)
            {
                int to = __builtin_ctzll(captures);
                Move move;
                move.from_square = static_cast<Square>(from);
                move.to_square = static_cast<Square>(to);
                if (to >= 56) // promotion capture
                {
                    for (PieceType pt : {QUEEN, ROOK, BISHOP, KNIGHT})
                    {
                        move.flag = PROMOTION;
                        move.piecetype = pt;
                        add_move(list, move);
                    }
                }
                else
                {
                    move.piecetype = PAWN;
                    move.flag = (pos.en_passant_square != NO_SQUARE &&
                                 to == pos.en_passant_square)
                                    ? EN_PASSANT
                                    : NORMAL;
                    add_move(list, move);
                }
                captures &= captures - 1;
            }
        }
        else
        {
            Bitboard single_push = (pawn >> 8) & empty;
            Bitboard double_push = ((single_push & 0x0000FF0000000000ULL) >> 8) & empty;
            Bitboard captures = 0ULL;
            captures |= (pawn >> 7) & ~0x0101010101010101ULL & enemy;
            captures |= (pawn >> 9) & ~0x8080808080808080ULL & enemy;

            if (pos.en_passant_square != NO_SQUARE)
            {
                Bitboard ep = square_bb(pos.en_passant_square);
                captures |= (pawn >> 7) & ~0x0101010101010101ULL & ep;
                captures |= (pawn >> 9) & ~0x8080808080808080ULL & ep;
            }

            while (single_push)
            {
                int to = __builtin_ctzll(single_push);
                Move move;
                move.from_square = static_cast<Square>(from);
                move.to_square = static_cast<Square>(to);
                move.piecetype = PAWN;

                if (to <= 7)
                {
                    // generate all 4 promots 1.5.6 debugging
                    for (PieceType pt : {QUEEN, ROOK, BISHOP, KNIGHT})
                    {
                        move.flag = PROMOTION;
                        move.piecetype = pt;
                        add_move(list, move);
                    }
                }

                else
                {
                    move.flag = NORMAL;
                    add_move(list, move);
                }
                single_push &= single_push - 1;
            }

            while (double_push)
            {
                int to = __builtin_ctzll(double_push);
                Move move;
                move.from_square = static_cast<Square>(from);
                move.to_square = static_cast<Square>(to);
                move.piecetype = PAWN;
                move.flag = NORMAL;
                add_move(list, move);
                double_push &= double_push - 1;
            }

            while (captures)
            {
                int to = __builtin_ctzll(captures);
                Move move;
                move.from_square = static_cast<Square>(from);
                move.to_square = static_cast<Square>(to);

                if (to <= 7) // promotion capture
                {
                    for (PieceType pt : {QUEEN, ROOK, BISHOP, KNIGHT})
                    {
                        move.flag = PROMOTION;
                        move.piecetype = pt;
                        add_move(list, move);
                    }
                }
                else
                {
                    move.piecetype = PAWN;
                    move.flag = (pos.en_passant_square != NO_SQUARE &&
                                 to == pos.en_passant_square)
                                    ? EN_PASSANT
                                    : NORMAL;
                    add_move(list, move);
                }
                captures &= captures - 1;
            }
        }
        pawns &= pawns - 1;
    }
}

// ============================================================
// TOP-LEVEL MOVE GENERATION
// ============================================================

// Resets the move list and calls each piece-type generator in turn.
// Produces pseudo-legal moves — legality (leaving own king in check) is
// filtered separately by filter_legal_moves() in legal.cpp.
void generate_moves(const Position &pos, MoveList &list)
{
    list.count = 0;
    generate_king_moves(pos, list);
    generate_knight_moves(pos, list);
    generate_sliding_moves(pos, list);
    generate_pawn_moves(pos, list);
}

// ============================================================
// MOVE ORDERING
// ============================================================

// Assigns a numeric score to a move for ordering purposes. Captures are scored
// using MVV-LVA (Most Valuable Victim – Least Valuable Attacker): a table indexed
// by [attacker type][victim type] gives scores in the range 100–605, offset by
// +10000 so all captures outrank quiet moves. En passant is treated as a
// pawn-takes-pawn capture (score 105 + 10000). Quiet moves that match a killer
// slot at this depth score 9000 or 8000 respectively. All other quiet moves fall
// back to the history heuristic table (from-square × to-square).
// performance upgrades 1.1.0
// performance upgrade 1.7.0 implement killer heurestic
int score_move(const Position &pos, const Move &move, int depth) // 1.12.1
{
    // capturing a piece now gets a high base score
    Piece captured = piece_on(pos, move.to_square);

    if (captured != NO_PIECE || move.flag == EN_PASSANT)
    {
        if (move.flag == EN_PASSANT)
            return 105 + 10000;

        // MVV-LVA capture piece value - attacking piece value
        const int mvv_lva[6][6] = {
            // victim : P N B R Q K
            {105, 205, 305, 405, 505, 605}, // attacker P
            {104, 204, 304, 404, 504, 604}, // attacker N
            {103, 203, 303, 403, 503, 603}, // attacker B
            {102, 202, 302, 402, 502, 602}, // attacker R
            {101, 201, 301, 401, 501, 601}, // attacker Q
            {100, 200, 300, 400, 500, 600}, // attacker K
        };

        // get tghe attacker and victim piece types
        Piece attacker = piece_on(pos, move.from_square);
        int attacker_type = attacker % 6; // strip the colour
        int victim_type = captured % 6;   // strip colour

        return mvv_lva[attacker_type][victim_type] + 10000; // high enough base score? tweak later
    }

    // peformance upgrade 1.7.0 for the killer move heurestic bonuses
    if (depth >= 0 && depth < MAX_DEPTH)
    {
        if (move.from_square == killer_moves[depth][0].from_square && move.to_square == killer_moves[depth][0].to_square)
            return 9000;
        if (move.from_square == killer_moves[depth][1].from_square && move.to_square == killer_moves[depth][1].to_square)
            return 8000;
    }

    return history_table[move.from_square][move.to_square]; // performance upgrade 1.8.0 history heurestic lol dugongs :D
}

// Sorts the move list in descending order of score_move() using insertion sort.
// Insertion sort is efficient here because move lists are short (typically < 50)
// and are often nearly sorted already due to the TT best-move being placed first.
void order_moves(const Position &pos, MoveList &list, int depth)
{
    int scores[256];
    for (int i = 0; i < list.count; i++)
        scores[i] = score_move(pos, list.moves[i], depth);

    for (int i = 1; i < list.count; i++)
    {
        Move key = list.moves[i];
        int key_score = scores[i];
        int j = i - 1;
        while (j >= 0 && scores[j] < key_score)
        {
            list.moves[j + 1] = list.moves[j];
            scores[j + 1] = scores[j];
            j--;
        }
        list.moves[j + 1] = key;
        scores[j + 1] = key_score;
    }
}

// ============================================================
// search.cpp
// ============================================================

#include "search.h"
#include "transposition.h"
#include "book.h"
#include <iostream>
#include <algorithm>
#include <chrono>

extern uint64_t side_key;

Move killer_moves[MAX_DEPTH][2];
int history_table[64][64];
int g_last_depth = 0;

std::atomic<bool> stop_search(false);
Move ponder_move = {NO_SQUARE, NO_SQUARE, NO_PIECE_TYPE, NORMAL}; // 1.15.0.0 pondering
Move ponder_best_so_far = {NO_SQUARE, NO_SQUARE, NO_PIECE_TYPE, NORMAL};

// Resets all killer move slots to empty (NO_SQUARE) at the start of a new search.
// Killer moves from a previous search are no longer valid for the new position's depth table.
void clear_killers()
{
    for (int i = 0; i < MAX_DEPTH; i++)
    {
        killer_moves[i][0] = {NO_SQUARE, NO_SQUARE, NO_PIECE_TYPE, NORMAL};
        killer_moves[i][1] = {NO_SQUARE, NO_SQUARE, NO_PIECE_TYPE, NORMAL};
    }
}

// Zeros out the entire history heuristic table (from-square x to-square).
// Must be called before each new search so stale scores from prior positions don't
// pollute move ordering.
void clear_history()
{
    for (int i = 0; i < 64; i++)
        for (int j = 0; j < 64; j++)
            history_table[i][j] = 0;
}

// ============================================================
// TIME MANAGEMENT
// ============================================================

// Determines how many milliseconds to budget for the current move given the
// remaining clock time and increment. Uses a ~30-moves-remaining heuristic and
// caps the result so a single move can never consume more than 10% of the clock,
// with a hard floor of 50ms to avoid degenerate instant moves.
int calculate_time(int remaining_ms, int increment_ms)
{
    // ~30 moves remaining assumption, use most of the increment
    int time_for_move = (remaining_ms / 30) + (increment_ms * 3 / 4);

    // Never use more than 10% of remaining time on one move
    int max_single_move = remaining_ms / 10;
    if (time_for_move > max_single_move)
        time_for_move = max_single_move;

    // Hard floor: always think at least 50ms to avoid instant moves
    if (time_for_move < 50)
        time_for_move = 50;

    return time_for_move;
}

// ============================================================
// QUIESCENCE SEARCH
// ============================================================

// Extends the search beyond the horizon by evaluating only captures (and en passant)
// until the position is "quiet". This prevents the horizon effect where a bad capture
// just outside the main search depth looks artificially good because the recapture
// is never seen. Uses a stand-pat score as a lower bound: if the static eval already
// beats beta, we can return immediately (the opponent wouldn't allow this position)
int quiescence(Position &pos, int alpha, int beta, int depth)
{
    // Hard depth limit to prevent runaway q-search in crazy positions
    if (depth <= -12)
        return evaluate(pos);

    int stand_pat = evaluate(pos);

    if (stand_pat >= beta)
        return beta;
    if (stand_pat > alpha)
        alpha = stand_pat;

    MoveList list;
    generate_moves(pos, list);
    filter_legal_moves(pos, list);
    order_moves(pos, list, depth);

    for (int i = 0; i < list.count; i++)
    {
        // Only search captures and en passant
        if (piece_on(pos, list.moves[i].to_square) == NO_PIECE &&
            list.moves[i].flag != EN_PASSANT)
            continue;

        UndoInfo undo;
        make_move(pos, list.moves[i], undo);
        int score = -quiescence(pos, -beta, -alpha, depth - 1);
        unmake_move(pos, list.moves[i], undo);

        if (score >= beta)
            return beta;
        if (score > alpha)
            alpha = score;
    }

    return alpha;
}

// Core recursive alpha-beta search with several enhancements:
// transposition table (TT) probing, null-move pruning, reverse futility pruning,
// forward futility pruning, Late Move Reductions (LMR), and Principal Variation
// Search (PVS). Returns the score of the position from the perspective of the
// side to move. Alpha is the best score the maximising side can already guarantee;
// beta is the best score the minimising side can already guarantee. A score >= beta
// causes an immediate cutoff (the opponent won't allow this line).
int alphaBeta(Position &pos, int depth, int alpha, int beta,
              uint64_t *stack, int stack_size, bool null_allowed)
{
    uint64_t hash = pos.hash;
    // A PV node has a window wider than 1 (alpha+1 < beta). Zero-window calls
    // (alpha == beta-1) are used for LMR/PVS probes and must not be treated as PV
    // nodes, otherwise we'd skip TT cutoffs on every reduced search.
    bool is_pv = (beta - alpha > 1);

    // ---- Repetition: always 0, no clever scoring ----
    for (int i = 0; i < stack_size - 1; i++)
        if (stack[i] == hash)
        {
            return 0;
        }

    // ---- TT probe ----
    // Skip TT cutoffs on PV nodes so we don't collapse the PV line.
    TTEntry *entry = tt.probe(hash);
    if (entry != nullptr && entry->depth >= depth && !is_pv)
    {
        if (entry->flag == TT_EXACT)
            return entry->score;
        if (entry->flag == TT_ALPHA && entry->score <= alpha)
            return alpha;
        if (entry->flag == TT_BETA && entry->score >= beta)
            return beta;
    }

    // ---- Leaf node ----
    if (depth == 0)
        return quiescence(pos, alpha, beta, depth);

    bool in_check = is_in_check(pos, pos.side_to_move);

    // ---- Static eval (computed once, reused by both futility checks) ----
    // Don't compute if in check. Eval will be unreliable when the king is attacked.
    int static_eval = (!in_check) ? evaluate(pos) : -INF;

    // ---- Reverse futility pruning (static null move) ----
    // If static eval is so far above beta that even a "bad" move won't fall
    // below it, we can safely return early. Only at shallow depths, not in
    // check, not on PV nodes.
    if (!is_pv && !in_check && depth <= 3)
    {
        int rfp_margin = 120 * depth; // 120cp per depth: 120/240/360
        if (static_eval - rfp_margin >= beta)
            return static_eval;
    }

    //---- Null Move Pruning ----
    // If we can skip our turn entirely and the opponent still can't beat beta
    // at a reduced depth, the position is so good that a real move will also
    // beat beta — return early. Reduction R=2 means we search depth-3.
    // Disabled in check (zugzwang risk is high), on PV nodes, and when the
    // calling node already used a null move (null_allowed=false prevents
    // consecutive null moves which would be unsound).
    const int R = 2;
    if (null_allowed && !is_pv && !in_check && depth >= R + 1)
    {
        Square old_ep = pos.en_passant_square;
        uint64_t old_hash = pos.hash;

        if (old_ep != NO_SQUARE)
            pos.hash ^= en_passant_keys[old_ep % 8];
        pos.hash ^= side_key;

        pos.en_passant_square = NO_SQUARE;
        pos.side_to_move = (pos.side_to_move == WHITE) ? BLACK : WHITE;

        stack[stack_size] = pos.hash;
        int null_score = -alphaBeta(pos, depth - R - 1, -beta, -beta + 1,
                                    stack, stack_size + 1, false);

        pos.side_to_move = (pos.side_to_move == WHITE) ? BLACK : WHITE;
        pos.en_passant_square = old_ep;
        pos.hash = old_hash;

        if (null_score >= beta)
            return beta;
    }

    // ---- Move generation ----
    MoveList list;
    generate_moves(pos, list);
    filter_legal_moves(pos, list);

    // Seed search with TT best move if available
    if (entry != nullptr)
    {
        for (int i = 0; i < list.count; i++)
        {
            if (list.moves[i].from_square == entry->best.from_square &&
                list.moves[i].to_square == entry->best.to_square)
            {
                Move temp = list.moves[0];
                list.moves[0] = list.moves[i];
                list.moves[i] = temp;
                break;
            }
        }
    }
    order_moves(pos, list, depth);

    // ---- Terminal ----
    // No legal moves: if in check it's checkmate (score is -INF adjusted by depth
    // so shallower mates are preferred), otherwise it's stalemate (score 0).
    // Using depth in the mate score means the engine will choose the fastest mate
    // and avoid the slowest loss.
    if (list.count == 0)
        return in_check ? -INF + depth : 0;

    // ---- Normal futility pruning (setup) ----
    // At depth 1/2/3, if static eval is so far below alpha that even a good
    // quiet move is unlikely to raise it, skip quiet moves.
    // Margins: 150cp at d1, 300cp at d2, 450cp at d3.
    bool do_futility = (!in_check && !is_pv && depth <= 3);
    int futility_margin = 150 * depth;

    TTFlag tt_flag = TT_ALPHA;
    Move best_found = list.moves[0];
    int moves_searched = 0;

    //---- PVS (Principal Variation Search) ----------
    // The first move is searched with the full [alpha, beta] window. All subsequent
    // moves are first searched with a zero window (-alpha-1, -alpha) to quickly
    // test if they can beat alpha. If one does (a "re-search" is needed), we
    // re-search with the full window. This is sound because if we've ordered moves
    // well, later moves are unlikely to be best — the zero-window probe is usually
    // a cheap refutation.

    for (int i = 0; i < list.count; i++)
    {
        // Define BEFORE make_move
        bool is_capture = (piece_on(pos, list.moves[i].to_square) != NO_PIECE ||
                           list.moves[i].flag == EN_PASSANT);
        bool is_quiet = !is_capture && (list.moves[i].flag != PROMOTION);

        // Check if this is a killer move — don't reduce these
        bool is_killer = (list.moves[i].from_square == killer_moves[depth][0].from_square &&
                          list.moves[i].to_square == killer_moves[depth][0].to_square) ||
                         (list.moves[i].from_square == killer_moves[depth][1].from_square &&
                          list.moves[i].to_square == killer_moves[depth][1].to_square);

        // ---- Normal futility pruning (execution) ----
        // Skip quiet, non-killer moves that have no realistic chance of
        // raising alpha. Always search at least the first move.
        if (do_futility && moves_searched > 0 && is_quiet && !is_killer &&
            static_eval + futility_margin <= alpha)
        {
            continue;
        }

        UndoInfo undo;
        make_move(pos, list.moves[i], undo);
        stack[stack_size] = pos.hash;

        // Define AFTER make_move (need to see resulting position)
        bool gives_check = is_in_check(pos, pos.side_to_move);

        // ---- Don't futility-prune moves that give check ----
        // We already made the move above, so patch up: if it gives check and
        // we would have pruned it, un-prune by just searching it normally.
        // (The continue above happens before make_move, so this is fine.)

        int score;

        if (moves_searched == 0)
        {
            score = -alphaBeta(pos, depth - 1, -beta, -alpha,
                               stack, stack_size + 1, true);
        }
        else
        {
            // ---- Late Move Reductions (LMR) ----
            // Quiet, non-killer moves searched late (moves_searched >= 4) are
            // unlikely to be best if our move ordering is good. We reduce their
            // search depth by 1 (or 2 for very late moves) and search with a
            // zero window. If the reduced search raises alpha, we re-search at
            // full depth to confirm. Disabled in check, for killers, and for
            // captures/promotions which must be examined carefully.
            int reduction = 0;
            if (depth >= 3 && moves_searched >= 4 && is_quiet && !gives_check && !in_check && !is_killer)
            {
                reduction = 1;
                if (moves_searched >= 8)
                    reduction = 2;
            }

            score = -alphaBeta(pos, depth - 1 - reduction, -alpha - 1, -alpha,
                               stack, stack_size + 1, true);

            if (reduction > 0 && score > alpha)
            {
                score = -alphaBeta(pos, depth - 1, -alpha - 1, -alpha,
                                   stack, stack_size + 1, true);
            }

            if (score > alpha && score < beta)
            {
                score = -alphaBeta(pos, depth - 1, -beta, -alpha,
                                   stack, stack_size + 1, true);
            }
        }

        unmake_move(pos, list.moves[i], undo);
        moves_searched++;

        if (score >= beta)
        {
            // Only update killers and history for quiet moves — captures are
            // already ordered by MVV-LVA, so boosting them here would be redundant
            // and would dilute the signal for genuinely good quiet moves.
            if (is_quiet)
            {
                // Update killers
                if (depth >= 0 && depth < MAX_DEPTH)
                {
                    killer_moves[depth][1] = killer_moves[depth][0];
                    killer_moves[depth][0] = list.moves[i];
                }

                // Update history
                // Score is depth² so deeper cutoffs outweigh shallow ones —
                // a move that cuts off at depth 6 is far more valuable than
                // one that cuts off at depth 1.
                history_table[list.moves[i].from_square][list.moves[i].to_square] += depth * depth;
            }

            tt.store(hash, depth, beta, list.moves[i], TT_BETA);
            return beta;
        } // 1.14.0.7

        if (score > alpha)
        {
            alpha = score;
            tt_flag = TT_EXACT;
            best_found = list.moves[i];
        }
    }

    tt.store(hash, depth, alpha, best_found, tt_flag);
    return alpha;
}

// Top-level iterative deepening search. Searches from depth 1 up to max_depth,
// returning the best move found within the allotted time. Uses aspiration windows
// to narrow the alpha-beta window around the previous depth's score, dramatically
// cutting the number of nodes searched. If the score falls outside the window, it
// widens and re-searches. The best move from each completed depth is used to seed
// move ordering for the next depth (the "killer from previous iteration" trick).
// Also handles opening book lookup and pondering support.
Move best_move(Position &pos, int max_depth,
               uint64_t *game_history, int game_history_count,
               int remaining_ms, int increment_ms, bool skip_book)
{

    if (!skip_book && !out_of_book)
    {
        Move book_move = probe_book(pos);
        if (book_move.from_square != NO_SQUARE)
        {
            fprintf(stderr, "book move fired!\n");                       // <-- here
            ponder_move = {NO_SQUARE, NO_SQUARE, NO_PIECE_TYPE, NORMAL}; // 1.15.0.0 pondering tries to set a ponder move from the book
            return book_move;
        }
        else
            out_of_book = true;
    }
    // 1.14.1.1adebug
    auto start = std::chrono::steady_clock::now();
    auto elapsed_ms = [&]() -> int
    {
        auto now = std::chrono::steady_clock::now();
        return (int)std::chrono::duration_cast<std::chrono::milliseconds>(now - start).count();
    };

    int time_limit_ms = calculate_time(remaining_ms, increment_ms);

    uint64_t stack[1024];
    for (int i = 0; i < game_history_count; i++)
        stack[i] = game_history[i];

    stack[game_history_count] = pos.hash; // 1.14.0.10

    Move best = {NO_SQUARE, NO_SQUARE, NO_PIECE_TYPE, NORMAL};
    ponder_move = {NO_SQUARE, NO_SQUARE, NO_PIECE_TYPE, NORMAL}; // 1.15.0.0 move pondering
    int previous_score = 0;

    for (int depth = 1; depth <= max_depth; depth++)
    {
        // Don't start a new depth if we've already used 3/4 of the budget
        if (depth > 1 && elapsed_ms() > (time_limit_ms * 3 / 4))
            break;

        if (stop_search.load())
            break; // move pondering 1.15.0.0

        MoveList list;
        generate_moves(pos, list);
        filter_legal_moves(pos, list);

        // Seed move ordering with best move from previous depth
        if (best.from_square != NO_SQUARE)
        {
            for (int i = 0; i < list.count; i++)
            {
                if (list.moves[i].from_square == best.from_square &&
                    list.moves[i].to_square == best.to_square)
                {
                    Move temp = list.moves[0];
                    list.moves[0] = list.moves[i];
                    list.moves[i] = temp;
                    break;
                }
            }
        }
        order_moves(pos, list, depth);

        if (list.count == 0)
            break;

        // ASPIRATION WINDOW SETUP 1.14.0.13
        // Start with a narrow ±50cp window around the previous depth's score.
        // A narrow window makes alpha-beta much more efficient because more
        // branches can be pruned. If the true score falls outside the window
        // (a "fail-low" or "fail-high"), we widen and re-search. After 4 failed
        // widening attempts we fall back to a full-width (-INF, INF) search.
        Move depth_best = list.moves[0];
        int best_score = -INF;
        // REVERT FROM HERE FOR PVS FIX
        //  ---- Aspiration window setup ----
        int window = 50;
        int asp_alpha = (depth >= 3) ? std::max(-INF, previous_score - window) : -INF;
        int asp_beta = (depth >= 3) ? std::min(INF, previous_score + window) : INF;
        int asp_iters = 0;

        while (true)
        {
            best_score = -INF;
            depth_best = list.moves[0];

            for (int i = 0; i < list.count; i++)
            {
                // stop the check inside the move loop as well 1.15.0.0 pondering
                if (stop_search.load())
                    goto done;

                UndoInfo undo;
                make_move(pos, list.moves[i], undo);
                stack[game_history_count + 1] = pos.hash;

                int score;
                if (i == 0)
                {
                    score = -alphaBeta(pos, depth - 1, -asp_beta, -asp_alpha,
                                       stack, game_history_count + 2, true);

                    // Hyatt's fix: if the first (best) move fails low immediately,
                    // there is no point searching the remaining moves under the current
                    // window — the entire result will be a fail-low. Jump straight to
                    // window adjustment, which will widen asp_alpha and retry.
                    if (score <= asp_alpha)
                    {
                        unmake_move(pos, list.moves[i], undo);
                        best_score = score;
                        goto adjust_window;
                    }
                }
                else
                {
                    score = -alphaBeta(pos, depth - 1, -best_score - 1, -best_score,
                                       stack, game_history_count + 2, true);
                    if (score > best_score)
                    {
                        score = -alphaBeta(pos, depth - 1, -asp_beta, -best_score,
                                           stack, game_history_count + 2, true);
                    }
                }

                unmake_move(pos, list.moves[i], undo);

                if (score > best_score)
                {
                    best_score = score;
                    depth_best = list.moves[i];
                }

                if (elapsed_ms() > time_limit_ms || stop_search.load()) // 1.15.0.0 pondering
                    goto done;
            }

        adjust_window:
            if (best_score <= asp_alpha)
            {
                asp_alpha = std::max(-INF, asp_alpha - window);
                window *= 2;
            }
            else if (best_score >= asp_beta)
            {
                asp_beta = std::min(INF, asp_beta + window);
                window *= 2;
            }
            else
                break;

            asp_iters++;
            if (asp_iters >= 4)
            {
                asp_alpha = -INF;
                asp_beta = INF;
            }
        }

        best = depth_best;
        g_last_depth = depth;
        ponder_best_so_far = best;
        fprintf(stderr, "depth=%d score=%d elapsed=%dms\n",
                depth, best_score, elapsed_ms());
        fflush(stderr);

        // If we've found a forced mate, there's no point searching deeper
        // all additional depths would find the same (or longer) mate.
        if (best_score >= INF - MAX_DEPTH)
            break;
    }

done:
    return best;
}

// ============================================================
// transposition.cpp
// ============================================================

#include "transposition.h"

TranspositionTable tt;

// ============================================================
// uci.cpp
// ============================================================

#include "uci.h"
#include "fen.h"
#include "movegen.h"
#include "legal.h"
#include "search.h"
#include "eval.h"
#include "zobrist.h"
#include "transposition.h"
#include "makeunmake.h"
#include "book.h"
#include <iostream>
#include <sstream>
#include <string>
#include <thread>
#include <atomic>
#include <mutex>

// ============================================================
// PONDER STATE
// ============================================================
// Time controls saved when a "go ponder" command arrives so that when
// "ponderhit" comes in we can immediately start a real timed search
// without waiting for the GUI to resend the clock values.
int ponder_remaining_ms = 0;
int ponder_increment_ms = 0;

// Background thread that runs the ponder search. Kept as a named handle
// so stop_any_search() can join it cleanly before starting a new search.
std::thread ponder_thread;
bool is_pondering = false;

// Snapshot of the position and game history at the time pondering began.
// Copied by value so the ponder thread and UCI loop don't share mutable state.
Position ponder_pos;
uint64_t ponder_history[1024];
int ponder_history_count = 0;

// ============================================================
// UCI MOVE FORMATTING
// ============================================================
// Converts a square index (0-63) to its algebraic name (e.g. 28 -> "e4").
// Used as a building block by move_to_uci to produce the full move string.
std::string sq_to_uci(Square sq)
{
    std::string s;
    s += (char)('a' + sq % 8);
    s += (char)('1' + sq / 8);
    return s;
}

// Converts a Move to a UCI string (e.g. "e2e4", "e7e8q" for promotion).
// Promotion piece is appended as a lowercase letter; all other moves are
// simply from-square + to-square with no extra suffix.
std::string move_to_uci(const Move &move)
{
    std::string s = sq_to_uci(move.from_square) + sq_to_uci(move.to_square);
    if (move.flag == PROMOTION)
    {
        switch (move.piecetype)
        {
        case QUEEN:
            s += 'q';
            break;
        case ROOK:
            s += 'r';
            break;
        case BISHOP:
            s += 'b';
            break;
        case KNIGHT:
            s += 'n';
            break;
        default:
            break;
        }
    }
    return s;
}

// Parses a UCI move token (e.g. "e2e4") by generating all legal moves for
// the position and returning the one whose UCI string matches. Returns an
// empty move (NO_SQUARE) if no legal move matches, which signals a parse error.
Move parse_uci_move(Position &pos, const std::string &token)
{
    MoveList list;
    generate_moves(pos, list);
    filter_legal_moves(pos, list);
    for (int i = 0; i < list.count; i++)
        if (move_to_uci(list.moves[i]) == token)
            return list.moves[i];
    return {NO_SQUARE, NO_SQUARE, NO_PIECE_TYPE, NORMAL};
}

// ============================================================
// POSITION PARSING
// ============================================================
// Handles the UCI "position" command. Accepts either "startpos" or a raw FEN
// string, then replays any moves listed after the "moves" keyword so that pos
// reflects the current board state. Each resulting hash is pushed onto
// game_history for repetition detection during search.
void parse_position(Position &pos, const std::string &line,
                    uint64_t *game_history, int &game_history_count)
{
    std::istringstream ss(line);
    std::string token;
    ss >> token; // "position"

    ss >> token;
    if (token == "startpos")
    {
        pos = parse_fen(starting_fen());
        ss >> token;
    }
    else if (token == "fen")
    {
        std::string fen;
        while (ss >> token && token != "moves")
            fen += token + " ";
        pos = parse_fen(fen);
    }

    game_history_count = 0;
    game_history[game_history_count++] = pos.hash;

    if (token == "moves")
    {
        while (ss >> token)
        {
            Move move = parse_uci_move(pos, token);
            if (move.from_square == NO_SQUARE)
                break;
            UndoInfo undo;
            make_move(pos, move, undo);
            game_history[game_history_count++] = pos.hash;
        }
    }
}

// ============================================================
// PONDER MOVE EXTRACTION
// ============================================================
// After the engine picks its best move, this function advances the position
// by that move and looks up the resulting position in the transposition table
// to find the engine's predicted reply (the ponder move). Falls back to the
// first legal move if the TT has no entry, ensuring we always return something
// valid for the GUI to send back as "go ponder <move>".
static Move extract_ponder_move(Position pos, uint64_t *hist, int count, Move best)
{
    if (best.from_square == NO_SQUARE)
        return {NO_SQUARE, NO_SQUARE, NO_PIECE_TYPE, NORMAL};

    UndoInfo undo;
    make_move(pos, best, undo);

    Move pm = {NO_SQUARE, NO_SQUARE, NO_PIECE_TYPE, NORMAL};
    TTEntry *entry = tt.probe(pos.hash);
    if (entry && entry->best.from_square != NO_SQUARE)
    {
        pm = entry->best;
    }
    else
    {
        MoveList reply;
        generate_moves(pos, reply);
        filter_legal_moves(pos, reply);
        if (reply.count > 0)
            pm = reply.moves[0];
    }
    return pm;
}

// ============================================================
// SEARCH THREAD MANAGEMENT
// ============================================================
// Signals the background search thread to stop and blocks until it exits.
// Must be called before starting any new search to avoid two threads writing
// to shared state (TT, history table, ponder_best_so_far) simultaneously.
static void stop_any_search()
{
    if (ponder_thread.joinable())
    {
        stop_search = true;
        ponder_thread.join();
        stop_search = false;
    }
    is_pondering = false;
}

// ============================================================
// GO COMMAND
// ============================================================
// Handles the UCI "go" command. Parses time controls (wtime/btime/winc/binc)
// and the optional "ponder" and "movetime" flags, then either launches a
// background ponder search (returning immediately) or runs a normal timed
// search and prints "bestmove <move> ponder <move>" to stdout.
void parse_go(Position &pos, const std::string &line,
              uint64_t *game_history, int game_history_count)
{
    stop_any_search();

    std::istringstream ss(line);
    std::string token;
    ss >> token; // "go"

    bool is_ponder = false;
    int wtime = 60000, btime = 60000, winc = 0, binc = 0, movetime = -1;

    while (ss >> token)
    {
        if (token == "wtime")
            ss >> wtime;
        else if (token == "btime")
            ss >> btime;
        else if (token == "winc")
            ss >> winc;
        else if (token == "binc")
            ss >> binc;
        else if (token == "movetime")
            ss >> movetime;
        else if (token == "ponder")
            is_ponder = true;
    }

    int remaining = (pos.side_to_move == WHITE) ? wtime : btime;
    int increment = (pos.side_to_move == WHITE) ? winc : binc;

    // movetime overrides the clock — scale it up so calculate_time allocates
    // the full requested duration rather than a fraction of a fake clock.
    if (movetime > 0)
    {
        remaining = movetime * 30;
        increment = 0;
    }

    // Always save these — ponderhit needs them
    ponder_remaining_ms = remaining;
    ponder_increment_ms = increment;

    if (is_ponder)
    {
        // Save position so ponderhit can start a real search
        ponder_pos = pos;
        ponder_history_count = game_history_count;
        for (int i = 0; i < game_history_count; i++)
            ponder_history[i] = game_history[i];
        is_pondering = true;

        // Capture everything by value for the thread so the UCI loop can
        // freely modify pos and game_history without affecting the search.
        Position cap_pos = pos;
        int cap_count = game_history_count;
        uint64_t cap_hist[1024];
        for (int i = 0; i < cap_count; i++)
            cap_hist[i] = game_history[i];

        stop_search = false;
        ponder_thread = std::thread([cap_pos, cap_count, cap_hist]() mutable
                                    {
                                        // skip_book=true: book moves already played, don't re-fire
                                        best_move(cap_pos, 64, cap_hist, cap_count, 999999999, 0, true);
                                        // result discarded — we only care about TT population
                                    });
        return; // don't send bestmove yet
    }

    // Normal search — book allowed (skip_book=false)
    stop_search = false;
    Move best = best_move(pos, 64, game_history, game_history_count,
                          remaining, increment, false);

    // Safety net: verify move is legal before sending. In extremely rare cases
    // a hash collision in the TT can cause the search to return a move that
    // was legal in a different position — catching it here prevents an illegal
    // move from being sent to the GUI, which would forfeit the game.
    if (best.from_square != NO_SQUARE)
    {
        UndoInfo undo;
        make_move(pos, best, undo);
        bool left_in_check = is_in_check(pos, (pos.side_to_move == WHITE) ? BLACK : WHITE);
        unmake_move(pos, best, undo);

        if (left_in_check)
        {
            fprintf(stderr, "ILLEGAL MOVE DETECTED: %s — falling back\n",
                    move_to_uci(best).c_str());
            fflush(stderr);
            MoveList safe;
            generate_moves(pos, safe);
            filter_legal_moves(pos, safe);
            if (safe.count > 0)
                best = safe.moves[0];
        }
    }

    Move pm = extract_ponder_move(pos, game_history, game_history_count, best);
    ponder_move = pm;

    std::string bm = "bestmove " + move_to_uci(best);
    if (pm.from_square != NO_SQUARE)
        bm += " ponder " + move_to_uci(pm);

    std::cout << bm << "\n";
    std::cout.flush();
}

// ============================================================
// MAIN UCI LOOP
// ============================================================
// Reads UCI commands from stdin line by line and dispatches them to the
// appropriate handler. Blocks on std::getline so the engine is entirely
// event-driven — it only does work when the GUI sends a command.
void run_uci()
{
    Position pos = parse_fen(starting_fen());
    uint64_t game_history[1024];
    int game_history_count = 0;

    init_book(pos);

    std::string line;
    while (std::getline(std::cin, line))
    {
        // Strip Windows-style carriage returns so commands parse correctly
        // on both platforms without needing separate builds.
        if (!line.empty() && line.back() == '\r')
            line.pop_back();

        fprintf(stderr, "UCI IN: '%s'\n", line.c_str());
        fflush(stderr);

        if (line.empty())
            continue;

        if (line == "uci")
        {
            // Identify the engine and advertise supported options.
            // "Ponder type check default true" tells the GUI this engine
            // supports pondering so it will send "go ponder" commands.
            std::cout << "id name Dugong\n";
            std::cout << "id author Angad\n";
            std::cout << "option name Ponder type check default true\n";
            std::cout << "uciok\n";
            std::cout.flush();
            fprintf(stderr, "sent uciok with ponder option\n");
            fflush(stderr);
        }
        else if (line == "isready")
        {
            // Sent by the GUI to confirm the engine has finished initialising.
            // All heavy init (TT allocation, book loading) happens at startup,
            // so we can respond immediately.
            std::cout << "readyok\n";
            std::cout.flush();
        }
        else if (line == "ucinewgame")
        {
            // Reset all search state between games so history heuristics and
            // TT entries from a previous game don't influence the new one.
            stop_any_search();
            tt.clear();
            clear_killers();
            clear_history();
            out_of_book = false;
            pos = parse_fen(starting_fen());
            game_history_count = 0;
        }
        else if (line == "stop")
        {
            // If a ponderhit search is running, it must send bestmove before stopping
            // stop_any_search handles that via stop_search flag
            // stop_any_search();
            bool was_pondering = is_pondering;
            stop_any_search();

            // If we were pondering, the GUI expects a bestmove response even
            // on stop. Send the best move found so far rather than nothing,
            // falling back to "0000" (null move) if the search hadn't started.
            if (was_pondering)
            {
                std::string bm = "bestmove ";
                bm += (ponder_best_so_far.from_square != NO_SQUARE)
                          ? move_to_uci(ponder_best_so_far)
                          : "0000";
                fprintf(stderr, "stop sending: %s\n", bm.c_str());
                fflush(stderr);
                std::cout << bm << "\n";
                std::cout.flush();
                is_pondering = false;
            }
        }
        else if (line == "ponderhit")
        {
            // Opponent played our ponder move: stop infinite search,
            // run a real timed search on the same position, send bestmove.
            // We use the time controls saved during "go ponder" since the
            // GUI does not resend them with "ponderhit".
            stop_any_search();

            Position ph_pos = pos;
            int ph_count = game_history_count;
            uint64_t ph_hist[1024];
            for (int i = 0; i < ph_count; i++)
                ph_hist[i] = game_history[i];

            int ph_rem = ponder_remaining_ms;
            int ph_inc = ponder_increment_ms;

            fprintf(stderr, "ponderhit: rem=%d inc=%d\n", ph_rem, ph_inc);
            fflush(stderr);

            // Run in a thread so UCI loop stays responsive.
            // If a new go arrives before this finishes, stop_any_search()
            // will kill it cleanly and parse_go runs instead.
            stop_search = false;
            ponder_thread = std::thread([ph_pos, ph_count, ph_rem, ph_inc, ph_hist]() mutable
                                        {
                Move best = best_move(ph_pos, 64, ph_hist, ph_count,
                                      ph_rem, ph_inc, true);

                    // Same illegal-move safety net as in parse_go — TT collisions
                    // can theoretically surface here too, so we validate before sending.
                    if (best.from_square != NO_SQUARE)
                    {
                        UndoInfo undo;
                        make_move(ph_pos, best, undo);
                        bool left_in_check = is_in_check(ph_pos, (ph_pos.side_to_move == WHITE) ? BLACK : WHITE);
                        unmake_move(ph_pos, best, undo);

                        if (left_in_check)
                        {
                            fprintf(stderr, "ILLEGAL MOVE DETECTED in ponderhit: %s — falling back\n",
                            move_to_uci(best).c_str());
                            fflush(stderr);
                            MoveList safe;
                            generate_moves(ph_pos, safe);
                            filter_legal_moves(ph_pos, safe);
                            if (safe.count > 0)
                                best = safe.moves[0];
                        }
                    }

                // If we were stopped externally, don't send bestmove cause 
                // parse_go will send it from its own normal search.
                if (stop_search.load() || best.from_square == NO_SQUARE)
                {
                    fprintf(stderr, "ponderhit aborted\n");
                    fflush(stderr);
                    return;
                }

                Move pm = extract_ponder_move(ph_pos, ph_hist, ph_count, best);
                ponder_move = pm;

                std::string bm = "bestmove " + move_to_uci(best);
                if (pm.from_square != NO_SQUARE)
                    bm += " ponder " + move_to_uci(pm);

                fprintf(stderr, "ponderhit sending: %s\n", bm.c_str());
                fflush(stderr);
                std::cout << bm << "\n";
                std::cout.flush(); });
        }
        else if (line.substr(0, 8) == "position")
            parse_position(pos, line, game_history, game_history_count);
        else if (line.substr(0, 2) == "go")
            parse_go(pos, line, game_history, game_history_count);
        else if (line == "quit")
        {
            // Stop any running search before exiting so the ponder thread
            // doesn't outlive the process and cause undefined behaviour.
            stop_any_search();
            break;
        }
    }
}

// ============================================================
// zobrist.cpp
// ============================================================

#include "zobrist.h"
#include <random>

// ============================================================
// ZOBRIST HASH TABLES
// ============================================================

// One unique 64-bit key per (piece type, square) combination — XOR'd in/out
// as pieces move, allowing the hash to be updated incrementally in make_move
// rather than recomputed from scratch each time.
uint64_t piece_keys[PIECE_COUNT][64];

// XOR'd into the hash whenever Black is to move, making positions with the
// same pieces but different side-to-move distinct in the transposition table.
uint64_t side_key;

// One key per castling-rights bitmask (0–15, four bits: WK/WQ/BK/BQ).
// The old rights value is XOR'd out and the new value XOR'd in after each move.
uint64_t castling_keys[16];

// Indexed by file (0–7) rather than square, since en passant legality depends
// only on which file the capturable pawn is on, not the exact squares
uint64_t en_passant_keys[8];

// ============================================================
// INITIALISATION
// ============================================================

// Fillls all Zobrist tables with pseudo-random 64-bit values using a fixed seed.
// The seed is constant so the keys are identical across runs, which is required
// for the TT to remain valid when comparing hashes — any change to the seed
// invalidates all stored positions.
void init_zobrist()
{
    std::mt19937_64 rng(1234567890ULL);

    for (int piece = 0; piece < PIECE_COUNT; piece++)
        for (int sq = 0; sq < 64; sq++)
            piece_keys[piece][sq] = rng();

    side_key = rng();

    for (int i = 0; i < 16; i++)
        castling_keys[i] = rng();

    for (int i = 0; i < 8; i++)
        en_passant_keys[i] = rng();
}

// ============================================================
// FULL POSITION HASH
// ============================================================

// Computes the Zobrist hash from scratch by XOR-ing keys for every piece on
// the board, the side to move, castling rights, and en passant file. Only
// called when loading a position (e.g. from FEN) — during search the hash is
// maintained incrementally by make_move/unmake_move. The inner loop uses
// bb &= bb-1 (Kernighan's bit trick) to clear the lowest set bit each
// iteration, efficiently visiting every occupied square in the bitboard.
uint64_t hash_position(const Position &pos)
{
    uint64_t h = 0ULL;

    // Pieces
    for (int p = 0; p < PIECE_COUNT; p++)
    {
        Bitboard bb = pos.bitboards[p];
        while (bb)
        {
            int sq = __builtin_ctzll(bb);
            h ^= piece_keys[p][sq];
            bb &= bb - 1;
        }
    }

    // Side to move
    if (pos.side_to_move == BLACK)
        h ^= side_key;

    // Castling
    h ^= castling_keys[pos.castling_rights];

    // En Passant
    if (pos.en_passant_square != NO_SQUARE)
    {
        h ^= en_passant_keys[pos.en_passant_square % 8];
    }

    return h;
}

// ============================================================
// main.cpp
// ============================================================

#include "fen.h"
#include "display.h"
#include "movegen.h"
#include "legal.h"
#include "eval.h"
#include "search.h"
#include "zobrist.h"
#include "gui.h"
#include "transposition.h"
#include <iostream>
#include "uci.h"
#include "book.h"

// GUI MODE:
int main()
{
    init_zobrist();
    init_eval();
    tt.clear();
    clear_killers();
    clear_history();
    Position pos = parse_fen(starting_fen());
    init_book(pos);
    run_gui(pos);
    return 0;
}

// // UCI MODE:
// int main()
// {
//     init_zobrist();
//     init_eval();
//     tt.clear();
//     clear_killers();
//     clear_history();
//     run_uci();
//     return 0;
// }

// ============================================================
// END OF CONSOLIDATED PROGRAM CODE. REFER TO THE GOOGLE DRIVE FOR THE 28 WORKING FILES
// ============================================================