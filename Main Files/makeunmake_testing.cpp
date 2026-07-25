#define CATCH_CONFIG_MAIN
#include "catch_amalgamated.hpp"
#include "makeunmake.h"
#include "types.h"
#include "position.h"

// Helper: check if a piece exists on a square
bool has_piece(const Position &pos, Piece p, Square sq)
{
    return square_set(pos.bitboards[p], sq);
}

// -------------------------
// BASIC MOVE TEST
// -------------------------
TEST_CASE("Basic move make/unmake restores position", "[makeunmake]")
{
    Position pos{};
    pos.en_passant_square = NO_SQUARE;
    pos.castling_rights = 0;
    pos.halfmove_clock = 0;
    pos.fullmove_clock = 1;
    pos.side_to_move = WHITE;
    UndoInfo undo{};

    // Setup: white pawn on E2
    pos.bitboards[WP] |= square_bb(E2);
    pos.side_to_move = WHITE;

    Move move{E2, E4, PAWN};

    Position original = pos;

    make_move(pos, move, undo);
    unmake_move(pos, move, undo);

    REQUIRE(pos.bitboards[WP] == original.bitboards[WP]);
    REQUIRE(pos.side_to_move == original.side_to_move);
}

// -------------------------
// CAPTURE TEST
// -------------------------
TEST_CASE("Capture is made and unmade correctly", "[makeunmake]")
{
    Position pos{};
    UndoInfo undo{};

    pos.bitboards[WP] |= square_bb(E4);
    pos.bitboards[BP] |= square_bb(D5);
    pos.side_to_move = WHITE;

    Move move{E4, D5, PAWN};

    make_move(pos, move, undo);

    REQUIRE(has_piece(pos, WP, D5));
    REQUIRE_FALSE(has_piece(pos, BP, D5));

    unmake_move(pos, move, undo);

    REQUIRE(has_piece(pos, WP, E4));
    REQUIRE(has_piece(pos, BP, D5));
}

// -------------------------
// EN PASSANT TEST
// -------------------------
TEST_CASE("En passant works correctly", "[makeunmake]")
{
    Position pos{};
    UndoInfo undo{};

    // White pawn on E5, black pawn on D5
    pos.bitboards[WP] |= square_bb(E5);
    pos.bitboards[BP] |= square_bb(D5);
    pos.side_to_move = WHITE;

    Move move{E5, D6, PAWN, EN_PASSANT};

    make_move(pos, move, undo);

    REQUIRE(has_piece(pos, WP, D6));
    REQUIRE_FALSE(has_piece(pos, BP, D5));

    unmake_move(pos, move, undo);

    REQUIRE(has_piece(pos, WP, E5));
    REQUIRE(has_piece(pos, BP, D5));
}

// -------------------------
// CASTLING TEST
// -------------------------
TEST_CASE("Castling moves rook correctly", "[makeunmake]")
{
    Position pos{};
    UndoInfo undo{};

    pos.bitboards[WK] |= square_bb(E1);
    pos.bitboards[WR] |= square_bb(H1);
    pos.side_to_move = WHITE;

    Move move{E1, G1, KING, CASTLING};

    make_move(pos, move, undo);

    REQUIRE(has_piece(pos, WK, G1));
    REQUIRE(has_piece(pos, WR, F1));

    unmake_move(pos, move, undo);

    REQUIRE(has_piece(pos, WK, E1));
    REQUIRE(has_piece(pos, WR, H1));
}

// -------------------------
// PROMOTION TEST
// -------------------------
TEST_CASE("Promotion works correctly", "[makeunmake]")
{
    Position pos{};
    UndoInfo undo{};

    pos.bitboards[WP] |= square_bb(E7);
    pos.side_to_move = WHITE;

    Move move{E7, E8, PAWN, PROMOTION};

    make_move(pos, move, undo);

    REQUIRE(has_piece(pos, WQ, E8));
    REQUIRE_FALSE(has_piece(pos, WP, E8));

    unmake_move(pos, move, undo);

    REQUIRE(has_piece(pos, WP, E7));
}

// -------------------------
// EN PASSANT SQUARE TEST
// -------------------------
TEST_CASE("Double pawn push sets en passant square", "[makeunmake]")
{
    Position pos{};
    UndoInfo undo{};

    pos.bitboards[WP] |= square_bb(E2);
    pos.side_to_move = WHITE;

    Move move{E2, E4, PAWN};

    make_move(pos, move, undo);

    REQUIRE(pos.en_passant_square == E3);
}

// -------------------------
// CASTLING RIGHTS TEST
// -------------------------
TEST_CASE("Moving king removes castling rights", "[makeunmake]")
{
    Position pos{};
    UndoInfo undo{};

    pos.bitboards[WK] |= square_bb(E1);
    pos.castling_rights = WHITE_00 | WHITE_000;

    Move move{E1, E2, KING};

    make_move(pos, move, undo);

    REQUIRE((pos.castling_rights & (WHITE_00 | WHITE_000)) == 0);
}

// -------------------------
// FULL REVERSIBILITY TEST
// -------------------------
TEST_CASE("Position fully restored after make/unmake", "[makeunmake]")
{
    Position pos{};
    UndoInfo undo{};

    pos.bitboards[WP] |= square_bb(E2);
    pos.bitboards[BP] |= square_bb(E7);
    pos.side_to_move = WHITE;
    pos.castling_rights = WHITE_00 | WHITE_000;
    pos.en_passant_square = NO_SQUARE;

    Position original = pos;

    Move move{E2, E4, PAWN};

    make_move(pos, move, undo);
    unmake_move(pos, move, undo);

    REQUIRE(pos.bitboards[WP] == original.bitboards[WP]);
    REQUIRE(pos.bitboards[BP] == original.bitboards[BP]);
    REQUIRE(pos.side_to_move == original.side_to_move);
    REQUIRE(pos.castling_rights == original.castling_rights);
    REQUIRE(pos.en_passant_square == original.en_passant_square);
}

TEST_CASE("Sequence of moves fully reversible", "[makeunmake][sequence]")
{
    Position pos{};
    UndoInfo u1{}, u2{}, u3{};

    pos.bitboards[WP] |= square_bb(E2);
    pos.bitboards[BP] |= square_bb(E7);
    pos.side_to_move = WHITE;

    Position original = pos;

    Move m1{E2, E4, PAWN};
    Move m2{E7, E5, PAWN};
    Move m3{E4, E5, PAWN}; // capture

    make_move(pos, m1, u1);
    make_move(pos, m2, u2);
    make_move(pos, m3, u3);

    unmake_move(pos, m3, u3);
    unmake_move(pos, m2, u2);
    unmake_move(pos, m1, u1);

    REQUIRE(pos.bitboards[WP] == original.bitboards[WP]);
    REQUIRE(pos.bitboards[BP] == original.bitboards[BP]);
    REQUIRE(pos.side_to_move == original.side_to_move);
}

TEST_CASE("En passant square clears after non-pawn move", "[makeunmake][ep]")
{
    Position pos{};
    UndoInfo u1{}, u2{};

    pos.bitboards[WP] |= square_bb(E2);
    pos.bitboards[WN] |= square_bb(G1);
    pos.side_to_move = WHITE;

    Move pawn_move{E2, E4, PAWN};
    make_move(pos, pawn_move, u1);

    REQUIRE(pos.en_passant_square == E3);

    Move knight_move{G1, F3, KNIGHT};
    make_move(pos, knight_move, u2);

    REQUIRE(pos.en_passant_square == NO_SQUARE);
}

TEST_CASE("Castling rights removed when rook is captured", "[makeunmake][castling]")
{
    Position pos{};
    UndoInfo undo{};

    pos.bitboards[WR] |= square_bb(A1);
    pos.bitboards[BP] |= square_bb(A2);
    pos.castling_rights = WHITE_000;

    Move move{A2, A1, PAWN}; // capture rook
    pos.side_to_move = BLACK;

    make_move(pos, move, undo);

    REQUIRE((pos.castling_rights & WHITE_000) == 0);
}

TEST_CASE("Halfmove clock resets on capture", "[makeunmake][clock]")
{
    Position pos{};
    UndoInfo undo{};

    pos.bitboards[WP] |= square_bb(E4);
    pos.bitboards[BP] |= square_bb(D5);
    pos.halfmove_clock = 10;
    pos.side_to_move = WHITE;

    Move move{E4, D5, PAWN};

    make_move(pos, move, undo);

    REQUIRE(pos.halfmove_clock == 0);
}

TEST_CASE("Fullmove clock increments after black move", "[makeunmake][clock]")
{
    Position pos{};
    UndoInfo undo{};

    pos.bitboards[BP] |= square_bb(E7);
    pos.side_to_move = BLACK;
    pos.fullmove_clock = 5;

    Move move{E7, E6, PAWN};

    make_move(pos, move, undo);

    REQUIRE(pos.fullmove_clock == 6);
}

TEST_CASE("Castling rights restored after undo", "[makeunmake][castling]")
{
    Position pos{};
    UndoInfo undo{};

    pos.bitboards[WK] |= square_bb(E1);
    pos.castling_rights = WHITE_00 | WHITE_000;
    pos.side_to_move = WHITE;

    Move move{E1, E2, KING};

    make_move(pos, move, undo);
    unmake_move(pos, move, undo);

    REQUIRE(pos.castling_rights == (WHITE_00 | WHITE_000));
}

TEST_CASE("Promotion with capture works", "[makeunmake][promotion]")
{
    Position pos{};
    UndoInfo undo{};

    pos.bitboards[WP] |= square_bb(E7);
    pos.bitboards[BR] |= square_bb(E8);
    pos.side_to_move = WHITE;

    Move move{E7, E8, PAWN, PROMOTION};

    make_move(pos, move, undo);

    REQUIRE(square_set(pos.bitboards[WQ], E8));
    REQUIRE_FALSE(square_set(pos.bitboards[BR], E8));

    unmake_move(pos, move, undo);

    REQUIRE(square_set(pos.bitboards[WP], E7));
    REQUIRE(square_set(pos.bitboards[BR], E8));
}

TEST_CASE("Long sequence fully reversible", "[makeunmake][stress]")
{
    Position pos{};
    UndoInfo u[6];

    pos.bitboards[WP] |= square_bb(E2);
    pos.bitboards[BP] |= square_bb(D7);
    pos.bitboards[WN] |= square_bb(G1);
    pos.bitboards[BN] |= square_bb(B8);
    pos.side_to_move = WHITE;

    Position original = pos;

    Move moves[] = {
        {E2, E4, PAWN},
        {D7, D5, PAWN},
        {G1, F3, KNIGHT},
        {B8, C6, KNIGHT},
        {E4, D5, PAWN}, // capture
        {C6, D4, KNIGHT}};

    for (int i = 0; i < 6; i++)
        make_move(pos, moves[i], u[i]);

    for (int i = 5; i >= 0; i--)
        unmake_move(pos, moves[i], u[i]);

    REQUIRE(pos.bitboards[WP] == original.bitboards[WP]);
    REQUIRE(pos.bitboards[BP] == original.bitboards[BP]);
    REQUIRE(pos.bitboards[WN] == original.bitboards[WN]);
    REQUIRE(pos.bitboards[BN] == original.bitboards[BN]);
    REQUIRE(pos.side_to_move == original.side_to_move);
}

TEST_CASE("En passant capture and undo restores exact state", "[makeunmake][ep]")
{
    Position pos{};
    pos.en_passant_square = NO_SQUARE;
    pos.castling_rights = 0;
    pos.halfmove_clock = 0;
    pos.fullmove_clock = 1;
    pos.side_to_move = WHITE;
    UndoInfo u1{}, u2{};

    pos.bitboards[WP] |= square_bb(E5);
    pos.bitboards[BP] |= square_bb(D7);
    pos.side_to_move = BLACK;

    Move m1{D7, D5, PAWN};
    make_move(pos, m1, u1);

    REQUIRE(pos.en_passant_square == D6);

    Move m2{E5, D6, PAWN, EN_PASSANT};
    make_move(pos, m2, u2);

    REQUIRE_FALSE(square_set(pos.bitboards[BP], D5));

    unmake_move(pos, m2, u2);
    unmake_move(pos, m1, u1);

    REQUIRE(square_set(pos.bitboards[WP], E5));
    REQUIRE(square_set(pos.bitboards[BP], D7));
    REQUIRE(pos.en_passant_square == NO_SQUARE);
}

TEST_CASE("Castling rights affected by rook capture and restored on undo", "[makeunmake][castling]")
{
    Position pos{};
    UndoInfo u1{}, u2{};

    pos.bitboards[WR] |= square_bb(H1);
    pos.bitboards[BP] |= square_bb(H2);
    pos.castling_rights = WHITE_00;
    pos.side_to_move = BLACK;

    Move capture{H2, H1, PAWN};
    make_move(pos, capture, u1);

    REQUIRE((pos.castling_rights & WHITE_00) == 0);

    unmake_move(pos, capture, u1);

    REQUIRE(pos.castling_rights == WHITE_00);
    REQUIRE(square_set(pos.bitboards[WR], H1));
}

TEST_CASE("Promotion followed by further moves and undo", "[makeunmake][promotion]")
{
    Position pos{};
    UndoInfo u1{}, u2{};

    pos.bitboards[WP] |= square_bb(E7);
    pos.side_to_move = WHITE;

    Move promote{E7, E8, PAWN, PROMOTION};
    make_move(pos, promote, u1);

    REQUIRE(square_set(pos.bitboards[WQ], E8));

    Move queen_move{E8, H8, QUEEN};
    make_move(pos, queen_move, u2);

    REQUIRE(square_set(pos.bitboards[WQ], H8));

    unmake_move(pos, queen_move, u2);
    unmake_move(pos, promote, u1);

    REQUIRE(square_set(pos.bitboards[WP], E7));
}

TEST_CASE("Multiple captures in sequence handled correctly", "[makeunmake][capture]")
{
    Position pos{};
    UndoInfo u1{}, u2{};

    pos.bitboards[WP] |= square_bb(E4);
    pos.bitboards[BP] |= square_bb(D5);
    pos.bitboards[BN] |= square_bb(C6);
    pos.side_to_move = WHITE;

    Move m1{E4, D5, PAWN}; // capture pawn
    make_move(pos, m1, u1);

    Move m2{C6, D4, KNIGHT};
    make_move(pos, m2, u2);

    unmake_move(pos, m2, u2);
    unmake_move(pos, m1, u1);

    REQUIRE(square_set(pos.bitboards[WP], E4));
    REQUIRE(square_set(pos.bitboards[BP], D5));
    REQUIRE(square_set(pos.bitboards[BN], C6));
}

TEST_CASE("Full state restored after complex move", "[makeunmake][state]")
{
    Position pos{};
    UndoInfo undo{};

    pos.bitboards[WP] |= square_bb(E2);
    pos.bitboards[BP] |= square_bb(E7);
    pos.castling_rights = WHITE_00 | BLACK_00;
    pos.en_passant_square = NO_SQUARE;
    pos.halfmove_clock = 7;
    pos.fullmove_clock = 3;
    pos.side_to_move = WHITE;

    Position original = pos;

    Move move{E2, E4, PAWN};
    make_move(pos, move, undo);
    unmake_move(pos, move, undo);

    REQUIRE(pos.bitboards[WP] == original.bitboards[WP]);
    REQUIRE(pos.castling_rights == original.castling_rights);
    REQUIRE(pos.en_passant_square == original.en_passant_square);
    REQUIRE(pos.halfmove_clock == original.halfmove_clock);
    REQUIRE(pos.fullmove_clock == original.fullmove_clock);
    REQUIRE(pos.side_to_move == original.side_to_move);
}
