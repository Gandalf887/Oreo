#define CATCH_CONFIG_MAIN
#include "catch_amalgamated.hpp"
#include "movegen.h"
#include "position.h"

// ---------------- HELPERS ----------------

bool move_exists(const MoveList &list, Square from, Square to)
{
    for (int i = 0; i < list.count; i++)
    {
        if (list.moves[i].from_square == from &&
            list.moves[i].to_square == to)
            return true;
    }
    return false;
}

// ---------------- KING ----------------

TEST_CASE("King moves from center", "[king]")
{
    Position pos = empty_position();
    pos.bitboards[WK] = 1ULL << E4;

    MoveList list{};
    generate_king_moves(pos, list);

    REQUIRE(list.count == 8);
}

TEST_CASE("King cannot move onto friendly piece", "[king]")
{
    Position pos = empty_position();
    pos.bitboards[WK] = 1ULL << E4;
    pos.bitboards[WP] = 1ULL << E5;

    MoveList list{};
    generate_king_moves(pos, list);

    REQUIRE_FALSE(move_exists(list, E4, E5));
}

// ---------------- KNIGHT ----------------

TEST_CASE("Knight moves from center", "[knight]")
{
    Position pos = empty_position();
    pos.bitboards[WN] = 1ULL << E4;

    MoveList list{};
    generate_knight_moves(pos, list);

    REQUIRE(list.count == 8);
}

TEST_CASE("Knight blocked by friendly", "[knight]")
{
    Position pos = empty_position();
    pos.bitboards[WN] = 1ULL << E4;
    pos.bitboards[WP] = 1ULL << F6;

    MoveList list{};
    generate_knight_moves(pos, list);

    REQUIRE_FALSE(move_exists(list, E4, F6));
}

// ---------------- ROOK ----------------

TEST_CASE("Rook moves empty board", "[rook]")
{
    Position pos = empty_position();
    pos.bitboards[WR] = 1ULL << D4;

    MoveList list{};
    generate_sliding_moves(pos, list);

    REQUIRE(list.count == 14);
}

TEST_CASE("Rook blocked correctly", "[rook]")
{
    Position pos = empty_position();
    pos.bitboards[WR] = 1ULL << D4;
    pos.bitboards[WP] = 1ULL << D6;

    MoveList list{};
    generate_sliding_moves(pos, list);

    REQUIRE(move_exists(list, D4, D5));
    REQUIRE_FALSE(move_exists(list, D4, D6));
    REQUIRE_FALSE(move_exists(list, D4, D7));
}

// ---------------- BISHOP ----------------

TEST_CASE("Bishop moves empty board", "[bishop]")
{
    Position pos = empty_position();
    pos.bitboards[WB] = 1ULL << D4;

    MoveList list{};
    generate_sliding_moves(pos, list);

    REQUIRE(list.count == 13);
}

// ---------------- PAWNS ----------------

TEST_CASE("Pawn single push", "[pawn]")
{
    Position pos = empty_position();
    pos.bitboards[WP] = 1ULL << E2;

    MoveList list{};
    generate_pawn_moves(pos, list);

    REQUIRE(move_exists(list, E2, E3));
}

TEST_CASE("Pawn double push", "[pawn]")
{
    Position pos = empty_position();
    pos.bitboards[WP] = 1ULL << E2;

    MoveList list{};
    generate_pawn_moves(pos, list);

    REQUIRE(move_exists(list, E2, E4));
}

TEST_CASE("Pawn capture", "[pawn]")
{
    Position pos = empty_position();
    pos.bitboards[WP] = 1ULL << E4;
    pos.bitboards[BP] = 1ULL << F5;

    MoveList list{};
    generate_pawn_moves(pos, list);

    REQUIRE(move_exists(list, E4, F5));
}

TEST_CASE("Pawn promotion", "[pawn]")
{
    Position pos = empty_position();
    pos.bitboards[WP] = 1ULL << E7;

    MoveList list{};
    generate_pawn_moves(pos, list);

    bool found = false;

    for (int i = 0; i < list.count; i++)
    {
        if (list.moves[i].to_square == E8 &&
            list.moves[i].flag == PROMOTION)
        {
            found = true;
        }
    }

    REQUIRE(found);
}

// ---------------- INTEGRATION ----------------

TEST_CASE("Generate moves sanity", "[integration]")
{
    Position pos = empty_position();
    pos.bitboards[WK] = 1ULL << E1;
    pos.bitboards[WN] = 1ULL << G1;
    pos.bitboards[WP] = 1ULL << E2;

    MoveList list{};
    generate_moves(pos, list);

    REQUIRE(list.count > 0);
}