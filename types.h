#pragma once
#include <cstdint>
#include <string>

using Bitboard = uint64_t;

enum Square {
    A1, B1, C1, D1, E1, F1, G1, H1,
    A2, B2, C2, D2, E2, F2, G2, H2,
    A3, B3, C3, D3, E3, F3, G3, H3,
    A4, B4, C4, D4, E4, F4, G4, H4,
    A5, B5, C5, D5, E5, F5, G5, H5,
    A6, B6, C6, D6, E6, F6, G6, H6,
    A7, B7, C7, D7, E7, F7, G7, H7,
    A8, B8, C8, D8, E8, F8, G8, H8,
    NO_SQUARE = -1
};

enum PieceType { PAWN, KNIGHT, BISHOP, ROOK, QUEEN, KING, NO_PIECE_TYPE };

enum Colour { WHITE, BLACK, NO_COLOUR };

enum Piece 
{
    WP, WN, WB, WR, WQ, WK,
    BP, BN, BB, BR, BQ, BK,
    NO_PIECE,
    PIECE_COUNT = 12
};

enum CastlingRights 
{
    NO_CASTLING = 0,
    WHITE_00 = 1, //WHITE <KING>SIDE
    WHITE_000 = 2, //WHITE <QUEEN>SIDE
    BLACK_00 = 4, //BLACK <KING>SIDE
    BLACK_000 = 8, //BLACK <QUEEN>SIDE
    ALL_CASTLING = 15  
};

inline Bitboard square_bb(Square s) 
{
    return 1ULL << s; 
}

inline bool square_set(Bitboard b, Square s) 
{ 
    return (b>>s) & 1ULL; 
}