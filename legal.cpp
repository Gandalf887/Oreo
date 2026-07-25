#include "legal.h"
#include "movegen.h"

// ============================================================
// ATTACK DETECTION
// ============================================================

// Returns true if sq is attacked by any piece of the given colour.
// Uses a "reverse attack" technique: instead of enumerating attacker moves,
// we place a virtual piece of each type on sq and check whether its attack
// pattern intersects the real attacker bitboard. This is correct because
// attack patterns are symmetric (a knight on A attacks B iff a knight on B
// attacks A). Pawn attacks are not symmetric, so White and Black are handled
// separately: a square attacked by a White pawn is below-diagonal from that
// pawn, so we look at squares diagonally behind sq relative to the attacker's
// direction. Sliding piece attacks (rook/bishop) reuse the ray-casting
// generators from movegen.cpp, which already handle blockers correctly.
bool is_square_attacked(const Position &pos, Square sq, Colour attacker)
{
    // figure out right pieces to attack
    Bitboard pawns = attacker == WHITE ? pos.bitboards[WP] : pos.bitboards[BP];
    Bitboard knights = attacker == WHITE ? pos.bitboards[WN] : pos.bitboards[BN];
    Bitboard bishops = attacker == WHITE ? pos.bitboards[WB] : pos.bitboards[BB];
    Bitboard rooks = attacker == WHITE ? pos.bitboards[WR] : pos.bitboards[BR];
    Bitboard queens = attacker == WHITE ? pos.bitboards[WQ] : pos.bitboards[BQ];
    Bitboard kings = attacker == WHITE ? pos.bitboards[WK] : pos.bitboards[BK];
    Bitboard occupied = all_pieces(pos);
    Bitboard sq_bb = square_bb(sq);

    // knights
    Bitboard knight_attacks = 0ULL;
    knight_attacks |= (sq_bb << 17) & ~0x0101010101010101ULL;
    knight_attacks |= (sq_bb << 15) & ~0x8080808080808080ULL;
    knight_attacks |= (sq_bb << 10) & ~0x0303030303030303ULL;
    knight_attacks |= (sq_bb << 6) & ~0xC0C0C0C0C0C0C0C0ULL;
    knight_attacks |= (sq_bb >> 6) & ~0x0303030303030303ULL;
    knight_attacks |= (sq_bb >> 10) & ~0xC0C0C0C0C0C0C0C0ULL;
    knight_attacks |= (sq_bb >> 15) & ~0x0101010101010101ULL;
    knight_attacks |= (sq_bb >> 17) & ~0x8080808080808080ULL;
    if (knight_attacks & knights)
        return true;

    // king attacks
    Bitboard king_attacks = 0ULL;
    king_attacks |= ((sq_bb & ~0x8080808080808080ULL) << 1); // east
    king_attacks |= ((sq_bb & ~0x0101010101010101ULL) >> 1); // west
    king_attacks |= ((sq_bb & ~0x8080808080808080ULL) << 9); // NE
    king_attacks |= ((sq_bb & ~0x0101010101010101ULL) >> 9); // SW
    king_attacks |= ((sq_bb & ~0x0101010101010101ULL) << 7); // NW
    king_attacks |= ((sq_bb & ~0x8080808080808080ULL) >> 7); // SE
    king_attacks |= (sq_bb << 8);                            // north
    king_attacks |= (sq_bb >> 8);                            // south
    if (king_attacks & kings)
        return true;

    // pawn attacks
    if (attacker == WHITE)
    {
        // white pawns will attack up so check diagnoally below?
        Bitboard pawn_attacks = 0ULL;
        pawn_attacks |= (sq_bb >> 9) & ~0x8080808080808080ULL;
        pawn_attacks |= (sq_bb >> 7) & ~0x0101010101010101ULL;
        if (pawn_attacks & pawns)
            return true;
    }
    else
    {
        // black pawns downward then
        Bitboard pawn_attacks = 0ULL;
        pawn_attacks |= (sq_bb << 9) & ~0x0101010101010101ULL;
        pawn_attacks |= (sq_bb << 7) & ~0x8080808080808080ULL;
        if (pawn_attacks & pawns)
            return true;
    }

    // check the rook and queen (straights)
    Bitboard rook_queen = rooks | queens;
    if (rook_attacks(sq, occupied) & rook_queen)
        return true;

    // check the bishop and queen (diagnoal)
    Bitboard bishop_queen = bishops | queens;
    if (bishop_attacks(sq, occupied) & bishop_queen)
        return true;

    return false;
}
// ============================================================
// CHECK DETECTION
// ============================================================

// Returns true if the given colour's king is currently in check.
// Locates the king square via __builtin_ctzll (index of lowest set bit)
// then delegates to is_square_attacked() with the opposite colour as attacker.
bool is_in_check(const Position &pos, Colour colour)
{
    // find the kings square
    Bitboard king = colour == WHITE ? pos.bitboards[WK] : pos.bitboards[BK];
    Square king_sq = static_cast<Square>(__builtin_ctzll(king));

    // check the square attacked by enemey
    Colour attacker = colour == WHITE ? BLACK : WHITE;
    return is_square_attacked(pos, king_sq, attacker);
}

// ============================================================
// LEGALITY FILTERING
// ============================================================

// Reduces a pseudo-legal move list to only fully legal moves by making each
// move, checking whether the moving side's king is left in check, then
// unmaking it. Castling requires two extra checks before make_move: the king
// must not be in check on its origin square, and must not pass through an
// attacked square (the destination is already verified by generate_king_moves).
// The result overwrites the input list in place.
void filter_legal_moves(Position &pos, MoveList &list)
{
    MoveList legal;
    legal.count = 0;

    for (int i = 0; i < list.count; i++)
    {
        // special castling check — king cannot pass through attacked square
        if (list.moves[i].flag == CASTLING)
        {
            Colour enemy = pos.side_to_move == WHITE ? BLACK : WHITE;

            // check the square the king passes through
            Square pass_through;
            if (list.moves[i].to_square == G1)
                pass_through = F1;
            else if (list.moves[i].to_square == C1)
                pass_through = D1;
            else if (list.moves[i].to_square == G8)
                pass_through = F8;
            else
                pass_through = D8;

            if (is_square_attacked(pos, pass_through, enemy))
                continue;

            // also check king is not currently in check
            if (is_square_attacked(pos, list.moves[i].from_square, enemy))
                continue;
        }

        UndoInfo undo;
        make_move(pos, list.moves[i], undo);

        // check once moved still not in check
        if (!is_in_check(pos, undo.side_to_move))
        {
            add_move(legal, list.moves[i]);
        }

        unmake_move(pos, list.moves[i], undo);
    }

    list = legal;
}