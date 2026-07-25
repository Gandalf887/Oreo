// 1.14.0 overhaul starts below
#include "eval.h"

inline int iabs(int x) { return x < 0 ? -x : x; }; // 1.14.0.1 technically faster then include <cstlib> so this will do

// ============================================================
// FILE MASKS — initialised once via init_eval()
// ============================================================
Bitboard FILE_MASKS[8];
Bitboard RANK_MASKS[8];
Bitboard ADJACENT_FILES[8];

// Pre-computes bitmasks for each file, rank, and the two files adjacent to each file.
// Called once at startup; these masks are used throughout pawn and king evaluation.
void init_eval()
{
    for (int f = 0; f < 8; f++)
    {
        FILE_MASKS[f] = 0ULL;
        for (int r = 0; r < 8; r++)
            FILE_MASKS[f] |= 1ULL << (r * 8 + f);

        ADJACENT_FILES[f] = 0ULL;
        if (f > 0)
            ADJACENT_FILES[f] |= FILE_MASKS[f - 1];
        if (f < 7)
            ADJACENT_FILES[f] |= FILE_MASKS[f + 1];
    }
    for (int r = 0; r < 8; r++)
    {
        RANK_MASKS[r] = 0ULL;
        for (int f = 0; f < 8; f++)
            RANK_MASKS[r] |= 1ULL << (r * 8 + f);
    }
}

// ============================================================
// MATERIAL VALUES
// ============================================================
const int PIECE_VALUES[PIECE_COUNT] = {
    100, 320, 330, 500, 900, 20000,      // white
    -100, -320, -330, -500, -900, -20000 // black
};

// ============================================================
// PIECE-SQUARE TABLES (from White's perspective, a1=0, h8=63)
// BUG FIX: Tables are indexed [sq] for White, [mirror(sq)] for Black.
// 1.13.x had this backwards - White used mirror(sq), Black used sq.
// ============================================================

const int MG_PAWN_TABLE[64] = {
    0, 0, 0, 0, 0, 0, 0, 0,
    98, 134, 61, 95, 68, 126, 34, -11,
    -6, 7, 26, 31, 65, 56, 25, -20,
    -14, 13, 6, 21, 23, 12, 17, -23,
    -27, -2, -5, 12, 17, 6, 10, -25,
    -26, -4, -4, -10, 3, 3, 33, -12,
    -35, -1, -20, -23, -15, 24, 38, -22,
    0, 0, 0, 0, 0, 0, 0, 0};

const int EG_PAWN_TABLE[64] = {
    0, 0, 0, 0, 0, 0, 0, 0,
    178, 173, 158, 134, 147, 132, 165, 187,
    94, 100, 85, 67, 56, 53, 82, 84,
    32, 24, 13, 5, -2, 4, 17, 17,
    13, 9, -3, -7, -7, -8, 3, -1,
    4, 7, -6, 1, 0, -5, -1, -8,
    13, 8, 8, 10, 13, 0, 2, -7,
    0, 0, 0, 0, 0, 0, 0, 0};

const int MG_KNIGHT_TABLE[64] = {
    -50, -40, -30, -30, -30, -30, -40, -50,
    -40, -20, 0, 0, 0, 0, -20, -40,
    -30, 0, 10, 15, 15, 10, 0, -30,
    -30, 5, 15, 20, 20, 15, 5, -30,
    -30, 0, 15, 20, 20, 15, 0, -30,
    -30, 5, 10, 15, 15, 10, 5, -30,
    -40, -20, 0, 5, 5, 0, -20, -40,
    -50, -40, -30, -30, -30, -30, -40, -50};

const int EG_KNIGHT_TABLE[64] = {
    -58, -38, -13, -28, -31, -27, -63, -99,
    -25, -8, -25, -2, -9, -25, -24, -52,
    -24, -20, 10, 9, -1, -9, -19, -41,
    -17, 3, 22, 22, 22, 11, 8, -18,
    -18, -6, 16, 25, 16, 17, 4, -18,
    -23, -3, -1, 15, 10, -3, -20, -22,
    -42, -20, -10, -5, -2, -20, -23, -44,
    -29, -51, -23, -15, -22, -18, -50, -64};

const int MG_BISHOP_TABLE[64] = {
    -29, 4, -82, -37, -25, -42, 7, -8,
    -26, 16, -18, -13, 30, 59, 18, -47,
    -16, 37, 43, 40, 35, 50, 37, -2,
    -4, 5, 19, 50, 37, 37, 7, -2,
    -6, 13, 13, 26, 34, 12, 10, 4,
    0, 15, 15, 15, 14, 27, 18, 10,
    4, 15, 16, 0, 7, 21, 33, 1,
    -33, -3, -14, -21, -13, -12, -39, -21};

const int EG_BISHOP_TABLE[64] = {
    -14, -21, -11, -8, -7, -9, -17, -24,
    -8, -4, 7, -12, -3, -13, -4, -14,
    2, -8, 0, -1, -2, 6, 0, 4,
    -3, 9, 12, 9, 14, 10, 3, 2,
    -6, 3, 13, 19, 7, 10, -3, -9,
    -12, -3, 8, 10, 13, 3, -7, -15,
    -14, -18, -7, -1, 4, -9, -15, -27,
    -23, -9, -23, -5, -9, -16, -5, -17};

const int MG_ROOK_TABLE[64] = {
    32, 42, 32, 51, 63, 9, 31, 43,
    27, 32, 58, 62, 80, 67, 26, 44,
    -5, 19, 26, 36, 17, 45, 61, 16,
    -24, -11, 7, 26, 24, 35, -8, -20,
    -36, -26, -12, -1, 9, -7, 6, -23,
    -45, -25, -16, -17, 3, 0, -5, -33,
    -44, -16, -20, -9, -1, 11, -6, -71,
    -19, -13, 1, 17, 16, 7, -37, -26};

const int EG_ROOK_TABLE[64] = {
    13, 10, 18, 15, 12, 12, 8, 5,
    11, 13, 13, 11, -3, 3, 8, 3,
    7, 7, 7, 5, 4, -3, -5, -3,
    4, 3, 13, 1, 2, 1, -1, 2,
    3, 5, 8, 4, -5, -6, -8, -11,
    -4, 0, -5, -1, -7, -12, -8, -16,
    -6, -6, 0, 2, -9, -9, -11, -3,
    -9, 2, 3, -1, -5, -13, 4, -20};

const int MG_QUEEN_TABLE[64] = {
    -28, 0, 29, 12, 59, 44, 43, 45,
    -24, -39, -5, 1, -16, 57, 28, 54,
    -13, -17, 7, 8, 29, 56, 47, 57,
    -27, -27, -16, -16, -1, 17, -2, 1,
    -9, -26, -9, -10, -2, -4, 3, -3,
    -14, 2, -11, -2, -5, 2, 14, 5,
    -35, -8, 11, 2, 8, 15, -3, 1,
    -1, -18, -9, 10, -15, -25, -31, -50};

const int EG_QUEEN_TABLE[64] = {
    -9, 22, 22, 27, 27, 19, 10, 20,
    -17, 20, 32, 41, 58, 25, 30, 0,
    -20, 6, 9, 49, 47, 35, 19, 9,
    3, 22, 24, 45, 57, 40, 57, 36,
    -18, 28, 19, 47, 31, 34, 39, 23,
    -16, -27, 15, 6, 9, 17, 10, 5,
    -22, -23, -30, -16, -16, -23, -36, -32,
    -33, -28, -22, -43, -5, -32, -20, -41};

const int MG_KING_TABLE[64] = {
    -65, 23, 16, -15, -56, -34, 2, 13,
    29, -1, -20, -7, -8, -4, -38, -29,
    -9, 24, 2, -16, -20, 6, 22, -22,
    -17, -20, -12, -27, -30, -25, -14, -36,
    -49, -1, -27, -39, -46, -44, -33, -51,
    -14, -14, -22, -46, -44, -30, -15, -27,
    1, 7, -8, -64, -43, -16, 9, 8,
    -15, 36, 12, -54, 8, -28, 24, 14};

const int EG_KING_TABLE[64] = {
    -74, -35, -18, -18, -11, 15, 4, -17,
    -12, 17, 14, 17, 17, 38, 23, 11,
    10, 17, 23, 15, 20, 45, 44, 13,
    -8, 22, 24, 27, 26, 33, 26, 3,
    -18, -4, 21, 24, 27, 23, 9, -11,
    -19, -3, 11, 21, 23, 16, 7, -9,
    -27, -11, 4, 13, 14, 4, -5, -17,
    -53, -34, -21, -11, -28, -14, -24, -43};

// ============================================================
// HELPERS
// ============================================================

// BUG FIX: mirror flips rank so Black's tables read correctly.
// sq 0 (a1, White's back rank) maps to 56 (a8, Black's back rank).
inline int mirror(int sq) { return sq ^ 56; }

inline int popcount(Bitboard b) { return __builtin_popcountll(b); }

// Game phase: 24 = full middlegame, 0 = pure endgame.
// Returns a phase value from 0 (pure endgame) to 24 (full middlegame) based on
// remaining piece material. Knights/bishops = 1, rooks = 2, queens = 4.
// Used to blend middlegame and endgame scores via tapered_bonus().
int game_phase(const Position &pos)
{
    int phase = 0;
    phase += popcount(pos.bitboards[WN] | pos.bitboards[BN]) * 1;
    phase += popcount(pos.bitboards[WB] | pos.bitboards[BB]) * 1;
    phase += popcount(pos.bitboards[WR] | pos.bitboards[BR]) * 2;
    phase += popcount(pos.bitboards[WQ] | pos.bitboards[BQ]) * 4;
    return phase;
}

// Linearly interpolates between a middlegame (mg) and endgame (eg) score using
// the current game phase. At phase=24 returns mg; at phase=0 returns eg.
inline int tapered_bonus(int mg, int eg, int phase)
{
    return (mg * phase + eg * (24 - phase)) / 24;
}

// ============================================================
// PIECE-SQUARE TABLE LOOKUP
// BUG FIX: White uses sq directly; Black mirrors.
// 1.13.x had White mirroring and Black using sq — completely backwards.
// ============================================================

// Returns the middlegame piece-square bonus for a given piece on a given square.
// White pieces index the table directly; Black pieces use mirror(sq) and negate,
// so both sides are evaluated from White's perspective.
int mg_table(Piece piece, int sq)
{
    switch (piece)
    {
    case WP:
        return MG_PAWN_TABLE[sq];
    case WN:
        return MG_KNIGHT_TABLE[sq];
    case WB:
        return MG_BISHOP_TABLE[sq];
    case WR:
        return MG_ROOK_TABLE[sq];
    case WQ:
        return MG_QUEEN_TABLE[sq];
    case WK:
        return MG_KING_TABLE[sq];
    case BP:
        return -MG_PAWN_TABLE[mirror(sq)];
    case BN:
        return -MG_KNIGHT_TABLE[mirror(sq)];
    case BB:
        return -MG_BISHOP_TABLE[mirror(sq)];
    case BR:
        return -MG_ROOK_TABLE[mirror(sq)];
    case BQ:
        return -MG_QUEEN_TABLE[mirror(sq)];
    case BK:
        return -MG_KING_TABLE[mirror(sq)];
    default:
        return 0;
    }
}

// Same as mg_table but uses the endgame piece-square tables.
int eg_table(Piece piece, int sq)
{
    switch (piece)
    {
    case WP:
        return EG_PAWN_TABLE[sq];
    case WN:
        return EG_KNIGHT_TABLE[sq];
    case WB:
        return EG_BISHOP_TABLE[sq];
    case WR:
        return EG_ROOK_TABLE[sq];
    case WQ:
        return EG_QUEEN_TABLE[sq];
    case WK:
        return EG_KING_TABLE[sq];
    case BP:
        return -EG_PAWN_TABLE[mirror(sq)];
    case BN:
        return -EG_KNIGHT_TABLE[mirror(sq)];
    case BB:
        return -EG_BISHOP_TABLE[mirror(sq)];
    case BR:
        return -EG_ROOK_TABLE[mirror(sq)];
    case BQ:
        return -EG_QUEEN_TABLE[mirror(sq)];
    case BK:
        return -EG_KING_TABLE[mirror(sq)];
    default:
        return 0;
    }
}

// ============================================================
// PAWN STRUCTURE EVALUATION
// BUG FIX: Passed pawn bonus was being added TWICE — once as a
// flat bonus and again as an endgame extra. This made a rank-6
// passed pawn worth ~293cp, warping the entire eval. Now we use
// tapered_bonus() to blend a single MG and EG value.
// ============================================================

// Scores pawn structure: doubled pawn penalties, isolated pawn penalties, and
// passed pawn bonuses (tapered MG/EG). Passed pawn bonuses include king proximity
// terms — in the endgame, the defending king's distance from the passer and the
// attacking king's closeness to it both matter significantly.
int evaluate_pawns(const Position &pos, int phase)
{
    int score = 0;
    Bitboard wp = pos.bitboards[WP];
    Bitboard bp = pos.bitboards[BP];

    // King squares for proximity bonus
    int wk_sq = __builtin_ctzll(pos.bitboards[WK]);
    int wk_file = wk_sq % 8;
    int wk_rank = wk_sq / 8;
    int bk_sq = __builtin_ctzll(pos.bitboards[BK]);
    int bk_file = bk_sq % 8;
    int bk_rank = bk_sq / 8;

    // Passed pawn bonus tables (mg and eg separately)
    // Indexed by rank 0-7. rank 0 and 7 should never fire.
    static const int MG_PASSED[8] = {0, 10, 20, 35, 55, 80, 110, 0};
    static const int EG_PASSED[8] = {0, 20, 40, 65, 100, 145, 200, 0};

    for (int f = 0; f < 8; f++)
    {
        Bitboard wp_file = wp & FILE_MASKS[f];
        Bitboard bp_file = bp & FILE_MASKS[f];

        // Doubled pawn penalty
        if (popcount(wp_file) > 1)
            score -= 20;
        if (popcount(bp_file) > 1)
            score += 20;

        // Isolated pawn penalty
        if (wp_file && !(wp & ADJACENT_FILES[f]))
            score -= 15;
        if (bp_file && !(bp & ADJACENT_FILES[f]))
            score += 15;

        // ---- White passed pawns ----
        Bitboard wp_iter = wp_file;
        while (wp_iter)
        {
            int sq = __builtin_ctzll(wp_iter);
            int rank = sq / 8; // 0 = rank 1 (White's starting side)

            // Build a mask of all squares ahead of this pawn on this
            // and adjacent files (i.e. the pawn's "cone of concern").
            Bitboard ahead = 0ULL;
            for (int r = rank + 1; r < 8; r++)
            {
                ahead |= 1ULL << (r * 8 + f);
                if (f > 0)
                    ahead |= 1ULL << (r * 8 + f - 1);
                if (f < 7)
                    ahead |= 1ULL << (r * 8 + f + 1);
            }

            if (!(bp & ahead))
            {
                score += tapered_bonus(MG_PASSED[rank], EG_PASSED[rank], phase);
                // King proximity — only meaningful in endgame
                // Enemy king far from passer = more dangerous
                int bk_dist = iabs(bk_file - f) + iabs(bk_rank - rank);
                score += tapered_bonus(0, bk_dist * 5, phase);

                // Our king close to passer = better support
                int wk_dist = iabs(wk_file - f) + iabs(wk_rank - rank);
                score -= tapered_bonus(0, wk_dist * 3, phase);
            }
            wp_iter &= wp_iter - 1;
        }

        // ---- Black passed pawns ----
        Bitboard bp_iter = bp_file;
        while (bp_iter)
        {
            int sq = __builtin_ctzll(bp_iter);
            int rank = sq / 8; // 7 = rank 8 (Black's starting side)

            Bitboard ahead = 0ULL;
            for (int r = rank - 1; r >= 0; r--)
            {
                ahead |= 1ULL << (r * 8 + f);
                if (f > 0)
                    ahead |= 1ULL << (r * 8 + f - 1);
                if (f < 7)
                    ahead |= 1ULL << (r * 8 + f + 1);
            }

            // BUG FIX: Black's passed pawn rank bonus should reference
            // the *mirrored* rank (distance from promotion), not the raw rank.
            // rank 7 = Black's back rank (0 from promo), rank 0 = almost promoting.
            int mirrored_rank = 7 - rank;
            if (!(wp & ahead))
            {
                score -= tapered_bonus(MG_PASSED[mirrored_rank], EG_PASSED[mirrored_rank], phase);

                // Enemy king far from passer = more dangerous for White
                int wk_dist = iabs(wk_file - f) + iabs(wk_rank - rank);
                score -= tapered_bonus(0, wk_dist * 5, phase);

                // Black king close to passer = better support
                int bk_dist = iabs(bk_file - f) + iabs(bk_rank - rank);
                score += tapered_bonus(0, bk_dist * 3, phase);
            }

            bp_iter &= bp_iter - 1;
        }
    }

    return score;
}

// ============================================================
// KING SAFETY
// BUG FIX 1: King safety now only fires after castling (king on
// a/b/c or f/g/h file). Applying it from move 1 penalises
// normal development since e1/e8 have no pawn shield.
// BUG FIX 2: The open-file penalty for Black's king was checking
// White pawn absence but labelling it as a penalty on Black — the
// logic was the same sign for both sides, causing no differential.
// ============================================================

// Scores king shelter (pawn shield on the three files in front of the king) and
// open/semi-open file penalties near the king. Only applied after castling
// (king on a/b/c or f/g/h file) since the starting e-file has no natural shield.
// The total is scaled by game phase — king safety is far less important in the endgame.
int evaluate_king_safety(const Position &pos, int phase)
{
    Bitboard white_pawns = pos.bitboards[WP];
    Bitboard black_pawns = pos.bitboards[BP];

    int w_score = 0;
    int b_score = 0;

    // ---- White king ----
    int wk_sq = __builtin_ctzll(pos.bitboards[WK]);
    int wk_file = wk_sq % 8;
    int wk_rank = wk_sq / 8;

    // Only penalise/reward shield when king has actually castled.
    bool w_castled = (wk_file < 3 || wk_file > 4);
    if (w_castled && wk_rank < 6)
    {
        for (int df = -1; df <= 1; df++)
        {
            int f = wk_file + df;
            if (f < 0 || f > 7)
                continue;
            if (white_pawns & (1ULL << ((wk_rank + 1) * 8 + f)))
                w_score += 10;
            else if (wk_rank + 2 < 8 && (white_pawns & (1ULL << ((wk_rank + 2) * 8 + f))))
                w_score += 5;
            else
                w_score -= 15;
        }
    }

    // Open-file penalty near king (semi/fully open = attackers can rush in)
    for (int df = -1; df <= 1; df++)
    {
        int f = wk_file + df;
        if (f < 0 || f > 7)
            continue;
        bool no_wp = !(white_pawns & FILE_MASKS[f]);
        bool no_bp = !(black_pawns & FILE_MASKS[f]);
        if (no_wp && no_bp)
            w_score -= 20; // fully open
        else if (no_wp)
            w_score -= 10; // semi-open (White pawn gone)
    }

    // ---- Black king ----
    int bk_sq = __builtin_ctzll(pos.bitboards[BK]);
    int bk_file = bk_sq % 8;
    int bk_rank = bk_sq / 8;

    bool b_castled = (bk_file < 3 || bk_file > 4);
    if (b_castled && bk_rank > 1)
    {
        for (int df = -1; df <= 1; df++)
        {
            int f = bk_file + df;
            if (f < 0 || f > 7)
                continue;
            if (black_pawns & (1ULL << ((bk_rank - 1) * 8 + f)))
                b_score += 10;
            else if (bk_rank - 2 >= 0 && (black_pawns & (1ULL << ((bk_rank - 2) * 8 + f))))
                b_score += 5;
            else
                b_score -= 15;
        }
    }

    // BUG FIX: was checking White pawn absence for Black's open file penalty.
    for (int df = -1; df <= 1; df++)
    {
        int f = bk_file + df;
        if (f < 0 || f > 7)
            continue;
        bool no_wp = !(white_pawns & FILE_MASKS[f]);
        bool no_bp = !(black_pawns & FILE_MASKS[f]);
        if (no_wp && no_bp)
            b_score -= 20;
        else if (no_bp)
            b_score -= 10; // BUG FIX: was (no_wp) before
    }

    // Scale by phase — king safety matters far more in the middlegame.
    return ((w_score - b_score) * phase) / 24;
}

// ============================================================
// MOBILITY
// ============================================================

// Counts the number of legal destination squares for each piece type, weighted by
// piece: knights x3, bishops x2, rooks x2, queens x1. A higher weight on knights
// reflects that a short-range piece with few squares available is significantly
// worse than a long-range piece in the same situation.
int evaluate_mobility(const Position &pos)
{
    int score = 0;
    Bitboard occupied = all_pieces(pos);
    Bitboard white = white_pieces(pos);
    Bitboard black = black_pieces(pos);

    // Knights (weight 3 — lots of squares matters a lot for a short-range piece)
    Bitboard wn = pos.bitboards[WN];
    while (wn)
    {
        int sq = __builtin_ctzll(wn);
        score += popcount(knight_attacks_bb(sq) & ~white) * 3;
        wn &= wn - 1;
    }
    Bitboard bn = pos.bitboards[BN];
    while (bn)
    {
        int sq = __builtin_ctzll(bn);
        score -= popcount(knight_attacks_bb(sq) & ~black) * 3;
        bn &= bn - 1;
    }

    // Bishops (weight 2)
    Bitboard wb = pos.bitboards[WB];
    while (wb)
    {
        int sq = __builtin_ctzll(wb);
        score += popcount(bishop_attacks(static_cast<Square>(sq), occupied) & ~white) * 2;
        wb &= wb - 1;
    }
    Bitboard bb_pieces = pos.bitboards[BB];
    while (bb_pieces)
    {
        int sq = __builtin_ctzll(bb_pieces);
        score -= popcount(bishop_attacks(static_cast<Square>(sq), occupied) & ~black) * 2;
        bb_pieces &= bb_pieces - 1;
    }

    // Rooks (weight 2 — a trapped rook is a serious problem)
    Bitboard wr = pos.bitboards[WR];
    while (wr)
    {
        int sq = __builtin_ctzll(wr);
        score += popcount(rook_attacks(static_cast<Square>(sq), occupied) & ~white) * 2;
        wr &= wr - 1;
    }
    Bitboard br = pos.bitboards[BR];
    while (br)
    {
        int sq = __builtin_ctzll(br);
        score -= popcount(rook_attacks(static_cast<Square>(sq), occupied) & ~black) * 2;
        br &= br - 1;
    }

    // Queens (weight 1 — queens have very high raw mobility, so a small
    // weight still produces a meaningful bonus without dominating everything)
    Bitboard wq = pos.bitboards[WQ];
    while (wq)
    {
        int sq = __builtin_ctzll(wq);
        score += popcount((bishop_attacks(static_cast<Square>(sq), occupied) |
                           rook_attacks(static_cast<Square>(sq), occupied)) &
                          ~white) *
                 1;
        wq &= wq - 1;
    }
    Bitboard bq = pos.bitboards[BQ];
    while (bq)
    {
        int sq = __builtin_ctzll(bq);
        score -= popcount((bishop_attacks(static_cast<Square>(sq), occupied) |
                           rook_attacks(static_cast<Square>(sq), occupied)) &
                          ~black) *
                 1;
        bq &= bq - 1;
    }

    return score;
}

// ============================================================
// ROOK EVALUATION
// ============================================================

// Scores rook placement: +20 for an open file (no pawns of either colour),
// +10/−10 for a semi-open file (own pawns gone), and a bonus for occupying
// the 7th rank (rank 6 for White, rank 1 for Black) which traps the enemy king
// and attacks unadvanced pawns.
int evaluate_rooks(const Position &pos)
{
    int score = 0;
    Bitboard wp = pos.bitboards[WP];
    Bitboard bp = pos.bitboards[BP];

    Bitboard wr = pos.bitboards[WR];
    while (wr)
    {
        int sq = __builtin_ctzll(wr);
        int f = sq % 8;
        int r = sq / 8;
        bool no_wp = !(wp & FILE_MASKS[f]);
        bool no_bp = !(bp & FILE_MASKS[f]);

        if (no_wp && no_bp)
            score += 20; // open file
        else if (no_wp)
            score += 10; // semi-open (White's pawn gone)

        if (r == 6)
            score += 25; // 7th rank (0-indexed rank 6)

        wr &= wr - 1;
    }

    Bitboard br = pos.bitboards[BR];
    while (br)
    {
        int sq = __builtin_ctzll(br);
        int f = sq % 8;
        int r = sq / 8;
        bool no_wp = !(wp & FILE_MASKS[f]);
        bool no_bp = !(bp & FILE_MASKS[f]);

        if (no_wp && no_bp)
            score -= 20;
        else if (no_bp)
            score -= 10; // BUG FIX: was (no_wp) — wrong side

        if (r == 1)
            score -= 25; // 7th rank from Black's side (0-indexed rank 1)

        br &= br - 1;
    }

    return score;
}

//============================================================
// EVALUATE ENGDGAME MOPUP
//============================================================
// Add inside evaluate(), only when one side has no pawns and minimal material
// i.e. pure K+R or K+Q vs K endings 1.14.0.1
// In near-pure endgames (phase <= 4) with a large material imbalance, rewards
// pushing the losing king to a corner and closing the distance between kings.
// This guides the engine to convert won K+R/K+Q vs K positions that pure
// material/PST evaluation alone cannot reliably finish.
int evaluate_mopup(const Position &pos, int phase)
{
    // Only kick in when very close to pure endgame
    if (phase > 4)
        return 0;

    int score = 0;

    // If White has a large material advantage, push Black king to corner
    int material_diff = 0;
    for (int i = 0; i < PIECE_COUNT; i++)
        material_diff += PIECE_VALUES[i] * popcount(pos.bitboards[i]);

    if (material_diff > 400) // White is up a rook or more
    {
        int bk_sq = __builtin_ctzll(pos.bitboards[BK]);
        int bk_file = bk_sq % 8;
        int bk_rank = bk_sq / 8;

        // Manhattan distance from centre — corner = 6, centre = 0
        int centre_dist = (iabs(bk_file - 3) + iabs(bk_rank - 3));
        score += centre_dist * 10;

        // Bonus for White king being close to Black king (to assist mate)
        int wk_sq = __builtin_ctzll(pos.bitboards[WK]);
        int wk_file = wk_sq % 8;
        int wk_rank = wk_sq / 8;
        int king_dist = iabs(wk_file - bk_file) + iabs(wk_rank - bk_rank);
        score += (14 - king_dist) * 5;
    }

    if (material_diff < -400) // Black is up
    {
        int wk_sq = __builtin_ctzll(pos.bitboards[WK]);
        int wk_file = wk_sq % 8;
        int wk_rank = wk_sq / 8;
        int centre_dist = (iabs(wk_file - 3) + iabs(wk_rank - 3));
        score -= centre_dist * 10;

        int bk_sq = __builtin_ctzll(pos.bitboards[BK]);
        int bk_file = bk_sq % 8;
        int bk_rank = bk_sq / 8;
        int king_dist = iabs(wk_file - bk_file) + iabs(wk_rank - bk_rank);
        score -= (14 - king_dist) * 5;
    }

    return score;
}

//============================================================
// EVALUATE DEVELOPMENT
//============================================================
// Only applies in the opening (high phase = lots of pieces = early game) 1.14.0.2
// Applies opening-specific bonuses and penalties: rewards central pawn presence,
// penalises unmoved minor pieces and central pawns, penalises early queen
// development, and rewards castling. Fades out entirely below phase 10 so it
// has no effect once the opening is over.
int evaluate_development(const Position &pos, int phase)
{
    // Fade out completely by move ~10 (phase drops as pieces trade)
    if (phase < 10)
        return 0;

    int score = 0;

    // ---- Pawn centre control ----
    // Bonus for pawns on e4/d4 (White) and e5/d5 (Black)
    if (pos.bitboards[WP] & (1ULL << 27))
        score += 30; // d4
    if (pos.bitboards[WP] & (1ULL << 28))
        score += 30; // e4
    if (pos.bitboards[BP] & (1ULL << 35))
        score -= 30; // d5
    if (pos.bitboards[BP] & (1ULL << 36))
        score -= 30; // e5

    // ---- Penalty for unmoved central pawns ----
    // d2/e2 pawns still on starting square = not developing
    if (pos.bitboards[WP] & (1ULL << 11))
        score -= 20; // d2
    if (pos.bitboards[WP] & (1ULL << 12))
        score -= 20; // e2
    if (pos.bitboards[BP] & (1ULL << 51))
        score += 20; // d7
    if (pos.bitboards[BP] & (1ULL << 52))
        score += 20; // e7

    // ---- Penalty for unmoved minor pieces ----
    // Knights and bishops still on back rank
    if (pos.bitboards[WN] & (1ULL << 1))
        score -= 80; // b1
    if (pos.bitboards[WN] & (1ULL << 6))
        score -= 80; // g1
    if (pos.bitboards[WB] & (1ULL << 2))
        score -= 60; // c1
    if (pos.bitboards[WB] & (1ULL << 5))
        score -= 60; // f1
    if (pos.bitboards[BN] & (1ULL << 57))
        score += 80; // b8
    if (pos.bitboards[BN] & (1ULL << 62))
        score += 80; // g8
    if (pos.bitboards[BB] & (1ULL << 58))
        score += 60; // c8
    if (pos.bitboards[BB] & (1ULL << 61))
        score += 60; // f8

    // ---- Queen early development penalty ----
    // Queen moved before move ~6 = bad
    if (!(pos.bitboards[WQ] & (1ULL << 3)))
        score -= 20; // not on d1
    if (!(pos.bitboards[BQ] & (1ULL << 59)))
        score += 20; // not on d8

    // ---- Castling reward ----
    // Approximated: king off e1/e8 and not in centre = probably castled
    int wk_sq = __builtin_ctzll(pos.bitboards[WK]);
    int bk_sq = __builtin_ctzll(pos.bitboards[BK]);
    if (wk_sq == 6 || wk_sq == 2)
        score += 40; // g1 or c1
    if (bk_sq == 62 || bk_sq == 58)
        score -= 40; // g8 or c8

    // Scale by phase — only meaningful in the opening
    return score;
}

// ============================================================
// MAIN EVALUATE FUNCTION
// ============================================================

// Top-level evaluation function. Sums material, tapered piece-square tables,
// bishop pair bonus, and all structural sub-evaluations. Returns the score from
// the perspective of the side to move (positive = good for the mover), which is
// the convention required by the negamax search framework.
int evaluate(const Position &pos)
{
    int score = 0;
    int phase = game_phase(pos);

    // Material + tapered piece-square tables
    for (int i = 0; i < PIECE_COUNT; i++)
    {
        Bitboard bb = pos.bitboards[i];
        while (bb)
        {
            int sq = __builtin_ctzll(bb);
            score += PIECE_VALUES[i];
            score += tapered_bonus(
                mg_table(static_cast<Piece>(i), sq),
                eg_table(static_cast<Piece>(i), sq),
                phase);
            bb &= bb - 1;
        }
    }

    // Bishop pair bonus
    if (popcount(pos.bitboards[WB]) >= 2)
        score += 30;
    if (popcount(pos.bitboards[BB]) >= 2)
        score -= 30;

    // Structural terms
    score += evaluate_pawns(pos, phase);
    score += evaluate_king_safety(pos, phase);
    score += evaluate_mobility(pos);
    score += evaluate_rooks(pos);
    score += evaluate_mopup(pos, phase);
    score += evaluate_development(pos, phase);

    // Return from the side-to-move's perspective (negamax convention)
    return pos.side_to_move == WHITE ? score : -score;
}