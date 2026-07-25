#include "uci.h"
#include "fen.h"
#include "movegen.h"
#include "legal.h"
#include "search.h"
#include "eval.h"
#include "zobrist.h"
#include "transposition.h"
#include "makeunmake.h"
#include "book.h"
#include <iostream>
#include <sstream>
#include <string>
#include <thread>
#include <atomic>
#include <mutex>

// ============================================================
// PONDER STATE
// ============================================================
// Time controls saved when a "go ponder" command arrives so that when
// "ponderhit" comes in we can immediately start a real timed search
// without waiting for the GUI to resend the clock values.
int ponder_remaining_ms = 0;
int ponder_increment_ms = 0;

// Background thread that runs the ponder search. Kept as a named handle
// so stop_any_search() can join it cleanly before starting a new search.
std::thread ponder_thread;
bool is_pondering = false;

// Snapshot of the position and game history at the time pondering began.
// Copied by value so the ponder thread and UCI loop don't share mutable state.
Position ponder_pos;
uint64_t ponder_history[1024];
int ponder_history_count = 0;

// ============================================================
// UCI MOVE FORMATTING
// ============================================================
// Converts a square index (0-63) to its algebraic name (e.g. 28 -> "e4").
// Used as a building block by move_to_uci to produce the full move string.
std::string sq_to_uci(Square sq)
{
    std::string s;
    s += (char)('a' + sq % 8);
    s += (char)('1' + sq / 8);
    return s;
}

// Converts a Move to a UCI string (e.g. "e2e4", "e7e8q" for promotion).
// Promotion piece is appended as a lowercase letter; all other moves are
// simply from-square + to-square with no extra suffix.
std::string move_to_uci(const Move &move)
{
    std::string s = sq_to_uci(move.from_square) + sq_to_uci(move.to_square);
    if (move.flag == PROMOTION)
    {
        switch (move.piecetype)
        {
        case QUEEN:
            s += 'q';
            break;
        case ROOK:
            s += 'r';
            break;
        case BISHOP:
            s += 'b';
            break;
        case KNIGHT:
            s += 'n';
            break;
        default:
            break;
        }
    }
    return s;
}

// Parses a UCI move token (e.g. "e2e4") by generating all legal moves for
// the position and returning the one whose UCI string matches. Returns an
// empty move (NO_SQUARE) if no legal move matches, which signals a parse error.
Move parse_uci_move(Position &pos, const std::string &token)
{
    MoveList list;
    generate_moves(pos, list);
    filter_legal_moves(pos, list);
    for (int i = 0; i < list.count; i++)
        if (move_to_uci(list.moves[i]) == token)
            return list.moves[i];
    return {NO_SQUARE, NO_SQUARE, NO_PIECE_TYPE, NORMAL};
}

// ============================================================
// POSITION PARSING
// ============================================================
// Handles the UCI "position" command. Accepts either "startpos" or a raw FEN
// string, then replays any moves listed after the "moves" keyword so that pos
// reflects the current board state. Each resulting hash is pushed onto
// game_history for repetition detection during search.
void parse_position(Position &pos, const std::string &line,
                    uint64_t *game_history, int &game_history_count)
{
    std::istringstream ss(line);
    std::string token;
    ss >> token; // "position"

    ss >> token;
    if (token == "startpos")
    {
        pos = parse_fen(starting_fen());
        ss >> token;
    }
    else if (token == "fen")
    {
        std::string fen;
        while (ss >> token && token != "moves")
            fen += token + " ";
        pos = parse_fen(fen);
    }

    game_history_count = 0;
    game_history[game_history_count++] = pos.hash;

    if (token == "moves")
    {
        while (ss >> token)
        {
            Move move = parse_uci_move(pos, token);
            if (move.from_square == NO_SQUARE)
                break;
            UndoInfo undo;
            make_move(pos, move, undo);
            game_history[game_history_count++] = pos.hash;
        }
    }
}

// ============================================================
// PONDER MOVE EXTRACTION
// ============================================================
// After the engine picks its best move, this function advances the position
// by that move and looks up the resulting position in the transposition table
// to find the engine's predicted reply (the ponder move). Falls back to the
// first legal move if the TT has no entry, ensuring we always return something
// valid for the GUI to send back as "go ponder <move>".
static Move extract_ponder_move(Position pos, uint64_t *hist, int count, Move best)
{
    if (best.from_square == NO_SQUARE)
        return {NO_SQUARE, NO_SQUARE, NO_PIECE_TYPE, NORMAL};

    UndoInfo undo;
    make_move(pos, best, undo);

    Move pm = {NO_SQUARE, NO_SQUARE, NO_PIECE_TYPE, NORMAL};
    TTEntry *entry = tt.probe(pos.hash);
    if (entry && entry->best.from_square != NO_SQUARE)
    {
        pm = entry->best;
    }
    else
    {
        MoveList reply;
        generate_moves(pos, reply);
        filter_legal_moves(pos, reply);
        if (reply.count > 0)
            pm = reply.moves[0];
    }
    return pm;
}

// ============================================================
// SEARCH THREAD MANAGEMENT
// ============================================================
// Signals the background search thread to stop and blocks until it exits.
// Must be called before starting any new search to avoid two threads writing
// to shared state (TT, history table, ponder_best_so_far) simultaneously.
static void stop_any_search()
{
    if (ponder_thread.joinable())
    {
        stop_search = true;
        ponder_thread.join();
        stop_search = false;
    }
    is_pondering = false;
}

// ============================================================
// GO COMMAND
// ============================================================
// Handles the UCI "go" command. Parses time controls (wtime/btime/winc/binc)
// and the optional "ponder" and "movetime" flags, then either launches a
// background ponder search (returning immediately) or runs a normal timed
// search and prints "bestmove <move> ponder <move>" to stdout.
void parse_go(Position &pos, const std::string &line,
              uint64_t *game_history, int game_history_count)
{
    stop_any_search();

    std::istringstream ss(line);
    std::string token;
    ss >> token; // "go"

    bool is_ponder = false;
    int wtime = 60000, btime = 60000, winc = 0, binc = 0, movetime = -1;

    while (ss >> token)
    {
        if (token == "wtime")
            ss >> wtime;
        else if (token == "btime")
            ss >> btime;
        else if (token == "winc")
            ss >> winc;
        else if (token == "binc")
            ss >> binc;
        else if (token == "movetime")
            ss >> movetime;
        else if (token == "ponder")
            is_ponder = true;
    }

    int remaining = (pos.side_to_move == WHITE) ? wtime : btime;
    int increment = (pos.side_to_move == WHITE) ? winc : binc;

    // movetime overrides the clock — scale it up so calculate_time allocates
    // the full requested duration rather than a fraction of a fake clock.
    if (movetime > 0)
    {
        remaining = movetime * 30;
        increment = 0;
    }

    // Always save these — ponderhit needs them
    ponder_remaining_ms = remaining;
    ponder_increment_ms = increment;

    if (is_ponder)
    {
        // Save position so ponderhit can start a real search
        ponder_pos = pos;
        ponder_history_count = game_history_count;
        for (int i = 0; i < game_history_count; i++)
            ponder_history[i] = game_history[i];
        is_pondering = true;

        // Capture everything by value for the thread so the UCI loop can
        // freely modify pos and game_history without affecting the search.
        Position cap_pos = pos;
        int cap_count = game_history_count;
        uint64_t cap_hist[1024];
        for (int i = 0; i < cap_count; i++)
            cap_hist[i] = game_history[i];

        stop_search = false;
        ponder_thread = std::thread([cap_pos, cap_count, cap_hist]() mutable
                                    {
                                        // skip_book=true: book moves already played, don't re-fire
                                        best_move(cap_pos, 64, cap_hist, cap_count, 999999999, 0, true);
                                        // result discarded — we only care about TT population
                                    });
        return; // don't send bestmove yet
    }

    // Normal search — book allowed (skip_book=false)
    stop_search = false;
    Move best = best_move(pos, 64, game_history, game_history_count,
                          remaining, increment, false);

    // Safety net: verify move is legal before sending. In extremely rare cases
    // a hash collision in the TT can cause the search to return a move that
    // was legal in a different position — catching it here prevents an illegal
    // move from being sent to the GUI, which would forfeit the game.
    if (best.from_square != NO_SQUARE)
    {
        UndoInfo undo;
        make_move(pos, best, undo);
        bool left_in_check = is_in_check(pos, (pos.side_to_move == WHITE) ? BLACK : WHITE);
        unmake_move(pos, best, undo);

        if (left_in_check)
        {
            fprintf(stderr, "ILLEGAL MOVE DETECTED: %s — falling back\n",
                    move_to_uci(best).c_str());
            fflush(stderr);
            MoveList safe;
            generate_moves(pos, safe);
            filter_legal_moves(pos, safe);
            if (safe.count > 0)
                best = safe.moves[0];
        }
    }

    Move pm = extract_ponder_move(pos, game_history, game_history_count, best);
    ponder_move = pm;

    std::string bm = "bestmove " + move_to_uci(best);
    if (pm.from_square != NO_SQUARE)
        bm += " ponder " + move_to_uci(pm);

    std::cout << bm << "\n";
    std::cout.flush();
}

// ============================================================
// MAIN UCI LOOP
// ============================================================
// Reads UCI commands from stdin line by line and dispatches them to the
// appropriate handler. Blocks on std::getline so the engine is entirely
// event-driven — it only does work when the GUI sends a command.
void run_uci()
{
    Position pos = parse_fen(starting_fen());
    uint64_t game_history[1024];
    int game_history_count = 0;

    init_book(pos);

    std::string line;
    while (std::getline(std::cin, line))
    {
        // Strip Windows-style carriage returns so commands parse correctly
        // on both platforms without needing separate builds.
        if (!line.empty() && line.back() == '\r')
            line.pop_back();

        fprintf(stderr, "UCI IN: '%s'\n", line.c_str());
        fflush(stderr);

        if (line.empty())
            continue;

        if (line == "uci")
        {
            // Identify the engine and advertise supported options.
            // "Ponder type check default true" tells the GUI this engine
            // supports pondering so it will send "go ponder" commands.
            std::cout << "id name Dugong\n";
            std::cout << "id author Angad\n";
            std::cout << "option name Ponder type check default true\n";
            std::cout << "uciok\n";
            std::cout.flush();
            fprintf(stderr, "sent uciok with ponder option\n");
            fflush(stderr);
        }
        else if (line == "isready")
        {
            // Sent by the GUI to confirm the engine has finished initialising.
            // All heavy init (TT allocation, book loading) happens at startup,
            // so we can respond immediately.
            std::cout << "readyok\n";
            std::cout.flush();
        }
        else if (line == "ucinewgame")
        {
            // Reset all search state between games so history heuristics and
            // TT entries from a previous game don't influence the new one.
            stop_any_search();
            tt.clear();
            clear_killers();
            clear_history();
            out_of_book = false;
            pos = parse_fen(starting_fen());
            game_history_count = 0;
        }
        else if (line == "stop")
        {
            // If a ponderhit search is running, it must send bestmove before stopping
            // stop_any_search handles that via stop_search flag
            // stop_any_search();
            bool was_pondering = is_pondering;
            stop_any_search();

            // If we were pondering, the GUI expects a bestmove response even
            // on stop. Send the best move found so far rather than nothing,
            // falling back to "0000" (null move) if the search hadn't started.
            if (was_pondering)
            {
                std::string bm = "bestmove ";
                bm += (ponder_best_so_far.from_square != NO_SQUARE)
                          ? move_to_uci(ponder_best_so_far)
                          : "0000";
                fprintf(stderr, "stop sending: %s\n", bm.c_str());
                fflush(stderr);
                std::cout << bm << "\n";
                std::cout.flush();
                is_pondering = false;
            }
        }
        else if (line == "ponderhit")
        {
            // Opponent played our ponder move: stop infinite search,
            // run a real timed search on the same position, send bestmove.
            // We use the time controls saved during "go ponder" since the
            // GUI does not resend them with "ponderhit".
            stop_any_search();

            Position ph_pos = pos;
            int ph_count = game_history_count;
            uint64_t ph_hist[1024];
            for (int i = 0; i < ph_count; i++)
                ph_hist[i] = game_history[i];

            int ph_rem = ponder_remaining_ms;
            int ph_inc = ponder_increment_ms;

            fprintf(stderr, "ponderhit: rem=%d inc=%d\n", ph_rem, ph_inc);
            fflush(stderr);

            // Run in a thread so UCI loop stays responsive.
            // If a new go arrives before this finishes, stop_any_search()
            // will kill it cleanly and parse_go runs instead.
            stop_search = false;
            ponder_thread = std::thread([ph_pos, ph_count, ph_rem, ph_inc, ph_hist]() mutable
                                        {
                Move best = best_move(ph_pos, 64, ph_hist, ph_count,
                                      ph_rem, ph_inc, true);

                    // Same illegal-move safety net as in parse_go — TT collisions
                    // can theoretically surface here too, so we validate before sending.
                    if (best.from_square != NO_SQUARE)
                    {
                        UndoInfo undo;
                        make_move(ph_pos, best, undo);
                        bool left_in_check = is_in_check(ph_pos, (ph_pos.side_to_move == WHITE) ? BLACK : WHITE);
                        unmake_move(ph_pos, best, undo);

                        if (left_in_check)
                        {
                            fprintf(stderr, "ILLEGAL MOVE DETECTED in ponderhit: %s — falling back\n",
                            move_to_uci(best).c_str());
                            fflush(stderr);
                            MoveList safe;
                            generate_moves(ph_pos, safe);
                            filter_legal_moves(ph_pos, safe);
                            if (safe.count > 0)
                                best = safe.moves[0];
                        }
                    }

                // If we were stopped externally, don't send bestmove cause 
                // parse_go will send it from its own normal search.
                if (stop_search.load() || best.from_square == NO_SQUARE)
                {
                    fprintf(stderr, "ponderhit aborted\n");
                    fflush(stderr);
                    return;
                }

                Move pm = extract_ponder_move(ph_pos, ph_hist, ph_count, best);
                ponder_move = pm;

                std::string bm = "bestmove " + move_to_uci(best);
                if (pm.from_square != NO_SQUARE)
                    bm += " ponder " + move_to_uci(pm);

                fprintf(stderr, "ponderhit sending: %s\n", bm.c_str());
                fflush(stderr);
                std::cout << bm << "\n";
                std::cout.flush(); });
        }
        else if (line.substr(0, 8) == "position")
            parse_position(pos, line, game_history, game_history_count);
        else if (line.substr(0, 2) == "go")
            parse_go(pos, line, game_history, game_history_count);
        else if (line == "quit")
        {
            // Stop any running search before exiting so the ponder thread
            // doesn't outlive the process and cause undefined behaviour.
            stop_any_search();
            break;
        }
    }
}