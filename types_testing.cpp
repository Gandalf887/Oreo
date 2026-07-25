#include "catch_amalgamated.hpp"
#include "types.h"
#include "position.h"

TEST_CASE("sqaure_bb sets the correct bitl", "[bitboard]")
{
    REQUIRE(square_bb(A1) == 1ULL << 0);
    REQUIRE(square_bb(B1) == 1ULL << 1);
    REQUIRE(square_bb(H1) == 1ULL << 7);
    REQUIRE(square_bb(A2) == 1ULL << 8);
    REQUIRE(square_bb(H8) == 1ULL << 63);
}

TEST_CASE("empty_position working and initialises", "[position]")
{
    Position pos = empty_position();
    for (int i = 0; i < PIECE_COUNT; i++)
    {
        REQUIRE(pos.bitboards[i] == 0ULL);
    }

    REQUIRE(pos.side_to_move == WHITE);
    REQUIRE(pos.castling_rights == NO_CASTLING);
    REQUIRE(pos.en_passant_square == NO_SQUARE);
    REQUIRE(pos.halfmove_clock == 0);
    REQUIRE(pos.fullmove_clock == 1);
}

TEST_CASE("white pieces returns correctly", "[bitboards]")
{
    Position pos = empty_position();

    pos.bitboards[WP] = Square(0);
    pos.bitboards[WN] = Square(1);
    pos.bitboards[WB] = Square(2);
    pos.bitboards[WR] = Square(3);
    pos.bitboards[WQ] = Square(4);
    pos.bitboards[WK] = Square(5);

    Bitboard expected = 0;
    for (int i = 0; i <= 5; i++)
        expected |= Square(i);

    REQUIRE(white_pieces(pos) == expected);
}

TEST_CASE("black pieces return correctly", "[bitboards]")
{
    Position pos = empty_position();

    pos.bitboards[BP] = Square(0);
    pos.bitboards[BN] = Square(1);
    pos.bitboards[BB] = Square(2);
    pos.bitboards[BR] = Square(3);
    pos.bitboards[BQ] = Square(4);
    pos.bitboards[BK] = Square(5);

    Bitboard expected = 0;
    for (int i = 0; i <= 5; i++)
        expected |= Square(i);
    REQUIRE(black_pieces(pos) == expected);
}
