#define CATCH_CONFIG_MAIN
#include "catch_amalgamated.hpp"

#include "types.h"
#include "position.h"
#include "move.h"
#include "makeunmake.h"
#include "movegen.h"
#include "legal.h"

// ---------------------------------------------------------------------------
// Small helper – build a Move struct cleanly
// ---------------------------------------------------------------------------
static Move make_move_struct(Square from, Square to, PieceType pt, MoveFlag flag = NORMAL)
{
    Move m;
    m.from_square = from;
    m.to_square = to;
    m.piecetype = pt;
    m.flag = flag;
    return m;
}

// ---------------------------------------------------------------------------
// is_square_attacked – knights
// ---------------------------------------------------------------------------

TEST_CASE("Knight on D2 attacks E4", "[knight][attack]")
{
    Position pos = empty_position();
    pos.bitboards[WN] |= square_bb(D2);
    REQUIRE(is_square_attacked(pos, E4, WHITE));
}

TEST_CASE("Knight on D2 does not attack D4 (straight)", "[knight][attack]")
{
    Position pos = empty_position();
    pos.bitboards[WN] |= square_bb(D2);
    REQUIRE_FALSE(is_square_attacked(pos, D4, WHITE));
}

TEST_CASE("King on H1 does not wrap east to A2", "[king][wrap]")
{
    Position pos = empty_position();
    pos.bitboards[WK] |= square_bb(H1);

    Bitboard sq_bb = square_bb(A2);
    Bitboard wk = pos.bitboards[WK];
    printf("WK on H1 = %llu\n", wk);
    printf("A2 sq_bb = %llu\n", sq_bb);
    printf("A2 <<1 masked = %llu\n", (sq_bb << 1) & ~0x8080808080808080ULL);
    printf("A2 >>1 masked = %llu\n", (sq_bb >> 1) & ~0x0101010101010101ULL);
    printf("A2 <<7 masked = %llu\n", (sq_bb << 7) & ~0x0101010101010101ULL);
    printf("A2 >>7 masked = %llu\n", (sq_bb >> 7) & ~0x8080808080808080ULL);
    printf("A2 <<8       = %llu\n", (sq_bb << 8));
    printf("A2 >>8       = %llu\n", (sq_bb >> 8));
    printf("A2 <<9 masked = %llu\n", (sq_bb << 9) & ~0x8080808080808080ULL);
    printf("A2 >>9 masked = %llu\n", (sq_bb >> 9) & ~0x0101010101010101ULL);
    printf("WK & any of above?\n");

    REQUIRE_FALSE(is_square_attacked(pos, A2, WHITE));
}

TEST_CASE("King on H4 does not wrap east to A5", "[king][wrap]")
{
    Position pos = empty_position();
    pos.bitboards[WK] |= square_bb(H4);
    REQUIRE_FALSE(is_square_attacked(pos, A5, WHITE)); // <<1 wrap
    REQUIRE_FALSE(is_square_attacked(pos, A4, WHITE)); // <<9 NE wrap
}

TEST_CASE("King on A4 does not wrap west to H3", "[king][wrap]")
{
    Position pos = empty_position();
    pos.bitboards[WK] |= square_bb(A4);
    REQUIRE_FALSE(is_square_attacked(pos, H3, WHITE)); // >>1 wrap
    REQUIRE_FALSE(is_square_attacked(pos, H4, WHITE)); // >>9 SW wrap
}
TEST_CASE("Knight on B1 attacks A3 and C3", "[knight][attack]")
{
    Position pos = empty_position();
    pos.bitboards[WN] |= square_bb(B1);
    REQUIRE(is_square_attacked(pos, A3, WHITE));
    REQUIRE(is_square_attacked(pos, C3, WHITE));
}

// ---------------------------------------------------------------------------
// is_square_attacked – king
// ---------------------------------------------------------------------------
TEST_CASE("King on E1 attacks all adjacent squares", "[king][attack]")
{
    Position pos = empty_position();
    pos.bitboards[WK] |= square_bb(E1);
    REQUIRE(is_square_attacked(pos, D1, WHITE));
    REQUIRE(is_square_attacked(pos, F1, WHITE));
    REQUIRE(is_square_attacked(pos, D2, WHITE));
    REQUIRE(is_square_attacked(pos, E2, WHITE));
    REQUIRE(is_square_attacked(pos, F2, WHITE));
}

TEST_CASE("King on A1 does not wrap to H-file", "[king][wrap]")
{
    Position pos = empty_position();
    pos.bitboards[WK] |= square_bb(A1);
    REQUIRE_FALSE(is_square_attacked(pos, H1, WHITE));
    REQUIRE_FALSE(is_square_attacked(pos, H2, WHITE));
}

TEST_CASE("King on H8 does not wrap to A-file", "[king][wrap]")
{
    Position pos = empty_position();
    pos.bitboards[BK] |= square_bb(H8);
    REQUIRE_FALSE(is_square_attacked(pos, A8, BLACK));
    REQUIRE_FALSE(is_square_attacked(pos, A7, BLACK));
}

// ---------------------------------------------------------------------------
// is_square_attacked – pawns
// ---------------------------------------------------------------------------
TEST_CASE("White pawn on E2 attacks D3 and F3", "[pawn][attack]")
{
    Position pos = empty_position();
    pos.bitboards[WP] |= square_bb(E2);
    REQUIRE(is_square_attacked(pos, D3, WHITE));
    REQUIRE(is_square_attacked(pos, F3, WHITE));
}

TEST_CASE("White pawn on E2 does not attack E3 (straight push)", "[pawn][attack]")
{
    Position pos = empty_position();
    pos.bitboards[WP] |= square_bb(E2);
    REQUIRE_FALSE(is_square_attacked(pos, E3, WHITE));
}

TEST_CASE("Black pawn on E7 attacks D6 and F6", "[pawn][attack]")
{
    Position pos = empty_position();
    pos.bitboards[BP] |= square_bb(E7);
    REQUIRE(is_square_attacked(pos, D6, BLACK));
    REQUIRE(is_square_attacked(pos, F6, BLACK));
}

TEST_CASE("Black pawn on E7 does not attack E6 (straight push)", "[pawn][attack]")
{
    Position pos = empty_position();
    pos.bitboards[BP] |= square_bb(E7);
    REQUIRE_FALSE(is_square_attacked(pos, E6, BLACK));
}

TEST_CASE("White pawn on A4 does not wrap to H5", "[pawn][wrap]")
{
    Position pos = empty_position();
    pos.bitboards[WP] |= square_bb(A4);
    REQUIRE_FALSE(is_square_attacked(pos, H5, WHITE));
}

TEST_CASE("Black pawn on H5 does not wrap to A4", "[pawn][wrap]")
{
    Position pos = empty_position();
    pos.bitboards[BP] |= square_bb(H5);
    REQUIRE_FALSE(is_square_attacked(pos, A4, BLACK));
}

// ---------------------------------------------------------------------------
// is_square_attacked – rook (sliding, blocking)
// ---------------------------------------------------------------------------
TEST_CASE("White rook on A1 attacks A8 on open file", "[rook][sliding]")
{
    Position pos = empty_position();
    pos.bitboards[WR] |= square_bb(A1);
    REQUIRE(is_square_attacked(pos, A8, WHITE));
}

TEST_CASE("White rook on A1 attacks H1 on open rank", "[rook][sliding]")
{
    Position pos = empty_position();
    pos.bitboards[WR] |= square_bb(A1);
    REQUIRE(is_square_attacked(pos, H1, WHITE));
}

TEST_CASE("White rook on A1 blocked by pawn on A4 – A8 not attacked", "[rook][blocking]")
{
    Position pos = empty_position();
    pos.bitboards[WR] |= square_bb(A1);
    pos.bitboards[WP] |= square_bb(A4);
    REQUIRE_FALSE(is_square_attacked(pos, A8, WHITE));
}

TEST_CASE("White rook on A1 blocked by pawn on A4 – A4 itself IS attacked", "[rook][blocking]")
{
    Position pos = empty_position();
    pos.bitboards[WR] |= square_bb(A1);
    pos.bitboards[WP] |= square_bb(A4);
    REQUIRE(is_square_attacked(pos, A4, WHITE));
}

// ---------------------------------------------------------------------------
// is_square_attacked – bishop / queen
// ---------------------------------------------------------------------------
TEST_CASE("White bishop on C1 attacks F4 on open diagonal", "[bishop][sliding]")
{
    Position pos = empty_position();
    pos.bitboards[WB] |= square_bb(C1);
    REQUIRE(is_square_attacked(pos, F4, WHITE));
}

TEST_CASE("White bishop on C1 blocked by pawn on E3 – G5 not attacked", "[bishop][blocking]")
{
    Position pos = empty_position();
    pos.bitboards[WB] |= square_bb(C1);
    pos.bitboards[WP] |= square_bb(E3);
    REQUIRE_FALSE(is_square_attacked(pos, G5, WHITE));
}

TEST_CASE("White queen on D4 attacks along rank, file, and both diagonals", "[queen]")
{
    Position pos = empty_position();
    pos.bitboards[WQ] |= square_bb(D4);
    REQUIRE(is_square_attacked(pos, D8, WHITE)); // file north
    REQUIRE(is_square_attacked(pos, H4, WHITE)); // rank east
    REQUIRE(is_square_attacked(pos, G7, WHITE)); // diagonal NE
    REQUIRE(is_square_attacked(pos, A1, WHITE)); // diagonal SW
}

// ---------------------------------------------------------------------------
// is_in_check
// ---------------------------------------------------------------------------
TEST_CASE("White king in check from black rook on same file", "[check]")
{
    Position pos = empty_position();
    pos.bitboards[WK] |= square_bb(E1);
    pos.bitboards[BK] |= square_bb(A8);
    pos.bitboards[BR] |= square_bb(E8);
    REQUIRE(is_in_check(pos, WHITE));
}

TEST_CASE("White king NOT in check when rook is blocked by own pawn", "[check]")
{
    Position pos = empty_position();
    pos.bitboards[WK] |= square_bb(E1);
    pos.bitboards[WP] |= square_bb(E2);
    pos.bitboards[BK] |= square_bb(A8);
    pos.bitboards[BR] |= square_bb(E8);
    REQUIRE_FALSE(is_in_check(pos, WHITE));
}

TEST_CASE("Black king in check from white knight on D6", "[check][knight]")
{
    Position pos = empty_position();
    // D6=43, E8=60 – difference is 17, valid knight move
    pos.bitboards[BK] |= square_bb(E8);
    pos.bitboards[WK] |= square_bb(A1);
    pos.bitboards[WN] |= square_bb(D6);
    REQUIRE(is_in_check(pos, BLACK));
}

TEST_CASE("White king in check from black pawn on D5", "[check][pawn]")
{
    Position pos = empty_position();
    // Black pawn on D5 attacks E4 diagonally downward
    pos.bitboards[WK] |= square_bb(E4);
    pos.bitboards[BK] |= square_bb(H8);
    pos.bitboards[BP] |= square_bb(D5);
    REQUIRE(is_in_check(pos, WHITE));
}

TEST_CASE("Neither king in check with pawn buffers on ranks 2 and 7", "[check][quiet]")
{
    Position pos = empty_position();
    pos.bitboards[WK] |= square_bb(E1);
    pos.bitboards[BK] |= square_bb(E8);
    for (Square sq : {A2, B2, C2, D2, E2, F2, G2, H2})
        pos.bitboards[WP] |= square_bb(sq);
    for (Square sq : {A7, B7, C7, D7, E7, F7, G7, H7})
        pos.bitboards[BP] |= square_bb(sq);
    REQUIRE_FALSE(is_in_check(pos, WHITE));
    REQUIRE_FALSE(is_in_check(pos, BLACK));
}

// ---------------------------------------------------------------------------
// filter_legal_moves
// ---------------------------------------------------------------------------
TEST_CASE("filter_legal_moves: pinned rook move off the pin-file is removed", "[legal][filter]")
{
    // Ke1, WRe4 pinned by BRe8.
    // Re4-d4 leaves e-file open → illegal.
    // Re4-e5 stays on e-file → still blocks → legal.
    Position pos = empty_position();
    pos.bitboards[WK] |= square_bb(E1);
    pos.bitboards[WR] |= square_bb(E4);
    pos.bitboards[BR] |= square_bb(E8);
    pos.bitboards[BK] |= square_bb(H8);
    pos.side_to_move = WHITE;

    MoveList list;
    list.count = 0;
    add_move(list, make_move_struct(E4, D4, ROOK)); // illegal
    add_move(list, make_move_struct(E4, E5, ROOK)); // legal

    filter_legal_moves(pos, list);

    REQUIRE(list.count == 1);
    REQUIRE(list.moves[0].to_square == E5);
}

TEST_CASE("filter_legal_moves: capturing the checker is legal", "[legal][filter]")
{
    // Ke1 in check from BRe8; WRe4 can capture on e8.
    Position pos = empty_position();
    pos.bitboards[WK] |= square_bb(E1);
    pos.bitboards[WR] |= square_bb(E4);
    pos.bitboards[BR] |= square_bb(E8);
    pos.bitboards[BK] |= square_bb(H8);
    pos.side_to_move = WHITE;

    MoveList list;
    list.count = 0;
    add_move(list, make_move_struct(E4, E8, ROOK));

    filter_legal_moves(pos, list);

    REQUIRE(list.count == 1);
    REQUIRE(list.moves[0].to_square == E8);
}

TEST_CASE("filter_legal_moves: all moves kept when no check risk", "[legal][filter]")
{
    Position pos = empty_position();
    pos.bitboards[WK] |= square_bb(A1);
    pos.bitboards[WN] |= square_bb(C3);
    pos.bitboards[BK] |= square_bb(H8);
    pos.side_to_move = WHITE;

    MoveList list;
    list.count = 0;
    // All 8 knight moves from C3 – none expose Ka1
    add_move(list, make_move_struct(C3, B1, KNIGHT));
    add_move(list, make_move_struct(C3, D1, KNIGHT));
    add_move(list, make_move_struct(C3, A2, KNIGHT));
    add_move(list, make_move_struct(C3, E2, KNIGHT));
    add_move(list, make_move_struct(C3, A4, KNIGHT));
    add_move(list, make_move_struct(C3, E4, KNIGHT));
    add_move(list, make_move_struct(C3, B5, KNIGHT));
    add_move(list, make_move_struct(C3, D5, KNIGHT));

    filter_legal_moves(pos, list);

    REQUIRE(list.count == 8);
}

TEST_CASE("filter_legal_moves: king cannot step onto attacked square", "[legal][filter][king]")
{
    // Ke1, BRe8 covers the whole e-file.
    // Ke2 illegal, Kd1 and Kf1 legal.
    Position pos = empty_position();
    pos.bitboards[WK] |= square_bb(E1);
    pos.bitboards[BR] |= square_bb(E8);
    pos.bitboards[BK] |= square_bb(A8);
    pos.side_to_move = WHITE;

    MoveList list;
    list.count = 0;
    add_move(list, make_move_struct(E1, E2, KING)); // illegal – e-file covered
    add_move(list, make_move_struct(E1, D1, KING)); // legal
    add_move(list, make_move_struct(E1, F1, KING)); // legal

    filter_legal_moves(pos, list);

    REQUIRE(list.count == 2);
    for (int i = 0; i < list.count; i++)
        REQUIRE(list.moves[i].to_square != E2);
}