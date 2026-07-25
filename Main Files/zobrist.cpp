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
    for (int p = 0; p < PIECE_COUNT; p++) {
        Bitboard bb = pos.bitboards[p];
        while (bb) {
            int sq = __builtin_ctzll(bb);
            h ^= piece_keys[p][sq];
            bb &= bb - 1;
        }
    }

    // Side to move
    if (pos.side_to_move == BLACK) h ^= side_key;

    // Castling
    h ^= castling_keys[pos.castling_rights];

    // En Passant
    if (pos.en_passant_square != NO_SQUARE) {
        h ^= en_passant_keys[pos.en_passant_square % 8];
    }

    return h;
}