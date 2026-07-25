#include "makeunmake.h"
#include "zobrist.h"

// ============================================================
// PIECE LOOKUPS
// ============================================================

// Scans all 12 bitboards to find which piece occupies sq.
// Returns NO_PIECE if the square is empty. Called frequently during
// make/unmake so results are used immediately rather than cached.
Piece piece_on(const Position &pos, Square sq)
{
    for (int i = 0; i < PIECE_COUNT; i++)
    {
        if (square_set(pos.bitboards[i], sq))
            return static_cast<Piece>(i);
    }
    return NO_PIECE;
}

// ============================================================
// MAKE MOVES
// ============================================================

// Applies a move to the position and incrementally updates the Zobrist hash.
// All state needed to reverse the move (captured piece, castling rights,
// en passant square, clocks, hash) is saved into undo before anything changes.
// Hash updates follow the XOR-in/XOR-out pattern: remove the old value, apply
// the change, XOR in the new value. Special cases handled in order:
//   EN_PASSANT  — the captured pawn sits one rank behind the destination, not on it.
//   CASTLING    — the rook is also relocated; all four cases are handled explicitly.
//   PROMOTION   — the pawn on to_square is replaced by the promoted piece type;
//                 both the pawn and promoted piece are XOR'd into the hash separately.
// En passant rights are reset each move and only re-set for double pawn pushes
// (|diff| == 16). Castling rights are stripped whenever a king or corner rook moves
// or is captured, then the new rights value is XOR'd into the hashs
void make_move(Position &pos, const Move &move, UndoInfo &undo)
{
    // save current state
    undo.captured = piece_on(pos, move.to_square);
    undo.side_to_move = pos.side_to_move;
    undo.castling_rights = pos.castling_rights;
    undo.en_passant_square = pos.en_passant_square;
    undo.fullmove_clock = pos.fullmove_clock;
    undo.halfmove_clock = pos.halfmove_clock;
    undo.hash = pos.hash; //1.14.0 bug fixes and overhaul updates

    uint64_t h = pos.hash;

    // figure out which piece is moving
    Piece piece = piece_on(pos, move.from_square);

    // XOR out old en passant if any
    if (pos.en_passant_square != NO_SQUARE)
        h ^= en_passant_keys[pos.en_passant_square % 8];

    // XOR out old castling rights
    h ^= castling_keys[pos.castling_rights];

    // remove captured piece from hash
    if (undo.captured != NO_PIECE)
    {
        pos.bitboards[undo.captured] &= ~square_bb(move.to_square);
        h ^= piece_keys[undo.captured][move.to_square];
    }

    // move the piece
    pos.bitboards[piece] &= ~square_bb(move.from_square);
    pos.bitboards[piece] |= square_bb(move.to_square);
    h ^= piece_keys[piece][move.from_square]; // remove from old square
    h ^= piece_keys[piece][move.to_square];   // add to new square

    // special moves
    if (move.flag == EN_PASSANT)
    {
        Square ep_pawn = static_cast<Square>(move.to_square + (pos.side_to_move == WHITE ? -8 : 8));
        Piece ep_piece = pos.side_to_move == WHITE ? BP : WP;
        pos.bitboards[ep_piece] &= ~square_bb(ep_pawn);
        h ^= piece_keys[ep_piece][ep_pawn];
    }

    if (move.flag == CASTLING)
    {
        if (move.to_square == G1)
        {
            pos.bitboards[WR] &= ~square_bb(H1);
            pos.bitboards[WR] |= square_bb(F1);
            h ^= piece_keys[WR][H1];
            h ^= piece_keys[WR][F1];
        }
        else if (move.to_square == C1)
        {
            pos.bitboards[WR] &= ~square_bb(A1);
            pos.bitboards[WR] |= square_bb(D1);
            h ^= piece_keys[WR][A1];
            h ^= piece_keys[WR][D1];
        }
        else if (move.to_square == G8)
        {
            pos.bitboards[BR] &= ~square_bb(H8);
            pos.bitboards[BR] |= square_bb(F8);
            h ^= piece_keys[BR][H8];
            h ^= piece_keys[BR][F8];
        }
        else if (move.to_square == C8)
        {
            pos.bitboards[BR] &= ~square_bb(A8);
            pos.bitboards[BR] |= square_bb(D8);
            h ^= piece_keys[BR][A8];
            h ^= piece_keys[BR][D8];
        }
    }

    if (move.flag == PROMOTION)
    {
        Piece promoted;
        if (pos.side_to_move == WHITE)
        {
            switch (move.piecetype)
            {
            case QUEEN:  promoted = WQ; break;
            case ROOK:   promoted = WR; break;
            case BISHOP: promoted = WB; break;
            case KNIGHT: promoted = WN; break;
            default:     promoted = WQ; break;
            }
        }
        else
        {
            switch (move.piecetype)
            {
            case QUEEN:  promoted = BQ; break;
            case ROOK:   promoted = BR; break;
            case BISHOP: promoted = BB; break;
            case KNIGHT: promoted = BN; break;
            default:     promoted = BQ; break;
            }
        }
        // remove pawn, add promoted piece
        h ^= piece_keys[piece][move.to_square];      // remove pawn from to_square
        pos.bitboards[piece] &= ~square_bb(move.to_square);
        pos.bitboards[promoted] |= square_bb(move.to_square);
        h ^= piece_keys[promoted][move.to_square];   // add promoted piece
    }

    // update en passant hash
    pos.en_passant_square = NO_SQUARE;
    if (move.piecetype == PAWN)
    {
        int diff = move.to_square - move.from_square;
        if (diff == 16)
        {
            pos.en_passant_square = static_cast<Square>(move.from_square + 8);
            h ^= en_passant_keys[pos.en_passant_square % 8];
        }
        else if (diff == -16)
        {
            pos.en_passant_square = static_cast<Square>(move.from_square - 8);
            h ^= en_passant_keys[pos.en_passant_square % 8];
        }
    }

    // update castling rights
    if (piece == WK) pos.castling_rights &= ~(WHITE_00 | WHITE_000);
    if (piece == BK) pos.castling_rights &= ~(BLACK_00 | BLACK_000);
    if (move.from_square == A1 || move.to_square == A1) pos.castling_rights &= ~WHITE_000;
    if (move.from_square == A8 || move.to_square == A8) pos.castling_rights &= ~BLACK_000;
    if (move.from_square == H1 || move.to_square == H1) pos.castling_rights &= ~WHITE_00;
    if (move.from_square == H8 || move.to_square == H8) pos.castling_rights &= ~BLACK_00;

    // XOR in new castling rights
    h ^= castling_keys[pos.castling_rights];

    // flip side to move
    h ^= side_key;

    // update clocks
    pos.halfmove_clock++;
    if (undo.captured != NO_PIECE || move.piecetype == PAWN)
        pos.halfmove_clock = 0;
    if (pos.side_to_move == BLACK)
        pos.fullmove_clock++;
    pos.side_to_move = (pos.side_to_move == WHITE) ? BLACK : WHITE;

    pos.hash = h;
}

// ============================================================
// UNMAKE MOVE
// ============================================================

// Reverses a move by restoring all position state from the UndoInfo saved during
// make_move. The hash is restored directly from undo rather than being
// recomputed incrementally — this is simpler and equally correct since the saved
// hash is guaranteed to be consistent with the restored state. Promotion is
// handled first: the promoted piece on to_square is the removed and the original pawn
// is restored on from_square. En passant restores the captured pawn on the square
// behind the destination. Castling moves the rook back to its original corner.
void unmake_move(Position &pos, const Move &move, const UndoInfo &undo)
{
    // just restore the full state - hash restores automatically
    pos.side_to_move = undo.side_to_move;

    Piece piece = piece_on(pos, move.to_square);

    if (move.flag == PROMOTION)
    {
        Piece promoted;
        if (pos.side_to_move == WHITE)
        {
            switch (move.piecetype)
            {
            case QUEEN:  promoted = WQ; break;
            case ROOK:   promoted = WR; break;
            case BISHOP: promoted = WB; break;
            case KNIGHT: promoted = WN; break;
            default:     promoted = WQ; break;
            }
        }
        else
        {
            switch (move.piecetype)
            {
            case QUEEN:  promoted = BQ; break;
            case ROOK:   promoted = BR; break;
            case BISHOP: promoted = BB; break;
            case KNIGHT: promoted = BN; break;
            default:     promoted = BQ; break;
            }
        }
        pos.bitboards[promoted] &= ~square_bb(move.to_square);
        Piece pawn = pos.side_to_move == WHITE ? WP : BP;
        pos.bitboards[pawn] |= square_bb(move.from_square);
    }
    else
    {
        pos.bitboards[piece] &= ~square_bb(move.to_square);
        pos.bitboards[piece] |= square_bb(move.from_square);
    }

    if (undo.captured != NO_PIECE)
        pos.bitboards[undo.captured] |= square_bb(move.to_square);

    if (move.flag == EN_PASSANT)
    {
        Square ep_pawn = static_cast<Square>(move.to_square + (pos.side_to_move == WHITE ? -8 : 8));
        Piece ep_piece = pos.side_to_move == WHITE ? BP : WP;
        pos.bitboards[ep_piece] |= square_bb(ep_pawn);
    }

    if (move.flag == CASTLING)
    {
        if (move.to_square == G1)
        {
            pos.bitboards[WR] &= ~square_bb(F1);
            pos.bitboards[WR] |= square_bb(H1);
        }
        else if (move.to_square == C1)
        {
            pos.bitboards[WR] &= ~square_bb(D1);
            pos.bitboards[WR] |= square_bb(A1);
        }
        else if (move.to_square == G8)
        {
            pos.bitboards[BR] &= ~square_bb(F8);
            pos.bitboards[BR] |= square_bb(H8);
        }
        else if (move.to_square == C8)
        {
            pos.bitboards[BR] &= ~square_bb(D8);
            pos.bitboards[BR] |= square_bb(A8);
        }
    }

    pos.castling_rights = undo.castling_rights;
    pos.en_passant_square = undo.en_passant_square;
    pos.halfmove_clock = undo.halfmove_clock;
    pos.fullmove_clock = undo.fullmove_clock;

    // restore hash from undo - recompute since we restored all state
    // simplest correct approach: just recompute
    pos.hash = undo.hash; //1.14.0 fixes
}