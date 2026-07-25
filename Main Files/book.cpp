#include "book.h"
#include "uci.h"
#include "movegen.h"
#include "legal.h"
#include "makeunmake.h"
#include <cstdlib>
#include <ctime>
#include <vector>
#include <unordered_map>
#include <string>
#include <algorithm>
#include <random>

// ============================================================
// PRIORITY TIERS
//   PRIORITY_FALLBACK  (0) -> flank/sideline responses, last resort
//   PRIORITY_STANDARD  (1) -> normal mainline responses as Black
//   PRIORITY_PREFERRED (2) -> engine's primary repertoire as White
//
// probe_book() always picks the highest available tier at a
// given position, then does weighted-random within that tier.
// This means:
//   - Black response lines never fire when engine is White
//   - Preferred White lines always beat standard ones
//   - Weights let you bias toward sharper or safer variations
// ============================================================

static const int PRIORITY_FALLBACK  = 0;
static const int PRIORITY_STANDARD  = 1;
static const int PRIORITY_PREFERRED = 2;

// ============================================================
// GLOBALS
// ============================================================

static std::mt19937 rng(std::random_device{}());
bool out_of_book = false;
std::unordered_map<uint64_t, std::vector<BookMove>> opening_book;

// ============================================================
// ADD LINE
// Walks the position move by move, recording hash -> BookMove
// at every step except the last (the last move IS the response,
// so it gets stored at the second-to-last position's hash).
//
// If a hash already has a BookMove with the same UCI string,
// we keep the highest priority / weight rather than duplicating.
// ============================================================

void add_line(Position pos,
              const std::vector<std::string> &moves,
              int priority,
              int weight)
{
    for (int i = 0; i < (int)moves.size() - 1; i++)
    {
        // Check if this hash already has this UCI move recorded
        bool already_recorded = false;
        auto it = opening_book.find(pos.hash);
        if (it != opening_book.end())
        {
            for (auto &bm : it->second)
            {
                if (bm.uci == moves[i])
                {
                    // Keep the highest priority/weight seen for this move
                    bm.priority = std::max(bm.priority, priority);
                    bm.weight   = std::max(bm.weight,   weight);
                    already_recorded = true;
                    break;
                }
            }
        }

        if (!already_recorded)
            opening_book[pos.hash].push_back({moves[i], weight, priority});

        // Advance the position by making the move
        MoveList list;
        generate_moves(pos, list);
        filter_legal_moves(pos, list);

        bool found = false;
        for (int j = 0; j < list.count; j++)
        {
            Move m = list.moves[j];
            std::string uci = sq_to_uci(m.from_square) +
                              sq_to_uci(m.to_square);
            if (uci == moves[i])
            {
                UndoInfo undo;
                make_move(pos, m, undo);
                found = true;
                break;
            }
        }
        if (!found)
            break; // bad move string — stop processing this line
    }
}

// ============================================================
// INIT BOOK
// Lines are grouped by priority tier. Within each tier, weight
// controls how often a move is chosen vs its siblings at the
// same position. Higher weight = chosen more often.
//
// PRIORITY_PREFERRED (2) — White's primary repertoire
// PRIORITY_STANDARD  (1) — Black responses to common White setups
// PRIORITY_FALLBACK  (0) — responses to rare/flank openings
// ============================================================

void init_book(const Position &start_pos)
{
    static std::mt19937 rng(std::random_device{}());

    // ============================================================
    // PRIORITY_PREFERRED — White's primary repertoire
    // These always fire when the engine is White because no Black
    // response line can reach priority 2.
    // ============================================================

    // ---------------------------------------------------------------
    // e4 — Ruy Lopez (main preferred system after 1.e4)
    // ---------------------------------------------------------------

    // Ruy Lopez — Closed (weight 15 — main workhorse)
    add_line(start_pos, {"e2e4", "e7e5", "g1f3", "b8c6", "f1b5",
                         "a7a6", "b5a4", "g8f6", "e1g1", "f8e7",
                         "f1e1", "b7b5", "a4b3", "d7d6", "c2c3", "e8g8"},
             PRIORITY_PREFERRED, 15);

    // Ruy Lopez — Closed, Chigorin (…Na5 …c5)
    add_line(start_pos, {"e2e4", "e7e5", "g1f3", "b8c6", "f1b5",
                         "a7a6", "b5a4", "g8f6", "e1g1", "f8e7",
                         "f1e1", "b7b5", "a4b3", "d7d6", "c2c3", "e8g8",
                         "h2h3", "c6a5", "b3c2", "c7c5", "d2d4"},
             PRIORITY_PREFERRED, 15);

    // Ruy Lopez — Exchange (vs …a6, trade on c6)
    add_line(start_pos, {"e2e4", "e7e5", "g1f3", "b8c6", "f1b5",
                         "a7a6", "b5c6", "d7c6", "e1g1", "f7f6",
                         "d2d4", "e5d4", "f3d4", "c8d7", "b1c3"},
             PRIORITY_PREFERRED, 8);

    // Ruy Lopez — Berlin (solid, endgame-oriented)
    add_line(start_pos, {"e2e4", "e7e5", "g1f3", "b8c6", "f1b5",
                         "g8f6", "e1g1", "f6e4", "d2d4", "e4d6",
                         "b5c6", "d7c6", "d4e5", "d6f5", "d1d8",
                         "e8d8", "b1c3"},
             PRIORITY_PREFERRED, 10);

    // Ruy Lopez — Archangel (…Bb4 aggressive)
    add_line(start_pos, {"e2e4", "e7e5", "g1f3", "b8c6", "f1b5",
                         "a7a6", "b5a4", "g8f6", "e1g1", "f8b4",
                         "f1e1", "b7b5", "a4b3", "e8g8", "c2c3", "d7d5"},
             PRIORITY_PREFERRED, 8);

    // ---------------------------------------------------------------
    // e4 — Italian (secondary e4 system)
    // ---------------------------------------------------------------

    // Italian — Giuoco Piano
    add_line(start_pos, {"e2e4", "e7e5", "g1f3", "b8c6", "f1c4",
                         "f8c5", "c2c3", "g8f6", "d2d4", "e5d4",
                         "c3d4", "c5b4", "c1d2", "b4d2", "b1d2", "d7d5"},
             PRIORITY_PREFERRED, 10);

    // Italian — Two Knights solid
    add_line(start_pos, {"e2e4", "e7e5", "g1f3", "b8c6", "f1c4",
                         "g8f6", "d2d3", "f8c5", "c2c3", "d7d6",
                         "e1g1", "e8g8", "b1d2"},
             PRIORITY_PREFERRED, 8);

    // ---------------------------------------------------------------
    // e4 — Scotch (tertiary e4 system for variety)
    // ---------------------------------------------------------------

    // Scotch — Classical (…Bc5)
    add_line(start_pos, {"e2e4", "e7e5", "g1f3", "b8c6", "d2d4",
                         "e5d4", "f3d4", "f8c5", "c1e3", "d8f6",
                         "c2c3", "g8e7", "f1c4", "e8g8", "e1g1"},
             PRIORITY_PREFERRED, 7);

    // Scotch — Mieses (…Nf6)
    add_line(start_pos, {"e2e4", "e7e5", "g1f3", "b8c6", "d2d4",
                         "e5d4", "f3d4", "g8f6", "d4c6", "b7c6",
                         "e4e5", "d8e7", "d1e2", "f6d5", "c2c4",
                         "d5b6", "b1c3"},
             PRIORITY_PREFERRED, 7);

    // ---------------------------------------------------------------
    // e4 — Four Knights
    // ---------------------------------------------------------------

    add_line(start_pos, {"e2e4", "e7e5", "g1f3", "b8c6", "b1c3",
                         "g8f6", "f1b5", "f8b4", "e1g1", "e8g8",
                         "d2d3", "d7d6", "c1g5", "b4c3", "b2c3", "d8e7"},
             PRIORITY_PREFERRED, 8);

    // ---------------------------------------------------------------
    // d4 — London System (main d4 preferred system)
    // ---------------------------------------------------------------

    // London — vs …d5 …Nf6
    add_line(start_pos, {"d2d4", "d7d5", "g1f3", "g8f6", "c1f4",
                         "e7e6", "e2e3", "f8d6", "f4d6", "d8d6",
                         "f1d3", "e8g8", "e1g1", "b8d7", "b1d2",
                         "b7b6", "c2c3"},
             PRIORITY_PREFERRED, 15);

    // London — vs …c5
    add_line(start_pos, {"d2d4", "d7d5", "g1f3", "g8f6", "c1f4",
                         "c7c5", "e2e3", "b8c6", "c2c3", "d8b6",
                         "d1b3", "b6b3", "a2b3", "c5d4", "c3d4",
                         "e7e6", "b1c3"},
             PRIORITY_PREFERRED, 12);

    // London — vs KID setup (…g6)
    add_line(start_pos, {"d2d4", "g8f6", "g1f3", "g7g6", "c1f4",
                         "f8g7", "e2e3", "d7d6", "h2h3", "e8g8",
                         "f1e2", "b8d7", "e1g1", "c7c5", "c2c3"},
             PRIORITY_PREFERRED, 10);

    // ---------------------------------------------------------------
    // d4 — Queen's Gambit
    // ---------------------------------------------------------------

    // QGA — Classical
    add_line(start_pos, {"d2d4", "d7d5", "c2c4", "d5c4", "g1f3",
                         "g8f6", "e2e3", "e7e6", "f1c4", "c7c5",
                         "e1g1", "a7a6", "d1e2", "b7b5", "c4d3",
                         "c5d4", "e3d4"},
             PRIORITY_PREFERRED, 12);

    // QGD — Orthodox
    add_line(start_pos, {"d2d4", "d7d5", "c2c4", "e7e6", "b1c3",
                         "g8f6", "c1g5", "f8e7", "e2e3", "e8g8",
                         "g1f3", "h7h6", "g5h4", "b7b6", "f1d3",
                         "c8b7", "e1g1"},
             PRIORITY_PREFERRED, 12);

    // QGD — Exchange
    add_line(start_pos, {"d2d4", "d7d5", "c2c4", "e7e6", "b1c3",
                         "g8f6", "c4d5", "e6d5", "c1g5", "f8e7",
                         "e2e3", "c7c6", "g1f3", "b8d7", "f1d3",
                         "f6h5", "g5e7", "d8e7", "e1g1"},
             PRIORITY_PREFERRED, 8);

    // Semi-Slav — Meran
    add_line(start_pos, {"d2d4", "d7d5", "c2c4", "e7e6", "b1c3",
                         "g8f6", "g1f3", "c7c6", "e2e3", "b8d7",
                         "f1d3", "d5c4", "d3c4", "b7b5", "c4d3",
                         "c8b7", "e1g1", "b5b4"},
             PRIORITY_PREFERRED, 10);

    // ---------------------------------------------------------------
    // d4 — Catalan
    // ---------------------------------------------------------------

    // Catalan — Open (…dxc4)
    add_line(start_pos, {"d2d4", "d7d5", "c2c4", "e7e6", "g1f3",
                         "g8f6", "g2g3", "d5c4", "f1g2", "f8e7",
                         "e1g1", "e8g8", "d1c2", "a7a6", "a2a4",
                         "b8d7"},
             PRIORITY_PREFERRED, 10);

    // Catalan — Closed
    add_line(start_pos, {"d2d4", "d7d5", "c2c4", "e7e6", "g1f3",
                         "g8f6", "g2g3", "f8e7", "f1g2", "e8g8",
                         "e1g1", "b8d7", "d1c2", "c7c6", "f1d1",
                         "a7a6", "b1c3"},
             PRIORITY_PREFERRED, 10);

    // ---------------------------------------------------------------
    // d4 — Colle System
    // ---------------------------------------------------------------

    // Colle — Zukertort
    add_line(start_pos, {"d2d4", "d7d5", "g1f3", "g8f6", "e2e3",
                         "e7e6", "f1d3", "f8d6", "e1g1", "e8g8",
                         "b1d2", "b8d7", "f1e1", "c7c6", "e3e4"},
             PRIORITY_PREFERRED, 8);

    // Colle — vs …b6 fianchetto
    add_line(start_pos, {"d2d4", "g8f6", "g1f3", "e7e6", "e2e3",
                         "b7b6", "f1d3", "c8b7", "e1g1", "f8e7",
                         "b1d2", "d7d5", "c2c3", "e8g8", "d1e2"},
             PRIORITY_PREFERRED, 7);

    // ---------------------------------------------------------------
    // d4 — Slav Defence responses
    // ---------------------------------------------------------------

    // Slav — Main Line (…dxc4 …Bf5)
    add_line(start_pos, {"d2d4", "d7d5", "c2c4", "c7c6", "g1f3",
                         "g8f6", "b1c3", "d5c4", "a2a4", "c8f5",
                         "e2e3", "e7e6", "f1c4", "f8b4", "e1g1",
                         "e8g8", "d1e2"},
             PRIORITY_PREFERRED, 10);

    // Slav — Chebanenko (…a6)
    add_line(start_pos, {"d2d4", "d7d5", "c2c4", "c7c6", "g1f3",
                         "g8f6", "b1c3", "a7a6", "e2e3", "e7e6",
                         "f1d3", "d5c4", "d3c4", "b7b5", "c4d3"},
             PRIORITY_PREFERRED, 8);

    // ---------------------------------------------------------------
    // c4 — English Opening
    // ---------------------------------------------------------------

    // English — Symmetrical (…c5)
    add_line(start_pos, {"c2c4", "c7c5", "b1c3", "b8c6", "g1f3",
                         "g8f6", "g2g3", "g7g6", "f1g2", "f8g7",
                         "e1g1", "e8g8", "d2d4", "c5d4", "f3d4"},
             PRIORITY_PREFERRED, 10);

    // English — reversed Sicilian (…e5)
    add_line(start_pos, {"c2c4", "e7e5", "b1c3", "g8f6", "g1f3",
                         "b8c6", "g2g3", "f8b4", "f1g2", "e8g8",
                         "e1g1", "e5e4", "f3g5", "b4c3", "d2c3", "h7h6"},
             PRIORITY_PREFERRED, 8);

    // English — vs Hedgehog (…Nf6 …e6 …b6)
    add_line(start_pos, {"c2c4", "g8f6", "b1c3", "e7e6", "g1f3",
                         "b7b6", "g2g3", "c8b7", "f1g2", "f8e7",
                         "e1g1", "e8g8", "d2d4", "d7d6", "b2b3"},
             PRIORITY_PREFERRED, 7);

    // ---------------------------------------------------------------
    // Nf3 — King's Indian Attack
    // ---------------------------------------------------------------

    add_line(start_pos, {"g1f3", "d7d5", "g2g3", "g8f6", "f1g2",
                         "e7e6", "e1g1", "f8e7", "d2d3", "e8g8",
                         "b1d2", "c7c5", "e2e4", "b8c6", "f1e1"},
             PRIORITY_PREFERRED, 8);

    // ============================================================
    // PRIORITY_STANDARD — Black responses to common White setups
    // These fire when the engine is Black. Priority 1 means they
    // never conflict with preferred White lines (priority 2).
    // ============================================================

    // ---------------------------------------------------------------
    // vs 1.e4 — Sicilian Defence
    // ---------------------------------------------------------------

    // Sicilian — Najdorf (main Black weapon vs e4)
    add_line(start_pos, {"e2e4", "c7c5", "g1f3", "d7d6", "d2d4",
                         "c5d4", "f3d4", "g8f6", "b1c3", "a7a6",
                         "f1e2", "e7e5", "d4b3", "f8e7", "e1g1",
                         "e8g8", "c1e3"},
             PRIORITY_STANDARD, 15);

    // Sicilian — Najdorf, English Attack
    add_line(start_pos, {"e2e4", "c7c5", "g1f3", "d7d6", "d2d4",
                         "c5d4", "f3d4", "g8f6", "b1c3", "a7a6",
                         "c1e3", "c8g4", "f2f3", "g4h5", "d1d2",
                         "e7e5", "d4b3", "f8e7", "e1g1"},
             PRIORITY_STANDARD, 12);

    // Sicilian — Dragon
    add_line(start_pos, {"e2e4", "c7c5", "g1f3", "d7d6", "d2d4",
                         "c5d4", "f3d4", "g8f6", "b1c3", "g7g6",
                         "c1e3", "f8g7", "f2f3", "e8g8", "d1d2",
                         "b8c6", "e1c1"},
             PRIORITY_STANDARD, 10);

    // Sicilian — Scheveningen
    add_line(start_pos, {"e2e4", "c7c5", "g1f3", "d7d6", "d2d4",
                         "c5d4", "f3d4", "g8f6", "b1c3", "e7e6",
                         "f1e2", "f8e7", "e1g1", "e8g8", "f2f4",
                         "b8c6", "c1e3"},
             PRIORITY_STANDARD, 10);

    // Sicilian — Classical (…Nc6)
    add_line(start_pos, {"e2e4", "c7c5", "g1f3", "b8c6", "d2d4",
                         "c5d4", "f3d4", "g8f6", "b1c3", "d7d6",
                         "f1e2", "e7e5", "d4b3", "f8e7", "e1g1",
                         "e8g8"},
             PRIORITY_STANDARD, 10);

    // Sicilian — Kan / Taimanov
    add_line(start_pos, {"e2e4", "c7c5", "g1f3", "e7e6", "d2d4",
                         "c5d4", "f3d4", "a7a6", "b1c3", "d8c7",
                         "f1d3", "b8c6", "d4b3", "g8f6", "e1g1",
                         "f8e7", "c1e3"},
             PRIORITY_STANDARD, 10);

    // ---------------------------------------------------------------
    // vs 1.e4 — French Defence
    // ---------------------------------------------------------------

    // French — Classical (…Nf6)
    add_line(start_pos, {"e2e4", "e7e6", "d2d4", "d7d5", "b1c3",
                         "g8f6", "c1g5", "f8e7", "e4e5", "f6d7",
                         "g5e7", "d8e7", "f2f4", "a7a5", "d1d2",
                         "c7c5"},
             PRIORITY_STANDARD, 10);

    // French — Winawer (…Bb4)
    add_line(start_pos, {"e2e4", "e7e6", "d2d4", "d7d5", "b1c3",
                         "f8b4", "e4e5", "c7c5", "a2a3", "b4c3",
                         "b2c3", "g8e7", "d1g4", "e8g8", "g4d1",
                         "b8c6", "g1f3"},
             PRIORITY_STANDARD, 10);

    // French — Tarrasch (vs 3.Nd2)
    add_line(start_pos, {"e2e4", "e7e6", "d2d4", "d7d5", "b1d2",
                         "g8f6", "e4e5", "f6d7", "f1d3", "c7c5",
                         "c2c3", "b8c6", "g1e2", "c5d4", "c3d4",
                         "f7f6", "e5f6", "d7f6"},
             PRIORITY_STANDARD, 8);

    // French — Exchange (3.exd5)
    add_line(start_pos, {"e2e4", "e7e6", "d2d4", "d7d5", "e4d5",
                         "e6d5", "g1f3", "g8f6", "f1d3", "f8d6",
                         "e1g1", "e8g8", "h2h3", "b8c6", "c1f4",
                         "d6f4", "d1f4"},
             PRIORITY_STANDARD, 6);

    // ---------------------------------------------------------------
    // vs 1.e4 — Caro-Kann Defence
    // ---------------------------------------------------------------

    // Caro-Kann — Classical (…Bf5)
    add_line(start_pos, {"e2e4", "c7c6", "d2d4", "d7d5", "b1c3",
                         "d5e4", "c3e4", "c8f5", "e4g3", "f5g6",
                         "h2h4", "h7h6", "g1f3", "g8f6", "h4h5",
                         "g6h7", "f1d3", "h7d3", "d1d3"},
             PRIORITY_STANDARD, 10);

    // Caro-Kann — Advance (3.e5)
    add_line(start_pos, {"e2e4", "c7c6", "d2d4", "d7d5", "e4e5",
                         "c8f5", "g1f3", "e7e6", "f1e2", "g8e7",
                         "e1g1", "b8d7", "b1d2", "h7h6", "d1b3",
                         "d8c7", "c1e3"},
             PRIORITY_STANDARD, 10);

    // Caro-Kann — Panov Attack (4.c4)
    add_line(start_pos, {"e2e4", "c7c6", "d2d4", "d7d5", "e4d5",
                         "c6d5", "c2c4", "g8f6", "b1c3", "e7e6",
                         "g1f3", "f8e7", "c4d5", "e6d5", "f1b5",
                         "b8c6", "e1g1"},
             PRIORITY_STANDARD, 8);

    // ---------------------------------------------------------------
    // vs 1.e4 — Pirc / Modern
    // ---------------------------------------------------------------

    // Pirc — Classical System
    add_line(start_pos, {"e2e4", "d7d6", "d2d4", "g8f6", "b1c3",
                         "g7g6", "g1f3", "f8g7", "f1e2", "e8g8",
                         "e1g1", "c7c6", "a2a4", "b8d7", "h2h3"},
             PRIORITY_STANDARD, 8);

    // Pirc — Austrian Attack (f4)
    add_line(start_pos, {"e2e4", "d7d6", "d2d4", "g8f6", "b1c3",
                         "g7g6", "f2f4", "f8g7", "g1f3", "c7c5",
                         "d4d5", "e8g8", "f1e2", "e7e6", "e1g1",
                         "e6d5", "e4d5"},
             PRIORITY_STANDARD, 6);

    // ---------------------------------------------------------------
    // vs 1.e4 e5 — King's Gambit Declined (solid response)
    // ---------------------------------------------------------------

    add_line(start_pos, {"e2e4", "e7e5", "f2f4", "d7d5", "e4d5",
                         "e5e4", "d2d3", "g8f6", "d3e4", "f6e4",
                         "g1f3", "f8c5", "d1e2", "d8e7", "c1e3"},
             PRIORITY_STANDARD, 8);

    // ---------------------------------------------------------------
    // vs 1.d4 — King's Indian Defence
    // ---------------------------------------------------------------

    // KID — Classical (…e5)
    add_line(start_pos, {"d2d4", "g8f6", "c2c4", "g7g6", "b1c3",
                         "f8g7", "e2e4", "d7d6", "g1f3", "e8g8",
                         "f1e2", "e7e5", "e1g1", "b8c6", "d4d5",
                         "c6e7", "f3e1", "f6d7", "b2b4"},
             PRIORITY_STANDARD, 12);

    // KID — Samisch
    add_line(start_pos, {"d2d4", "g8f6", "c2c4", "g7g6", "b1c3",
                         "f8g7", "e2e4", "d7d6", "f2f3", "e8g8",
                         "c1e3", "e7e5", "d4d5", "b8d7", "d1d2",
                         "f6h5", "e1c1", "f7f5"},
             PRIORITY_STANDARD, 8);

    // KID — Four Pawns Attack
    add_line(start_pos, {"d2d4", "g8f6", "c2c4", "g7g6", "b1c3",
                         "f8g7", "e2e4", "d7d6", "f2f4", "e8g8",
                         "g1f3", "c7c5", "d4d5", "e7e6", "f1e2",
                         "e6d5", "c4d5"},
             PRIORITY_STANDARD, 6);

    // ---------------------------------------------------------------
    // vs 1.d4 — Nimzo-Indian Defence
    // ---------------------------------------------------------------

    // Nimzo — Rubinstein (4.e3)
    add_line(start_pos, {"d2d4", "g8f6", "c2c4", "e7e6", "b1c3",
                         "f8b4", "e2e3", "e8g8", "f1d3", "d7d5",
                         "g1f3", "c7c5", "e1g1", "d5c4", "d3c4",
                         "b8d7", "d1d3"},
             PRIORITY_STANDARD, 12);

    // Nimzo — Classical (4.Qc2)
    add_line(start_pos, {"d2d4", "g8f6", "c2c4", "e7e6", "b1c3",
                         "f8b4", "d1c2", "e8g8", "g1f3", "c7c5",
                         "d4d5", "e6d5", "c4d5", "b8a6", "e2e4",
                         "b4a5", "f1e2"},
             PRIORITY_STANDARD, 10);

    // Nimzo — Leningrad (4.Bg5)
    add_line(start_pos, {"d2d4", "g8f6", "c2c4", "e7e6", "b1c3",
                         "f8b4", "c1g5", "h7h6", "g5h4", "c7c5",
                         "d4d5", "d7d6", "e2e3", "b4c3", "b2c3",
                         "e6d5", "c4d5"},
             PRIORITY_STANDARD, 8);

    // ---------------------------------------------------------------
    // vs 1.d4 — Queen's Indian Defence
    // ---------------------------------------------------------------

    // QID — Main Line (4.g3 …Ba6)
    add_line(start_pos, {"d2d4", "g8f6", "c2c4", "e7e6", "g1f3",
                         "b7b6", "g2g3", "c8a6", "b2b3", "f8b4",
                         "c1d2", "b4e7", "f1g2", "c7c6", "e1g1",
                         "d7d5", "d1c2"},
             PRIORITY_STANDARD, 10);

    // QID — Petrosian (4.a3)
    add_line(start_pos, {"d2d4", "g8f6", "c2c4", "e7e6", "g1f3",
                         "b7b6", "a2a3", "c8a6", "d1c2", "a6b7",
                         "b1c3", "c7c5", "e2e4", "c5d4", "f3d4",
                         "b8c6", "d4c6"},
             PRIORITY_STANDARD, 8);

    // ---------------------------------------------------------------
    // vs 1.d4 — Grunfeld Defence
    // ---------------------------------------------------------------

    // Grunfeld — Exchange (5.e4)
    add_line(start_pos, {"d2d4", "g8f6", "c2c4", "g7g6", "b1c3",
                         "d7d5", "c4d5", "f6d5", "e2e4", "d5c3",
                         "b2c3", "f8g7", "g1f3", "c7c5", "f1e2",
                         "e8g8", "e1g1", "c5d4", "c3d4"},
             PRIORITY_STANDARD, 10);

    // Grunfeld — Russian System (7.Qb3)
    add_line(start_pos, {"d2d4", "g8f6", "c2c4", "g7g6", "b1c3",
                         "d7d5", "g1f3", "f8g7", "d1b3", "d5c4",
                         "b3c4", "e8g8", "e2e4", "c8g4", "c1e3",
                         "b8d7", "f1e2"},
             PRIORITY_STANDARD, 8);

    // ---------------------------------------------------------------
    // vs 1.d4 — Dutch Defence
    // ---------------------------------------------------------------

    // Dutch — Leningrad (…f5 …g6)
    add_line(start_pos, {"d2d4", "f7f5", "g1f3", "g8f6", "g2g3",
                         "g7g6", "f1g2", "f8g7", "e1g1", "e8g8",
                         "c2c4", "d7d6", "b1c3", "d8e8", "d1b3",
                         "c7c6", "c1f4"},
             PRIORITY_STANDARD, 8);

    // Dutch — Stonewall (…d5 …e6 …f5)
    add_line(start_pos, {"d2d4", "f7f5", "g1f3", "g8f6", "g2g3",
                         "e7e6", "f1g2", "d7d5", "e1g1", "f8d6",
                         "c2c4", "c7c6", "b2b3", "d8e7", "c1a3",
                         "d6a3", "b1a3"},
             PRIORITY_STANDARD, 6);

    // ============================================================
    // PRIORITY_FALLBACK — responses to flank/rare openings
    // Only fires if no standard or preferred move exists at that hash.
    // ============================================================

    // vs English (1.c4) — …e5 reversed Sicilian
    add_line(start_pos, {"c2c4", "e7e5", "b1c3", "g8f6", "g1f3",
                         "b8c6", "g2g3", "f8b4", "f1g2", "e8g8",
                         "e1g1", "d7d6", "d2d3", "c8f5", "e2e4"},
             PRIORITY_FALLBACK, 10);

    // vs English (1.c4) — …c5 symmetrical
    add_line(start_pos, {"c2c4", "c7c5", "b1c3", "b8c6", "g2g3",
                         "g7g6", "f1g2", "f8g7", "g1f3", "g8f6",
                         "e1g1", "e8g8", "d2d4", "c5d4", "f3d4",
                         "d7d6"},
             PRIORITY_FALLBACK, 10);

    // vs KIA (1.Nf3 2.g3) — French-like setup
    add_line(start_pos, {"g1f3", "d7d5", "g2g3", "c7c5", "f1g2",
                         "b8c6", "e1g1", "e7e5", "d2d3", "g8f6",
                         "b1d2", "f8e7", "e2e4", "d5d4", "a2a3"},
             PRIORITY_FALLBACK, 8);

    // vs Bird's (1.f4) — solid …d5 …Nf6
    add_line(start_pos, {"f2f4", "d7d5", "g1f3", "g8f6", "e2e3",
                         "g7g6", "f1e2", "f8g7", "e1g1", "e8g8",
                         "d2d3", "c7c5", "d1e1", "b8c6", "c2c3"},
             PRIORITY_FALLBACK, 8);
}

// ============================================================
// PROBE BOOK
// 1. Find all BookMoves at this position hash.
// 2. Select only those at the highest available priority tier.
// 3. Do a weighted-random draw within that tier.
//
// This guarantees:
//   - Preferred White lines always beat standard Black responses
//   - Fallback lines only fire when nothing better exists
//   - Variety within a tier prevents trivial preparation by opponents
// ============================================================

Move probe_book(const Position &pos)
{
    auto it = opening_book.find(pos.hash);
    if (it == opening_book.end())
        return {NO_SQUARE, NO_SQUARE, NO_PIECE_TYPE, NORMAL};

    const auto &entries = it->second;

    // Step 1 — find highest priority tier available
    int best_priority = -1;
    for (const auto &bm : entries)
        best_priority = std::max(best_priority, bm.priority);

    // Step 2 — collect candidates at that tier and sum weights
    int total_weight = 0;
    for (const auto &bm : entries)
        if (bm.priority == best_priority)
            total_weight += bm.weight;

    if (total_weight <= 0)
        return {NO_SQUARE, NO_SQUARE, NO_PIECE_TYPE, NORMAL};

    // Step 3 — weighted random draw
    int roll = std::uniform_int_distribution<int>(0, total_weight - 1)(rng);
    int cumulative = 0;
    std::string chosen_uci;

    for (const auto &bm : entries)
    {
        if (bm.priority != best_priority)
            continue;
        cumulative += bm.weight;
        if (roll < cumulative)
        {
            chosen_uci = bm.uci;
            break;
        }
    }

    // Step 4 — convert UCI string to a legal Move object
    MoveList list;
    generate_moves(const_cast<Position &>(pos), list);
    filter_legal_moves(const_cast<Position &>(pos), list);

    for (int i = 0; i < list.count; i++)
    {
        Move m = list.moves[i];
        std::string m_uci = sq_to_uci(m.from_square) +
                            sq_to_uci(m.to_square);
        if (m_uci == chosen_uci)
            return m;
    }

    // Move was in book but not legal — fail safe
    return {NO_SQUARE, NO_SQUARE, NO_PIECE_TYPE, NORMAL};
}