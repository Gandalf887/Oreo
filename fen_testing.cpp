#include "catch_amalgamated.hpp"
#include "fen.h"
#include "types.h"

TEST_CASE("starting_fen returns correct string", "[fen]")
{
    REQUIRE(starting_fen() ==
            "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1");
}

TEST_CASE("parse starting position - basic fields", "[fen]")
{
    Position pos = parse_fen(starting_fen());

    REQUIRE(pos.side_to_move == WHITE);
    REQUIRE(pos.castling_rights == (WHITE_00 | WHITE_000 | BLACK_00 | BLACK_000));
    REQUIRE(pos.en_passant_square == NO_SQUARE);
}

TEST_CASE("parse starting position - piece placement", "[fen]")
{
    Position pos = parse_fen(starting_fen());

    // Example: check white king on e1
    REQUIRE(pos.bitboards[WK] & square_bb(E1));

    // Example: check black queen on d8
    REQUIRE(pos.bitboards[BQ] & square_bb(D8));

    // Pawns
    REQUIRE(pos.bitboards[WP] & square_bb(A2));
    REQUIRE(pos.bitboards[BP] & square_bb(H7));
}

TEST_CASE("parse empty board", "[fen]")
{
    Position pos = parse_fen("8/8/8/8/8/8/8/8 w - - 0 1");

    for (int i = 0; i < PIECE_COUNT; i++)
    {
        REQUIRE(pos.bitboards[i] == 0);
    }
}

TEST_CASE("side to move parsing", "[fen]")
{
    Position pos = parse_fen("8/8/8/8/8/8/8/8 b - - 0 1");

    REQUIRE(pos.side_to_move == BLACK);
}

TEST_CASE("castling rights parsing", "[fen]")
{
    Position pos = parse_fen("8/8/8/8/8/8/8/8 w Kq - 0 1");

    REQUIRE(pos.castling_rights & WHITE_00);
    REQUIRE(pos.castling_rights & BLACK_000);
}

TEST_CASE("en passant parsing", "[fen]")
{
    Position pos = parse_fen("8/8/8/8/8/8/8/8 w - e3 0 1");

    REQUIRE(pos.en_passant_square == E3);
}

TEST_CASE("invalid piece character throws", "[fen]")
{
    REQUIRE_THROWS(parse_fen("8/8/8/8/8/8/8/8X w - - 0 1"));
}

TEST_CASE("halfmove and fullmove parsing", "[fen]")
{
    Position pos = parse_fen("8/8/8/8/8/8/8/8 w - - 5 10");

    REQUIRE(pos.halfmove_clock == 5);
    REQUIRE(pos.fullmove_clock == 10);
}