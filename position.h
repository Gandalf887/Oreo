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
    pos.hash = 0ULL; //1.13.0
    return pos;
}
