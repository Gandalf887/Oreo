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
            case 'P': piece = WP; break;
            case 'N': piece = WN; break;
            case 'B': piece = WB; break;
            case 'R': piece = WR; break;
            case 'Q': piece = WQ; break;
            case 'K': piece = WK; break;
            case 'p': piece = BP; break;
            case 'n': piece = BN; break;
            case 'b': piece = BB; break;
            case 'r': piece = BR; break;
            case 'q': piece = BQ; break;
            case 'k': piece = BK; break;
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
            case 'K': pos.castling_rights |= WHITE_00;  break;
            case 'Q': pos.castling_rights |= WHITE_000; break;
            case 'k': pos.castling_rights |= BLACK_00;  break;
            case 'q': pos.castling_rights |= BLACK_000; break;
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