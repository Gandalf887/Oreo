#include "movegen.h"
#include "search.h"
#include "legal.h"
#include <iostream>

// ============================================================
// KING MOVES
// ============================================================

// Generates all pseudo-legal king moves: one step in all eight directions with
// friendly-piece blocking, plus castling. Each diagonal/orthogonal shift is masked
// against the A-file (0x0101...ULL) or H-file (0x8080...ULL) to prevent the king
// from wrapping around the board edge. Castling checks that both the transit and
// destination squares are unoccupied and not attacked, as required by the rules.
void generate_king_moves(const Position &pos, MoveList &list)
{

    Bitboard king = pos.side_to_move == WHITE ? pos.bitboards[WK] : pos.bitboards[BK];
    Bitboard friendly = friendly_pieces(pos);

    int from = __builtin_ctzll(king);

    Bitboard attacks = 0ULL;
    attacks |= (king << 8);                          // north
    attacks |= (king >> 8);                          // south
    attacks |= (king << 1) & ~0x0101010101010101ULL; // east
    attacks |= (king >> 1) & ~0x8080808080808080ULL; // westa
    attacks |= (king << 9) & ~0x0101010101010101ULL; // north east
    attacks |= (king << 7) & ~0x8080808080808080ULL; // north west
    attacks |= (king >> 7) & ~0x0101010101010101ULL; // south east
    attacks |= (king >> 9) & ~0x8080808080808080ULL; // south west

    attacks &= ~friendly;

    while (attacks)
    {
        int to = __builtin_ctzll(attacks);
        Move move;
        move.from_square = static_cast<Square>(from);
        move.to_square = static_cast<Square>(to);
        move.flag = NORMAL;
        move.piecetype = KING;
        add_move(list, move);
        // clear squares by friendly pieces
        attacks &= attacks - 1;
    }

    // castling lol dugongs dugongs dugongs

    if (pos.side_to_move == WHITE)
    {
        Bitboard occ = all_pieces(pos);

        // kingside - just check squares are empty
        if ((pos.castling_rights & WHITE_00) &&
            !(occ & square_bb(F1)) &&
            !(occ & square_bb(G1)) &&
            !is_square_attacked(pos, E1, BLACK) &&
            !is_square_attacked(pos, F1, BLACK) &&
            !is_square_attacked(pos, G1, BLACK))
        {
            Move move;
            move.from_square = E1;
            move.to_square = G1;
            move.flag = CASTLING;
            move.piecetype = KING;
            add_move(list, move);
        }

        // queenside
        if ((pos.castling_rights & WHITE_000) &&
            !(occ & square_bb(B1)) &&
            !(occ & square_bb(C1)) &&
            !(occ & square_bb(D1)) &&
            !is_square_attacked(pos, E1, BLACK) &&
            !is_square_attacked(pos, D1, BLACK) &&
            !is_square_attacked(pos, C1, BLACK))
        {
            Move move;
            move.from_square = E1;
            move.to_square = C1;
            move.flag = CASTLING;
            move.piecetype = KING;
            add_move(list, move);
        }
    }
    else
    {
        Bitboard occ = all_pieces(pos);

        // BLACK kingside
        if ((pos.castling_rights & BLACK_00) &&
            !(occ & square_bb(F8)) &&
            !(occ & square_bb(G8)) &&
            !is_square_attacked(pos, E8, WHITE) &&
            !is_square_attacked(pos, F8, WHITE) &&
            !is_square_attacked(pos, G8, WHITE))
        {
            Move move;
            move.from_square = E8;
            move.to_square = G8;
            move.flag = CASTLING;
            move.piecetype = KING;
            add_move(list, move);
        }

        // queenside black
        if ((pos.castling_rights & BLACK_000) &&
            !(occ & square_bb(B8)) &&
            !(occ & square_bb(C8)) &&
            !(occ & square_bb(D8)) &&
            !is_square_attacked(pos, E8, WHITE) &&
            !is_square_attacked(pos, D8, WHITE) &&
            !is_square_attacked(pos, C8, WHITE))
        {
            Move move;
            move.from_square = E8;
            move.to_square = C8;
            move.flag = CASTLING;
            move.piecetype = KING;
            add_move(list, move);
        }
    }
}

// ============================================================
// KNIGHT MOVES
// ============================================================

// Generates all pseudo-legal knight moves for every knight of the side to move.
// Each of the 8 L-shaped leaps is computed by shifting the knight's bitboard and
// masking off wrap-around: jumps going 2 east mask the A+B files (0x0303...ULL),
// jumps going 2 west mask the G+H files (0xC0C0...ULL), and 1-file jumps mask
// only the single edge file, preventing the knight from appearing on the opposite
// side of the board.
void generate_knight_moves(const Position &pos, MoveList &list)
{
    Bitboard knights = pos.side_to_move == WHITE ? pos.bitboards[WN] : pos.bitboards[BN];
    Bitboard friendly = friendly_pieces(pos);

    while (knights)
    {
        int from = __builtin_ctzll(knights);
        Bitboard knight = 1ULL << from;

        Bitboard attacks = 0ULL;
        attacks |= (knight << 17) & ~0x0101010101010101ULL; // 2N 1E — dest can't be A file
        attacks |= (knight << 15) & ~0x8080808080808080ULL; // 2N 1W — dest can't be H file
        attacks |= (knight << 10) & ~0x0303030303030303ULL; // 1N 2E — dest can't be A or B file
        attacks |= (knight << 6) & ~0xC0C0C0C0C0C0C0C0ULL;  // 1N 2W — dest can't be G or H file
        attacks |= (knight >> 6) & ~0x0303030303030303ULL;  // 1S 2E — dest can't be A or B file
        attacks |= (knight >> 10) & ~0xC0C0C0C0C0C0C0C0ULL; // 1S 2W — dest can't be G or H file
        attacks |= (knight >> 15) & ~0x0101010101010101ULL; // 2S 1E — dest can't be A file
        attacks |= (knight >> 17) & ~0x8080808080808080ULL; // 2S 1W — dest can't be H file

        attacks &= ~friendly;

        while (attacks)
        {
            int to = __builtin_ctzll(attacks);
            Move move;
            move.from_square = static_cast<Square>(from);
            move.to_square = static_cast<Square>(to);
            move.flag = NORMAL;
            move.piecetype = KNIGHT;
            add_move(list, move);
            attacks &= attacks - 1;
        }

        knights &= knights - 1;
    }
}

// ============================================================
// SLIDING PIECE ATTACK GENERATORS
// ============================================================

// Returns a bitboard of all squares a rook on sq can attack given the occupied
// squares. Rays are cast in all four orthogonal directions (north, south, east,
// west); each ray includes the first occupied square it hits (a capture target)
// but stops there so pieces behind it are not falsely attacked.
Bitboard rook_attacks(Square sq, Bitboard occupied)
{
    Bitboard attacks = 0ULL;
    int r = sq / 8, f = sq % 8;

    // north movement up
    for (int i = r + 1; i < 8; i++)
    {
        attacks |= 1ULL << (i * 8 + f);
        if (occupied & (1ULL << (i * 8 + f)))
            break;
    }
    // south movement down
    for (int i = r - 1; i >= 0; i--)
    {
        attacks |= 1ULL << (i * 8 + f);
        if (occupied & (1ULL << (i * 8 + f)))
            break;
    }
    // move east left
    for (int i = f + 1; i < 8; i++)
    {
        attacks |= 1ULL << (r * 8 + i);
        if (occupied & (1ULL << (r * 8 + i)))
            break;
    }
    // move west right
    for (int i = f - 1; i >= 0; i--)
    {
        attacks |= 1ULL << (r * 8 + i);
        if (occupied & (1ULL << (r * 8 + i)))
            break;
    }

    return attacks;
}

// Returns a bitboard of all squares a bishop on sq can attack given the occupied
// squares. Identical ray-casting logic to rook_attacks but along the four diagonal
// directions (NE, NW, SE, SW). Both i (rank) and j (file) are stepped together
// and the loop terminates when either board edge or a blocker is reached.
// bishop sliding
Bitboard bishop_attacks(Square sq, Bitboard occupied)
{
    Bitboard attacks = 0ULL;
    int r = sq / 8, f = sq % 8;

    // north east
    for (int i = r + 1, j = f + 1; i < 8 && j < 8; i++, j++)
    {
        attacks |= 1ULL << (i * 8 + j);
        if (occupied & (1ULL << (i * 8 + j)))
            break;
    }

    // nort west
    for (int i = r + 1, j = f - 1; i < 8 && j >= 0; i++, j--)
    {
        attacks |= 1ULL << (i * 8 + j);
        if (occupied & (1ULL << (i * 8 + j)))
            break;
    }

    // south east
    for (int i = r - 1, j = f + 1; i >= 0 && j < 8; i--, j++)
    {
        attacks |= 1ULL << (i * 8 + j);
        if (occupied & (1ULL << (i * 8 + j)))
            break;
    }

    // south west
    for (int i = r - 1, j = f - 1; i >= 0 && j >= 0; i--, j--)
    {
        attacks |= 1ULL << (i * 8 + j);
        if (occupied & (1ULL << (i * 8 + j)))
            break;
    }

    return attacks;
}

// Generates pseudo-legal moves for rooks, bishops, and queens by calling the
// corresponding attack generators and iterating destination squares. Queen moves
// are the union of rook and bishop attacks — no separate ray logic is needed.
// rooks + bishops + queens
void generate_sliding_moves(const Position &pos, MoveList &list)
{
    Bitboard friendly = friendly_pieces(pos);
    Bitboard occupied = all_pieces(pos);

    // rooks
    Bitboard rooks = pos.side_to_move == WHITE ? pos.bitboards[WR] : pos.bitboards[BR];
    while (rooks)
    {
        int from = __builtin_ctzll(rooks);
        Bitboard attacks = rook_attacks(static_cast<Square>(from), occupied) & ~friendly;

        while (attacks)
        {
            int to = __builtin_ctzll(attacks);
            Move move;
            move.from_square = static_cast<Square>(from);
            move.to_square = static_cast<Square>(to);
            move.flag = NORMAL;
            move.piecetype = ROOK;
            add_move(list, move);
            attacks &= attacks - 1;
        }
        rooks &= rooks - 1;
    }

    // bishops
    Bitboard bishops = pos.side_to_move == WHITE ? pos.bitboards[WB] : pos.bitboards[BB];
    while (bishops)
    {
        int from = __builtin_ctzll(bishops);
        Bitboard attacks = bishop_attacks(static_cast<Square>(from), occupied) & ~friendly;

        while (attacks)
        {
            int to = __builtin_ctzll(attacks);
            Move move;
            move.from_square = static_cast<Square>(from);
            move.to_square = static_cast<Square>(to);
            move.flag = NORMAL;
            move.piecetype = BISHOP;
            add_move(list, move);
            attacks &= attacks - 1;
        }
        bishops &= bishops - 1;
    }

    // queens = rook + bishop
    Bitboard queens = pos.side_to_move == WHITE ? pos.bitboards[WQ] : pos.bitboards[BQ];
    while (queens)
    {
        int from = __builtin_ctzll(queens);
        Bitboard attacks = (rook_attacks(static_cast<Square>(from), occupied) | bishop_attacks(static_cast<Square>(from), occupied)) & ~friendly;

        while (attacks)
        {
            int to = __builtin_ctzll(attacks);
            Move move;
            move.from_square = static_cast<Square>(from);
            move.to_square = static_cast<Square>(to);
            move.flag = NORMAL;
            move.piecetype = QUEEN;
            add_move(list, move);
            attacks &= attacks - 1;
        }
        queens &= queens - 1;
    }
}

// ============================================================
// PAWN MOVES
// ============================================================

// Generates all pseudo-legal pawn moves: single push, double push from the
// starting rank, diagonal captures, and en passant. The double-push mask
// (0x0000000000FF0000ULL for White, 0x0000FF0000000000ULL for Black) ensures
// the pawn is still on its starting rank before the second step is allowed.
// Diagonal captures are masked against the A-file or H-file to prevent
// wrap-around. All four promotions (Q/R/B/N) are generated whenever a pawn
// reaches the back rank, including promotion captures.
// pawn moves
void generate_pawn_moves(const Position &pos, MoveList &list)
{
    Bitboard pawns = pos.side_to_move == WHITE ? pos.bitboards[WP] : pos.bitboards[BP];
    Bitboard empty = empty_squares(pos);
    Bitboard enemy = enemy_pieces(pos);

    while (pawns)
    {
        int from = __builtin_ctzll(pawns);
        Bitboard pawn = 1ULL << from;

        if (pos.side_to_move == WHITE)
        {
            Bitboard single_push = (pawn << 8) & empty;
            Bitboard double_push = ((single_push & 0x0000000000FF0000ULL) << 8) & empty;
            Bitboard captures = 0ULL;
            captures |= (pawn << 9) & ~0x0101010101010101ULL & enemy;
            captures |= (pawn << 7) & ~0x8080808080808080ULL & enemy;

            if (pos.en_passant_square != NO_SQUARE)
            {
                Bitboard ep = square_bb(pos.en_passant_square);
                captures |= (pawn << 9) & ~0x0101010101010101ULL & ep;
                captures |= (pawn << 7) & ~0x8080808080808080ULL & ep;
            }

            while (single_push)
            {
                int to = __builtin_ctzll(single_push);
                Move move;
                move.from_square = static_cast<Square>(from);
                move.to_square = static_cast<Square>(to);
                move.piecetype = PAWN;

                if (to >= 56)
                {
                    // generate all 4 promots 1.5.6 debugging
                    for (PieceType pt : {QUEEN, ROOK, BISHOP, KNIGHT})
                    {
                        move.flag = PROMOTION;
                        move.piecetype = pt;
                        add_move(list, move);
                    }
                }

                else
                {
                    move.flag = NORMAL;
                    add_move(list, move);
                }
                single_push &= single_push - 1;
            }

            while (double_push)
            {
                int to = __builtin_ctzll(double_push);
                Move move;
                move.from_square = static_cast<Square>(from);
                move.to_square = static_cast<Square>(to);
                move.piecetype = PAWN;
                move.flag = NORMAL;
                add_move(list, move);
                double_push &= double_push - 1;
            }

            while (captures)
            {
                int to = __builtin_ctzll(captures);
                Move move;
                move.from_square = static_cast<Square>(from);
                move.to_square = static_cast<Square>(to);
                if (to >= 56) // promotion capture
                {
                    for (PieceType pt : {QUEEN, ROOK, BISHOP, KNIGHT})
                    {
                        move.flag = PROMOTION;
                        move.piecetype = pt;
                        add_move(list, move);
                    }
                }
                else
                {
                    move.piecetype = PAWN;
                    move.flag = (pos.en_passant_square != NO_SQUARE &&
                                 to == pos.en_passant_square)
                                    ? EN_PASSANT
                                    : NORMAL;
                    add_move(list, move);
                }
                captures &= captures - 1;
            }
        }
        else
        {
            Bitboard single_push = (pawn >> 8) & empty;
            Bitboard double_push = ((single_push & 0x0000FF0000000000ULL) >> 8) & empty;
            Bitboard captures = 0ULL;
            captures |= (pawn >> 7) & ~0x0101010101010101ULL & enemy;
            captures |= (pawn >> 9) & ~0x8080808080808080ULL & enemy;

            if (pos.en_passant_square != NO_SQUARE)
            {
                Bitboard ep = square_bb(pos.en_passant_square);
                captures |= (pawn >> 7) & ~0x0101010101010101ULL & ep;
                captures |= (pawn >> 9) & ~0x8080808080808080ULL & ep;
            }

            while (single_push)
            {
                int to = __builtin_ctzll(single_push);
                Move move;
                move.from_square = static_cast<Square>(from);
                move.to_square = static_cast<Square>(to);
                move.piecetype = PAWN;

                if (to <= 7)
                {
                    // generate all 4 promots 1.5.6 debugging
                    for (PieceType pt : {QUEEN, ROOK, BISHOP, KNIGHT})
                    {
                        move.flag = PROMOTION;
                        move.piecetype = pt;
                        add_move(list, move);
                    }
                }

                else
                {
                    move.flag = NORMAL;
                    add_move(list, move);
                }
                single_push &= single_push - 1;
            }

            while (double_push)
            {
                int to = __builtin_ctzll(double_push);
                Move move;
                move.from_square = static_cast<Square>(from);
                move.to_square = static_cast<Square>(to);
                move.piecetype = PAWN;
                move.flag = NORMAL;
                add_move(list, move);
                double_push &= double_push - 1;
            }

            while (captures)
            {
                int to = __builtin_ctzll(captures);
                Move move;
                move.from_square = static_cast<Square>(from);
                move.to_square = static_cast<Square>(to);

                if (to <= 7) // promotion capture
                {
                    for (PieceType pt : {QUEEN, ROOK, BISHOP, KNIGHT})
                    {
                        move.flag = PROMOTION;
                        move.piecetype = pt;
                        add_move(list, move);
                    }
                }
                else
                {
                    move.piecetype = PAWN;
                    move.flag = (pos.en_passant_square != NO_SQUARE &&
                                 to == pos.en_passant_square)
                                    ? EN_PASSANT
                                    : NORMAL;
                    add_move(list, move);
                }
                captures &= captures - 1;
            }
        }
        pawns &= pawns - 1;
    }
}

// ============================================================
// TOP-LEVEL MOVE GENERATION
// ============================================================

// Resets the move list and calls each piece-type generator in turn.
// Produces pseudo-legal moves — legality (leaving own king in check) is
// filtered separately by filter_legal_moves() in legal.cpp.
void generate_moves(const Position &pos, MoveList &list)
{
    list.count = 0;
    generate_king_moves(pos, list);
    generate_knight_moves(pos, list);
    generate_sliding_moves(pos, list);
    generate_pawn_moves(pos, list);
}

// ============================================================
// MOVE ORDERING
// ============================================================

// Assigns a numeric score to a move for ordering purposes. Captures are scored
// using MVV-LVA (Most Valuable Victim – Least Valuable Attacker): a table indexed
// by [attacker type][victim type] gives scores in the range 100–605, offset by
// +10000 so all captures outrank quiet moves. En passant is treated as a
// pawn-takes-pawn capture (score 105 + 10000). Quiet moves that match a killer
// slot at this depth score 9000 or 8000 respectively. All other quiet moves fall
// back to the history heuristic table (from-square × to-square).
// performance upgrades 1.1.0
// performance upgrade 1.7.0 implement killer heurestic
int score_move(const Position &pos, const Move &move, int depth) // 1.12.1
{
    // capturing a piece now gets a high base score
    Piece captured = piece_on(pos, move.to_square);

    if (captured != NO_PIECE || move.flag == EN_PASSANT)
    {
        if (move.flag == EN_PASSANT)
            return 105 + 10000;

        // MVV-LVA capture piece value - attacking piece value
        const int mvv_lva[6][6] = {
            // victim : P N B R Q K
            {105, 205, 305, 405, 505, 605}, // attacker P
            {104, 204, 304, 404, 504, 604}, // attacker N
            {103, 203, 303, 403, 503, 603}, // attacker B
            {102, 202, 302, 402, 502, 602}, // attacker R
            {101, 201, 301, 401, 501, 601}, // attacker Q
            {100, 200, 300, 400, 500, 600}, // attacker K
        };

        // get tghe attacker and victim piece types
        Piece attacker = piece_on(pos, move.from_square);
        int attacker_type = attacker % 6; // strip the colour
        int victim_type = captured % 6;   // strip colour

        return mvv_lva[attacker_type][victim_type] + 10000; // high enough base score? tweak later
    }

    // peformance upgrade 1.7.0 for the killer move heurestic bonuses
    if (depth >= 0 && depth < MAX_DEPTH)
    {
        if (move.from_square == killer_moves[depth][0].from_square && move.to_square == killer_moves[depth][0].to_square)
            return 9000;
        if (move.from_square == killer_moves[depth][1].from_square && move.to_square == killer_moves[depth][1].to_square)
            return 8000;
    }

    return history_table[move.from_square][move.to_square]; // performance upgrade 1.8.0 history heurestic lol dugongs :D

}


// Sorts the move list in descending order of score_move() using insertion sort.
// Insertion sort is efficient here because move lists are short (typically < 50)
// and are often nearly sorted already due to the TT best-move being placed first.
void order_moves(const Position &pos, MoveList &list, int depth)
{
    int scores[256];
    for (int i = 0; i < list.count; i++)
        scores[i] = score_move(pos, list.moves[i], depth);

    for (int i = 1; i < list.count; i++)
    {
        Move key = list.moves[i];
        int key_score = scores[i];
        int j = i - 1;
        while (j >= 0 && scores[j] < key_score)
        {
            list.moves[j + 1] = list.moves[j];
            scores[j + 1] = scores[j];
            j--;
        }
        list.moves[j + 1] = key;
        scores[j + 1] = key_score;
    }
}