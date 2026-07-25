#include "search.h"
#include "transposition.h"
#include "book.h"
#include <iostream>
#include <algorithm>
#include <chrono>

extern uint64_t side_key;

Move killer_moves[MAX_DEPTH][2];
int history_table[64][64];
int g_last_depth = 0;

std::atomic<bool> stop_search(false);
Move ponder_move = {NO_SQUARE, NO_SQUARE, NO_PIECE_TYPE, NORMAL}; // 1.15.0.0 pondering
Move ponder_best_so_far = {NO_SQUARE, NO_SQUARE, NO_PIECE_TYPE, NORMAL};

// Resets all killer move slots to empty (NO_SQUARE) at the start of a new search.
// Killer moves from a previous search are no longer valid for the new position's depth table.
void clear_killers()
{
    for (int i = 0; i < MAX_DEPTH; i++)
    {
        killer_moves[i][0] = {NO_SQUARE, NO_SQUARE, NO_PIECE_TYPE, NORMAL};
        killer_moves[i][1] = {NO_SQUARE, NO_SQUARE, NO_PIECE_TYPE, NORMAL};
    }
}

// Zeros out the entire history heuristic table (from-square x to-square).
// Must be called before each new search so stale scores from prior positions don't
// pollute move ordering.
void clear_history()
{
    for (int i = 0; i < 64; i++)
        for (int j = 0; j < 64; j++)
            history_table[i][j] = 0;
}

// ============================================================
// TIME MANAGEMENT
// ============================================================

// Determines how many milliseconds to budget for the current move given the
// remaining clock time and increment. Uses a ~30-moves-remaining heuristic and
// caps the result so a single move can never consume more than 10% of the clock,
// with a hard floor of 50ms to avoid degenerate instant moves.
int calculate_time(int remaining_ms, int increment_ms)
{
    // ~30 moves remaining assumption, use most of the increment
    int time_for_move = (remaining_ms / 30) + (increment_ms * 3 / 4);

    // Never use more than 10% of remaining time on one move
    int max_single_move = remaining_ms / 10;
    if (time_for_move > max_single_move)
        time_for_move = max_single_move;

    // Hard floor: always think at least 50ms to avoid instant moves
    if (time_for_move < 50)
        time_for_move = 50;

    return time_for_move;
}

// ============================================================
// QUIESCENCE SEARCH
// ============================================================

// Extends the search beyond the horizon by evaluating only captures (and en passant)
// until the position is "quiet". This prevents the horizon effect where a bad capture
// just outside the main search depth looks artificially good because the recapture
// is never seen. Uses a stand-pat score as a lower bound: if the static eval already
// beats beta, we can return immediately (the opponent wouldn't allow this position)
int quiescence(Position &pos, int alpha, int beta, int depth)
{
    // Hard depth limit to prevent runaway q-search in crazy positions
    if (depth <= -12)
        return evaluate(pos);

    int stand_pat = evaluate(pos);

    if (stand_pat >= beta)
        return beta;
    if (stand_pat > alpha)
        alpha = stand_pat;

    MoveList list;
    generate_moves(pos, list);
    filter_legal_moves(pos, list);
    order_moves(pos, list, depth);

    for (int i = 0; i < list.count; i++)
    {
        // Only search captures and en passant
        if (piece_on(pos, list.moves[i].to_square) == NO_PIECE &&
            list.moves[i].flag != EN_PASSANT)
            continue;

        UndoInfo undo;
        make_move(pos, list.moves[i], undo);
        int score = -quiescence(pos, -beta, -alpha, depth - 1);
        unmake_move(pos, list.moves[i], undo);

        if (score >= beta)
            return beta;
        if (score > alpha)
            alpha = score;
    }

    return alpha;
}

// Core recursive alpha-beta search with several enhancements:
// transposition table (TT) probing, null-move pruning, reverse futility pruning,
// forward futility pruning, Late Move Reductions (LMR), and Principal Variation
// Search (PVS). Returns the score of the position from the perspective of the
// side to move. Alpha is the best score the maximising side can already guarantee;
// beta is the best score the minimising side can already guarantee. A score >= beta
// causes an immediate cutoff (the opponent won't allow this line).
int alphaBeta(Position &pos, int depth, int alpha, int beta,
              uint64_t *stack, int stack_size, bool null_allowed)
{
    uint64_t hash = pos.hash;
    // A PV node has a window wider than 1 (alpha+1 < beta). Zero-window calls
    // (alpha == beta-1) are used for LMR/PVS probes and must not be treated as PV
    // nodes, otherwise we'd skip TT cutoffs on every reduced search.
    bool is_pv = (beta - alpha > 1);

    // ---- Repetition: always 0, no clever scoring ----
    for (int i = 0; i < stack_size - 1; i++)
        if (stack[i] == hash)
        {
            return 0;
        }

    // ---- TT probe ----
    // Skip TT cutoffs on PV nodes so we don't collapse the PV line.
    TTEntry *entry = tt.probe(hash);
    if (entry != nullptr && entry->depth >= depth && !is_pv)
    {
        if (entry->flag == TT_EXACT)
            return entry->score;
        if (entry->flag == TT_ALPHA && entry->score <= alpha)
            return alpha;
        if (entry->flag == TT_BETA && entry->score >= beta)
            return beta;
    }

    // ---- Leaf node ----
    if (depth == 0)
        return quiescence(pos, alpha, beta, depth);

    bool in_check = is_in_check(pos, pos.side_to_move);

    // ---- Static eval (computed once, reused by both futility checks) ----
    // Don't compute if in check. Eval will be unreliable when the king is attacked.
    int static_eval = (!in_check) ? evaluate(pos) : -INF;

    // ---- Reverse futility pruning (static null move) ----
    // If static eval is so far above beta that even a "bad" move won't fall
    // below it, we can safely return early. Only at shallow depths, not in
    // check, not on PV nodes.
    if (!is_pv && !in_check && depth <= 3)
    {
        int rfp_margin = 120 * depth; // 120cp per depth: 120/240/360
        if (static_eval - rfp_margin >= beta)
            return static_eval;
    }

    //---- Null Move Pruning ----
    // If we can skip our turn entirely and the opponent still can't beat beta
    // at a reduced depth, the position is so good that a real move will also
    // beat beta — return early. Reduction R=2 means we search depth-3.
    // Disabled in check (zugzwang risk is high), on PV nodes, and when the
    // calling node already used a null move (null_allowed=false prevents
    // consecutive null moves which would be unsound).
    const int R = 2;
    if (null_allowed && !is_pv && !in_check && depth >= R + 1)
    {
        Square old_ep = pos.en_passant_square;
        uint64_t old_hash = pos.hash;

        if (old_ep != NO_SQUARE)
            pos.hash ^= en_passant_keys[old_ep % 8];
        pos.hash ^= side_key;

        pos.en_passant_square = NO_SQUARE;
        pos.side_to_move = (pos.side_to_move == WHITE) ? BLACK : WHITE;

        stack[stack_size] = pos.hash;
        int null_score = -alphaBeta(pos, depth - R - 1, -beta, -beta + 1,
                                    stack, stack_size + 1, false);

        pos.side_to_move = (pos.side_to_move == WHITE) ? BLACK : WHITE;
        pos.en_passant_square = old_ep;
        pos.hash = old_hash;

        if (null_score >= beta)
            return beta;
    }

    // ---- Move generation ----
    MoveList list;
    generate_moves(pos, list);
    filter_legal_moves(pos, list);

    // Seed search with TT best move if available
    if (entry != nullptr)
    {
        for (int i = 0; i < list.count; i++)
        {
            if (list.moves[i].from_square == entry->best.from_square &&
                list.moves[i].to_square == entry->best.to_square)
            {
                Move temp = list.moves[0];
                list.moves[0] = list.moves[i];
                list.moves[i] = temp;
                break;
            }
        }
    }
    order_moves(pos, list, depth);

    // ---- Terminal ----
    // No legal moves: if in check it's checkmate (score is -INF adjusted by depth
    // so shallower mates are preferred), otherwise it's stalemate (score 0).
    // Using depth in the mate score means the engine will choose the fastest mate
    // and avoid the slowest loss.
    if (list.count == 0)
        return in_check ? -INF + depth : 0;

    // ---- Normal futility pruning (setup) ----
    // At depth 1/2/3, if static eval is so far below alpha that even a good
    // quiet move is unlikely to raise it, skip quiet moves.
    // Margins: 150cp at d1, 300cp at d2, 450cp at d3.
    bool do_futility = (!in_check && !is_pv && depth <= 3);
    int futility_margin = 150 * depth;

    TTFlag tt_flag = TT_ALPHA;
    Move best_found = list.moves[0];
    int moves_searched = 0;

    //---- PVS (Principal Variation Search) ----------
    // The first move is searched with the full [alpha, beta] window. All subsequent
    // moves are first searched with a zero window (-alpha-1, -alpha) to quickly
    // test if they can beat alpha. If one does (a "re-search" is needed), we
    // re-search with the full window. This is sound because if we've ordered moves
    // well, later moves are unlikely to be best — the zero-window probe is usually
    // a cheap refutation.

    for (int i = 0; i < list.count; i++)
    {
        // Define BEFORE make_move
        bool is_capture = (piece_on(pos, list.moves[i].to_square) != NO_PIECE ||
                           list.moves[i].flag == EN_PASSANT);
        bool is_quiet = !is_capture && (list.moves[i].flag != PROMOTION);

        // Check if this is a killer move — don't reduce these
        bool is_killer = (list.moves[i].from_square == killer_moves[depth][0].from_square &&
                          list.moves[i].to_square == killer_moves[depth][0].to_square) ||
                         (list.moves[i].from_square == killer_moves[depth][1].from_square &&
                          list.moves[i].to_square == killer_moves[depth][1].to_square);

        // ---- Normal futility pruning (execution) ----
        // Skip quiet, non-killer moves that have no realistic chance of
        // raising alpha. Always search at least the first move.
        if (do_futility && moves_searched > 0 && is_quiet && !is_killer &&
            static_eval + futility_margin <= alpha)
        {
            continue;
        }

        UndoInfo undo;
        make_move(pos, list.moves[i], undo);
        stack[stack_size] = pos.hash;

        // Define AFTER make_move (need to see resulting position)
        bool gives_check = is_in_check(pos, pos.side_to_move);

        // ---- Don't futility-prune moves that give check ----
        // We already made the move above, so patch up: if it gives check and
        // we would have pruned it, un-prune by just searching it normally.
        // (The continue above happens before make_move, so this is fine.)

        int score;

        if (moves_searched == 0)
        {
            score = -alphaBeta(pos, depth - 1, -beta, -alpha,
                               stack, stack_size + 1, true);
        }
        else
        {
            // ---- Late Move Reductions (LMR) ----
            // Quiet, non-killer moves searched late (moves_searched >= 4) are
            // unlikely to be best if our move ordering is good. We reduce their
            // search depth by 1 (or 2 for very late moves) and search with a
            // zero window. If the reduced search raises alpha, we re-search at
            // full depth to confirm. Disabled in check, for killers, and for
            // captures/promotions which must be examined carefully.
            int reduction = 0;
            if (depth >= 3 && moves_searched >= 4 && is_quiet && !gives_check && !in_check && !is_killer)
            {
                reduction = 1;
                if (moves_searched >= 8)
                    reduction = 2;
            }

            score = -alphaBeta(pos, depth - 1 - reduction, -alpha - 1, -alpha,
                               stack, stack_size + 1, true);

            if (reduction > 0 && score > alpha)
            {
                score = -alphaBeta(pos, depth - 1, -alpha - 1, -alpha,
                                   stack, stack_size + 1, true);
            }

            if (score > alpha && score < beta)
            {
                score = -alphaBeta(pos, depth - 1, -beta, -alpha,
                                   stack, stack_size + 1, true);
            }
        }

        unmake_move(pos, list.moves[i], undo);
        moves_searched++;

        if (score >= beta)
        {
            // Only update killers and history for quiet moves — captures are
            // already ordered by MVV-LVA, so boosting them here would be redundant
            // and would dilute the signal for genuinely good quiet moves.
            if (is_quiet)
            {
                // Update killers
                if (depth >= 0 && depth < MAX_DEPTH)
                {
                    killer_moves[depth][1] = killer_moves[depth][0];
                    killer_moves[depth][0] = list.moves[i];
                }

                // Update history
                // Score is depth² so deeper cutoffs outweigh shallow ones —
                // a move that cuts off at depth 6 is far more valuable than
                // one that cuts off at depth 1.
                history_table[list.moves[i].from_square][list.moves[i].to_square] += depth * depth;
            }

            tt.store(hash, depth, beta, list.moves[i], TT_BETA);
            return beta;
        } // 1.14.0.7

        if (score > alpha)
        {
            alpha = score;
            tt_flag = TT_EXACT;
            best_found = list.moves[i];
        }
    }

    tt.store(hash, depth, alpha, best_found, tt_flag);
    return alpha;
}

// Top-level iterative deepening search. Searches from depth 1 up to max_depth,
// returning the best move found within the allotted time. Uses aspiration windows
// to narrow the alpha-beta window around the previous depth's score, dramatically
// cutting the number of nodes searched. If the score falls outside the window, it
// widens and re-searches. The best move from each completed depth is used to seed
// move ordering for the next depth (the "killer from previous iteration" trick).
// Also handles opening book lookup and pondering support.
Move best_move(Position &pos, int max_depth,
               uint64_t *game_history, int game_history_count,
               int remaining_ms, int increment_ms, bool skip_book)
{

    if (!skip_book && !out_of_book)
    {
        Move book_move = probe_book(pos);
        if (book_move.from_square != NO_SQUARE)
        {
            fprintf(stderr, "book move fired!\n");                       // <-- here
            ponder_move = {NO_SQUARE, NO_SQUARE, NO_PIECE_TYPE, NORMAL}; // 1.15.0.0 pondering tries to set a ponder move from the book
            return book_move;
        }
        else
            out_of_book = true;
    }
    // 1.14.1.1adebug
    auto start = std::chrono::steady_clock::now();
    auto elapsed_ms = [&]() -> int
    {
        auto now = std::chrono::steady_clock::now();
        return (int)std::chrono::duration_cast<std::chrono::milliseconds>(now - start).count();
    };

    int time_limit_ms = calculate_time(remaining_ms, increment_ms);

    uint64_t stack[1024];
    for (int i = 0; i < game_history_count; i++)
        stack[i] = game_history[i];

    stack[game_history_count] = pos.hash; // 1.14.0.10

    Move best = {NO_SQUARE, NO_SQUARE, NO_PIECE_TYPE, NORMAL};
    ponder_move = {NO_SQUARE, NO_SQUARE, NO_PIECE_TYPE, NORMAL}; // 1.15.0.0 move pondering
    int previous_score = 0;

    for (int depth = 1; depth <= max_depth; depth++)
    {
        // Don't start a new depth if we've already used 3/4 of the budget
        if (depth > 1 && elapsed_ms() > (time_limit_ms * 3 / 4))
            break;

        if (stop_search.load())
            break; // move pondering 1.15.0.0

        MoveList list;
        generate_moves(pos, list);
        filter_legal_moves(pos, list);

        // Seed move ordering with best move from previous depth
        if (best.from_square != NO_SQUARE)
        {
            for (int i = 0; i < list.count; i++)
            {
                if (list.moves[i].from_square == best.from_square &&
                    list.moves[i].to_square == best.to_square)
                {
                    Move temp = list.moves[0];
                    list.moves[0] = list.moves[i];
                    list.moves[i] = temp;
                    break;
                }
            }
        }
        order_moves(pos, list, depth);

        if (list.count == 0)
            break;

        // ASPIRATION WINDOW SETUP 1.14.0.13
        // Start with a narrow ±50cp window around the previous depth's score.
        // A narrow window makes alpha-beta much more efficient because more
        // branches can be pruned. If the true score falls outside the window
        // (a "fail-low" or "fail-high"), we widen and re-search. After 4 failed
        // widening attempts we fall back to a full-width (-INF, INF) search.
        Move depth_best = list.moves[0];
        int best_score = -INF;
        // REVERT FROM HERE FOR PVS FIX
        //  ---- Aspiration window setup ----
        int window = 50;
        int asp_alpha = (depth >= 3) ? std::max(-INF, previous_score - window) : -INF;
        int asp_beta = (depth >= 3) ? std::min(INF, previous_score + window) : INF;
        int asp_iters = 0;

        while (true)
        {
            best_score = -INF;
            depth_best = list.moves[0];

            for (int i = 0; i < list.count; i++)
            {
                // stop the check inside the move loop as well 1.15.0.0 pondering
                if (stop_search.load())
                    goto done;

                UndoInfo undo;
                make_move(pos, list.moves[i], undo);
                stack[game_history_count + 1] = pos.hash;

                int score;
                if (i == 0)
                {
                    score = -alphaBeta(pos, depth - 1, -asp_beta, -asp_alpha,
                                       stack, game_history_count + 2, true);

                    // Hyatt's fix: if the first (best) move fails low immediately,
                    // there is no point searching the remaining moves under the current
                    // window — the entire result will be a fail-low. Jump straight to
                    // window adjustment, which will widen asp_alpha and retry.
                    if (score <= asp_alpha)
                    {
                        unmake_move(pos, list.moves[i], undo);
                        best_score = score;
                        goto adjust_window;
                    }
                }
                else
                {
                    score = -alphaBeta(pos, depth - 1, -best_score - 1, -best_score,
                                       stack, game_history_count + 2, true);
                    if (score > best_score)
                    {
                        score = -alphaBeta(pos, depth - 1, -asp_beta, -best_score,
                                           stack, game_history_count + 2, true);
                    }
                }

                unmake_move(pos, list.moves[i], undo);

                if (score > best_score)
                {
                    best_score = score;
                    depth_best = list.moves[i];
                }

                if (elapsed_ms() > time_limit_ms || stop_search.load()) // 1.15.0.0 pondering
                    goto done;
            }

        adjust_window:
            if (best_score <= asp_alpha)
            {
                asp_alpha = std::max(-INF, asp_alpha - window);
                window *= 2;
            }
            else if (best_score >= asp_beta)
            {
                asp_beta = std::min(INF, asp_beta + window);
                window *= 2;
            }
            else
                break;

            asp_iters++;
            if (asp_iters >= 4)
            {
                asp_alpha = -INF;
                asp_beta = INF;
            }
        }

        best = depth_best;
        g_last_depth = depth;
        ponder_best_so_far = best;
        fprintf(stderr, "depth=%d score=%d elapsed=%dms\n",
                depth, best_score, elapsed_ms());
        fflush(stderr);

        // If we've found a forced mate, there's no point searching deeper
        // all additional depths would find the same (or longer) mate.
        if (best_score >= INF - MAX_DEPTH)
            break;
    }

done:
    return best;
}
