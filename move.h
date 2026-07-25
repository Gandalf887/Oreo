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
    //check piece captured

    Colour side_to_move;

    int castling_rights;

    Square en_passant_square;

    int halfmove_clock;

    int fullmove_clock;

    uint64_t hash;
};

