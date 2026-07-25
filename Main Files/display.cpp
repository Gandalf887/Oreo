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