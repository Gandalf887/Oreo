#include "gui.h"
#include "search.h"
#include "legal.h"
#include "transposition.h"
#include "zobrist.h"
#include "book.h"
#include <sstream>
#include <iomanip>
#include <string>
#include <cmath>
#include <thread>
#include <atomic>
#include <mutex>
#include <iostream>
#include <math.h>

using namespace std;

// ============================================================
// LAYOUT
// ============================================================
const int WINDOW_WIDTH = 1200;
const int WINDOW_HEIGHT = 820;
const int BOARD_OFFSET_X = 280;
const int BOARD_OFFSET_Y = 60;
const int SQUARE_SIZE = 86;
const int BOARD_SIZE = SQUARE_SIZE * 8;

// ============================================================
// COLOURS
// ============================================================
const Color BG_COLOR = {28, 27, 23, 255};
const Color LIGHT_SQ = {235, 209, 166, 255};
const Color DARK_SQ = {165, 117, 81, 255};
const Color HIGHLIGHT_SEL = {186, 202, 43, 180};
const Color HIGHLIGHT_MOV = {247, 247, 105, 140};
const Color HIGHLIGHT_LAST_FROM = {205, 210, 60, 160};
const Color HIGHLIGHT_LAST_TO = {205, 210, 60, 200};
const Color HIGHLIGHT_PONDER = {100, 180, 255, 100};
const Color HIGHLIGHT_CHECK = {220, 50, 50, 190};
const Color HIGHLIGHT_MATE_WIN = {60, 200, 80, 210};
const Color HIGHLIGHT_MATE_LOSE = {200, 40, 40, 210};
const Color TEXT_GRAY = {150, 150, 150, 255};
const Color TEXT_WHITE = {220, 220, 220, 255};
const Color TEXT_YELLOW = {200, 160, 80, 255};
const Color TEXT_GREEN = {100, 200, 100, 255};
const Color TEXT_PURPLE = {160, 100, 200, 255};
const Color TEXT_RED = {180, 80, 80, 255};
const Color TEXT_BLUE = {100, 160, 220, 255};
const Color RAY_WHITE = {255, 255, 255, 255};
const Color RAY_BLACK = {0, 0, 0, 255};

// ============================================================
// PIECE PNG'S & FONTS
// ============================================================
Texture2D piece_textures[12];
const char *PIECE_FILES[12] = {
    "resources/images/wP.png", "resources/images/wN.png",
    "resources/images/wB.png", "resources/images/wR.png",
    "resources/images/wQ.png", "resources/images/wK.png",
    "resources/images/bP.png", "resources/images/bN.png",
    "resources/images/bB.png", "resources/images/bR.png",
    "resources/images/bQ.png", "resources/images/bK.png"};

enum GameState
{
    SELECTING_SIDE,
    SELECTING_TIME,
    PLAYING
};

Font main_font;

static GameHistory gh;
static uint64_t game_history[1024];

// ── Externals from search.cpp / uci.cpp ───────────────────────────────────
extern int g_last_depth;
extern Move ponder_best_so_far;
extern bool out_of_book;
// stop_search : std::atomic<bool> declared in search.h

// ── GUI pondering state ───────────────────────────────────────────────────
static std::thread gui_ponder_thread;
static std::atomic<bool> gui_pondering{false};
static Move gui_ponder_move;    // move we are pondering over
static Position gui_ponder_pos; // board after engine move + ponder move
static uint64_t gui_ponder_history[1024];
static int gui_ponder_history_count = 0;

// Signals the background ponder thread to stop via stop_search, then joins it
// on a detached thread to avoid blocking the GUI. Resets stop_search to false
// once the ponder thread has exited so future searches are not immediately halted.
static void stop_ponder()
{
    if (gui_ponder_thread.joinable())
    {
        stop_search = true;
        std::thread([t = std::move(gui_ponder_thread)]() mutable
                    {
            t.join();
            stop_search = false; })
            .detach();
    }
    else
    {
        stop_search = false;
    }
    gui_pondering = false;
}

// Starts a background search on the position after the engine's move plus the
// expected reply (ponder move). Searches up to depth 24 with 30 seconds so it
// runs indefinitely until stop_ponder() is called. If the human plays the
// predicted move (a "ponder hit"), the search result can be used immediately.
static void start_ponder(Position cap_pos, uint64_t *cap_hist, int cap_count)
{
    stop_ponder();
    uint64_t hist_copy[1024];
    for (int i = 0; i < cap_count; i++)
        hist_copy[i] = cap_hist[i];
    stop_search = false;
    gui_pondering = true;
    gui_ponder_thread = std::thread([cap_pos, hist_copy, cap_count]() mutable
                                    { best_move(cap_pos, 24, hist_copy, cap_count, 30000, 0, true); });
}

// ── Small helpers ─────────────────────────────────────────────────────────
// Converts a board square index to pixel coordinates of the top-left corner of
// that square, accounting for board orientation (flipped = Black's perspective).
static void sq_to_screen(Square sq, bool flipped, int &sx, int &sy)
{
    int file = sq % 8, rank = sq / 8;
    sx = BOARD_OFFSET_X + (flipped ? 7 - file : file) * SQUARE_SIZE;
    sy = BOARD_OFFSET_Y + (flipped ? rank : 7 - rank) * SQUARE_SIZE;
}

// Converts a mouse pixel position to a board square index, returning NO_SQUARE
// if the click is outside the board area.
static Square screen_to_square(int mx, int my, bool flipped)
{
    int file = (mx - BOARD_OFFSET_X) / SQUARE_SIZE;
    int rank = (my - BOARD_OFFSET_Y) / SQUARE_SIZE;
    if (file < 0 || file >= 8 || rank < 0 || rank >= 8)
        return NO_SQUARE;
    return static_cast<Square>((flipped ? rank : 7 - rank) * 8 + (flipped ? 7 - file : file));
}

// Formats a time in seconds as "MM:SS", clamped to zero for negative values.
static string format_time(int s)
{
    if (s < 0)
        s = 0;
    char buf[16];
    snprintf(buf, sizeof(buf), "%02d:%02d", s / 60, s % 60);
    return buf;
}

// Draws text horizontally centred on x at the given y position using the global font.
void draw_centered(const char *text, int x, int y, int size, Color col)
{
    DrawTextEx(main_font, text,
               {(float)(x - MeasureText(text, size) / 2), (float)y},
               size, 1, col);
}

// Loads the font and all 12 piece textures (6 white, 6 black) from disk into GPU memory.
void load_assets()
{
    main_font = LoadFontEx("resources/fonts/centurygothic.ttf", 128, nullptr, 0);
    for (int i = 0; i < 12; i++)
        piece_textures[i] = LoadTexture(PIECE_FILES[i]);
}

// Releases all GPU texture and font resources. Called on shutdown.
void unload_assets()
{
    for (int i = 0; i < 12; i++)
        UnloadTexture(piece_textures[i]);
    UnloadFont(main_font);
}

// ── Draw helpers ──────────────────────────────────────────────────────────
// Draws the 64 board squares in alternating light/dark colours and renders rank
// numbers and file letters along the edges, respecting the flipped orientation.
void draw_board(bool flipped)
{
    for (int r = 0; r < 8; r++)
        for (int f = 0; f < 8; f++)
        {
            DrawRectangle(BOARD_OFFSET_X + (flipped ? 7 - f : f) * SQUARE_SIZE,
                          BOARD_OFFSET_Y + (flipped ? r : 7 - r) * SQUARE_SIZE,
                          SQUARE_SIZE, SQUARE_SIZE,
                          (r + f) % 2 != 0 ? LIGHT_SQ : DARK_SQ);
        }
    for (int i = 0; i < 8; i++)
    {
        char buf[4];
        snprintf(buf, sizeof(buf), "%d", flipped ? i + 1 : 8 - i);
        DrawTextEx(main_font, buf,
                   {(float)(BOARD_OFFSET_X - 20),
                    (float)(BOARD_OFFSET_Y + i * SQUARE_SIZE + SQUARE_SIZE / 2 - 10)},
                   18, 1, TEXT_GRAY);
        snprintf(buf, sizeof(buf), "%c", 'a' + (flipped ? 7 - i : i));
        DrawTextEx(main_font, buf,
                   {(float)(BOARD_OFFSET_X + i * SQUARE_SIZE + SQUARE_SIZE / 2 - 6),
                    (float)(BOARD_OFFSET_Y + BOARD_SIZE + 8)},
                   18, 1, TEXT_GRAY);
    }
}

// Highlights the from- and to-squares of the most recently played move in yellow.
void draw_last_move(Square from, Square to, bool flipped)
{
    if (from == NO_SQUARE || to == NO_SQUARE)
        return;
    int sx, sy;
    sq_to_screen(from, flipped, sx, sy);
    DrawRectangle(sx, sy, SQUARE_SIZE, SQUARE_SIZE, HIGHLIGHT_LAST_FROM);
    sq_to_screen(to, flipped, sx, sy);
    DrawRectangle(sx, sy, SQUARE_SIZE, SQUARE_SIZE, HIGHLIGHT_LAST_TO);
}

// Draws a translucent blue overlay on the square the engine expects the human to
// play to (the ponder move destination), giving a visual hint during pondering.
void draw_ponder_hint(Square sq, bool flipped)
{
    if (sq == NO_SQUARE)
        return;
    int sx, sy;
    sq_to_screen(sq, flipped, sx, sy);
    DrawRectangle(sx, sy, SQUARE_SIZE, SQUARE_SIZE, HIGHLIGHT_PONDER);
}

// Draws a red overlay on the king's square when the side to move is in check.
void draw_check_highlight(const Position &pos, bool flipped)
{
    if (!is_in_check(pos, pos.side_to_move))
        return;
    Bitboard kb = pos.side_to_move == WHITE ? pos.bitboards[WK] : pos.bitboards[BK];
    if (!kb)
        return;
    int sx, sy;
    sq_to_screen(static_cast<Square>(__builtin_ctzll(kb)), flipped, sx, sy);
    DrawRectangle(sx, sy, SQUARE_SIZE, SQUARE_SIZE, HIGHLIGHT_CHECK);
}

// At the end of the game, colours the losing king red and the winning king green
// to make the result visually clear on the board.
void draw_checkmate_kings(const Position &pos, bool flipped)
{
    Bitboard lb = pos.side_to_move == WHITE ? pos.bitboards[WK] : pos.bitboards[BK];
    Bitboard wb = pos.side_to_move == WHITE ? pos.bitboards[BK] : pos.bitboards[WK];
    int sx, sy;
    if (lb)
    {
        sq_to_screen(static_cast<Square>(__builtin_ctzll(lb)), flipped, sx, sy);
        DrawRectangle(sx, sy, SQUARE_SIZE, SQUARE_SIZE, HIGHLIGHT_MATE_LOSE);
    }
    if (wb)
    {
        sq_to_screen(static_cast<Square>(__builtin_ctzll(wb)), flipped, sx, sy);
        DrawRectangle(sx, sy, SQUARE_SIZE, SQUARE_SIZE, HIGHLIGHT_MATE_WIN);
    }
}

// Highlights the selected square in green-yellow and all legal destination squares
// for that piece with a semi-transparent yellow overlay and a small dot indicator.
void draw_highlights(const MoveList &list, Square selected, bool flipped)
{
    if (selected != NO_SQUARE)
    {
        int sx, sy;
        sq_to_screen(selected, flipped, sx, sy);
        DrawRectangle(sx, sy, SQUARE_SIZE, SQUARE_SIZE, HIGHLIGHT_SEL);
    }
    for (int i = 0; i < list.count; i++)
        if (list.moves[i].from_square == selected)
        {
            int sx, sy;
            sq_to_screen(list.moves[i].to_square, flipped, sx, sy);
            DrawRectangle(sx, sy, SQUARE_SIZE, SQUARE_SIZE, HIGHLIGHT_MOV);
            DrawCircle(sx + SQUARE_SIZE / 2, sy + SQUARE_SIZE / 2, 12, {0, 0, 0, 80});
        }
}

// Iterates all 64 squares, looks up which piece occupies each via the bitboards,
// and draws the corresponding texture scaled to SQUARE_SIZE.
void draw_pieces(const Position &pos, bool flipped)
{
    for (int r = 0; r < 8; r++)
        for (int f = 0; f < 8; f++)
        {
            Square sq = static_cast<Square>(r * 8 + f);
            int sx = BOARD_OFFSET_X + (flipped ? 7 - f : f) * SQUARE_SIZE;
            int sy = BOARD_OFFSET_Y + (flipped ? r : 7 - r) * SQUARE_SIZE;
            for (int i = 0; i < 12; i++)
                if (square_set(pos.bitboards[i], sq))
                {
                    float scale = (float)SQUARE_SIZE / piece_textures[i].width;
                    DrawTextureEx(piece_textures[i], {(float)sx, (float)sy}, 0.0f, scale, RAY_WHITE);
                    break;
                }
        }
}

// Draws the two player clock panels to the right of the board. The active clock's
// time pulses when below 30s and turns red below 10s. A thin progress bar shows
// the fraction of 30s remaining. Labels change to "(pondering)" while the engine
// thinks on the opponent's time.
void draw_clocks(double white_time, double black_time,
                 Colour side_to_move, Colour engine_side,
                 bool flipped, bool pondering_now)
{
    const int cx = BOARD_OFFSET_X + BOARD_SIZE + 30;
    const int cw = 240;
    const int mid_y = WINDOW_HEIGHT / 2;

    bool top_white = flipped;
    double top_time = top_white ? white_time : black_time;
    double bot_time = top_white ? black_time : white_time;
    Colour top_col = top_white ? WHITE : BLACK;
    Colour bot_col = top_white ? BLACK : WHITE;
    bool top_active = (side_to_move == top_col);

    auto label_for = [&](Colour c) -> string
    {
        if (c == engine_side && pondering_now)
            return string(c == WHITE ? "White" : "Black") + " (pondering)";
        return string(c == WHITE ? "White" : "Black") + (c == engine_side ? " (Engine)" : " (You)");
    };
    auto pulse = [](double t, bool active) -> float
    {
        if (!active || t >= 10.0)
            return 1.0f;
        return 1.0f + 0.04f * (1.0f - (float)(t / 10.0)) * sinf((float)(GetTime() * 6.0));
    };
    auto tcol = [](double t, bool active) -> Color
    {
        if (!active)
            return TEXT_GRAY;
        if (t < 10.0)
            return TEXT_RED;
        if (t < 30.0)
            return TEXT_YELLOW;
        return TEXT_WHITE;
    };
    auto draw_panel = [&](int py, double t, bool active, Colour c)
    {
        Color lc = (c == engine_side && pondering_now) ? TEXT_BLUE : TEXT_GRAY;
        DrawTextEx(main_font, label_for(c).c_str(), {(float)(cx + 8), (float)py}, 20, 1, lc);
        string ts = format_time((int)t);
        int fsz = (int)(68 * pulse(t, active));
        Vector2 ts_size = MeasureTextEx(main_font, ts.c_str(), fsz, 1);
        DrawTextEx(main_font, ts.c_str(),
                   {cx + 8 + (cw - 16 - ts_size.x) / 2.0f, (float)(py + 28)},
                   fsz, 1, tcol(t, active));
        if (active && t < 30.0)
        {
            Color bar = t < 10.0 ? TEXT_RED : TEXT_YELLOW;
            bar.a = 160;
            DrawRectangle(cx, py + 108, (int)(cw * (t / 30.0)), 4, bar);
            DrawRectangle(cx, py + 108, cw, 4, {80, 80, 80, 80});
        }
    };

    draw_panel(mid_y - 190, top_time, top_active, top_col);
    DrawLine(cx + 10, mid_y - 10, cx + cw - 10, mid_y - 10, {60, 60, 60, 200});
    draw_panel(mid_y + 20, bot_time, !top_active, bot_col);
}

// Renders the sidebar info panel: search depth, centipawn evaluation, last move
// played, pondering indicator, move history position, and keyboard shortcut hints.
void draw_info(int depth, double eval, const string &last_move,
               int history_current, int history_count, bool pondering)
{
    int px = 20, py = 40;
    DrawTextEx(main_font, ("Depth: " + to_string(depth)).c_str(), {(float)px, (float)py}, 26, 1, TEXT_RED);
    string es = (eval >= 0 ? "+" : "") + to_string(eval).substr(0, to_string(eval).find('.') + 3);
    DrawTextEx(main_font, ("Eval:  " + es).c_str(), {(float)px, (float)(py + 45)}, 26, 1, TEXT_YELLOW);
    DrawTextEx(main_font, ("Move:  " + last_move).c_str(), {(float)px, (float)(py + 90)}, 26, 1, TEXT_PURPLE);
    if (pondering)
        DrawTextEx(main_font, "Pondering...", {(float)px, (float)(py + 125)}, 20, 1, TEXT_BLUE);
    DrawTextEx(main_font, (to_string(history_current) + " / " + to_string(history_count)).c_str(),
               {(float)px, (float)(py + 155)}, 22, 1, TEXT_GRAY);
    DrawTextEx(main_font, "< > review moves", {(float)px, (float)(WINDOW_HEIGHT - 100)}, 18, 1, TEXT_GRAY);
    DrawTextEx(main_font, "F   flip board", {(float)px, (float)(WINDOW_HEIGHT - 74)}, 18, 1, TEXT_GRAY);
    DrawTextEx(main_font, "R   play again", {(float)px, (float)(WINDOW_HEIGHT - 48)}, 18, 1, TEXT_GRAY);
}

// Draws either the side-selection screen (White/Black buttons) or the time-control
// selection screen (1/3/5/10/15 minute buttons), depending on the current GameState.
void draw_selection_screen(GameState state)
{
    int cx = WINDOW_WIDTH / 2;
    if (state == SELECTING_SIDE)
    {
        draw_centered("Oreo 1.16.1.9", cx, 120, 52, TEXT_WHITE);
        draw_centered("Choose your side", cx, 240, 28, TEXT_GRAY);
        DrawRectangle(cx - 180, 310, 150, 65, LIGHT_SQ);
        draw_centered("Play White", cx - 105, 330, 24, RAY_BLACK);
        DrawRectangle(cx + 30, 310, 150, 65, DARK_SQ);
        draw_centered("Play Black", cx + 105, 330, 24, TEXT_WHITE);
    }
    else
    {
        draw_centered("Select Time Control", cx, 220, 32, TEXT_WHITE);
        vector<int> times = {1, 3, 5, 10, 15};
        for (int i = 0; i < (int)times.size(); i++)
        {
            int bx = cx - 290 + i * 130;
            DrawRectangle(bx, 310, 110, 65, {70, 70, 70, 255});
            DrawRectangleLines(bx, 310, 110, 65, TEXT_GRAY);
            draw_centered((to_string(times[i]) + " min").c_str(), bx + 55, 330, 22, TEXT_WHITE);
        }
    }
}

// Draws a semi-transparent overlay on the board with the game result message.
void draw_game_over(const string &message, bool is_checkmate,
                    const Position &pos, bool flipped)
{
    DrawRectangle(BOARD_OFFSET_X, BOARD_OFFSET_Y, BOARD_SIZE, BOARD_SIZE, {0, 0, 0, 120});
    int bx = BOARD_OFFSET_X + BOARD_SIZE / 2 - 180;
    int by = BOARD_OFFSET_Y + BOARD_SIZE / 2 - 60;
    DrawRectangle(bx, by, 360, 120, {40, 40, 40, 240});
    DrawRectangleLines(bx, by, 360, 120, TEXT_GRAY);
    draw_centered(message.c_str(), BOARD_OFFSET_X + BOARD_SIZE / 2, by + 20, 26, TEXT_WHITE);
    draw_centered("Press R to play again", BOARD_OFFSET_X + BOARD_SIZE / 2, by + 70, 20, TEXT_GRAY);
}

// Main GUI entry point. Opens the Raylib window and runs the game loop at 60 FPS.
// Manages all game state: menu flow, clock ticking, human input, engine thread
// dispatch, pondering, move history browsing (arrow keys), board flip (F), and
// game reset (R). The engine runs on a detached thread and signals completion via
// the engine_move_ready atomic flag so the GUI thread is never blocked.
void run_gui(Position &pos)
{
    InitWindow(WINDOW_WIDTH, WINDOW_HEIGHT, "Oreo 1.16.1.9");
    SetTargetFPS(60);
    load_assets();

    GameState state = SELECTING_SIDE;
    Colour engine_side = WHITE;
    bool flipped = false;

    Square selected = NO_SQUARE;
    Square last_from = NO_SQUARE;
    Square last_to = NO_SQUARE;
    Square ponder_hint_sq = NO_SQUARE;

    string last_move_str = "-";
    double last_eval = 0.0;
    int last_depth = 0;

    bool game_over = false;
    bool game_over_is_checkmate = false;
    string game_over_msg = "";
    bool human_turn = false;

    std::atomic<bool> engine_thinking{false};
    std::atomic<bool> engine_move_ready{false};
    Move engine_best_move;
    double engine_search_eval = 0.0;
    int engine_search_depth = 0;

    double white_time = 180.0;
    double black_time = 180.0;
    double selected_time = 180.0;

    int game_history_count = 0;
    game_history[game_history_count++] = hash_position(pos);

    gh.count = gh.current = 0;
    gh.positions[0] = pos;

    // ── Cached move list - only regenerated when position changes ──────────
    MoveList all_moves;
    bool moves_dirty = true;
    bool in_check = false;

    while (!WindowShouldClose())
    {
        double delta = GetFrameTime();
        if (delta > 0.5)
            delta = 0.0;

        BeginDrawing();
        ClearBackground(BG_COLOR);

        // ── MENUES ─────────────────────────────────────────────────────────
        if (state != PLAYING)
        {
            draw_selection_screen(state);
            if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON))
            {
                int mx = GetMouseX(), my = GetMouseY(), cx = WINDOW_WIDTH / 2;
                if (state == SELECTING_SIDE)
                {
                    if (mx >= cx - 180 && mx <= cx - 30 && my >= 310 && my <= 375)
                    {
                        engine_side = BLACK;
                        flipped = false;
                        state = SELECTING_TIME;
                    }
                    else if (mx >= cx + 30 && mx <= cx + 180 && my >= 310 && my <= 375)
                    {
                        engine_side = WHITE;
                        flipped = true;
                        state = SELECTING_TIME;
                    }
                }
                else
                {
                    vector<int> times = {1, 3, 5, 10, 15};
                    for (int i = 0; i < (int)times.size(); i++)
                    {
                        int bx = cx - 290 + i * 130;
                        if (mx >= bx && mx <= bx + 110 && my >= 310 && my <= 375)
                        {
                            selected_time = times[i] * 60.0;
                            white_time = black_time = selected_time;
                            human_turn = (engine_side == BLACK);
                            moves_dirty = true;
                            state = PLAYING;
                        }
                    }
                }
            }
            EndDrawing();
            continue;
        }

        // ── PLAYING ───────────────────────────────────────────────────────
        bool browsing = (gh.current < gh.count);

        // Rebuild move list only when position changed
        if (moves_dirty)
        {
            generate_moves(pos, all_moves);
            filter_legal_moves(pos, all_moves);
            in_check = is_in_check(pos, pos.side_to_move);
            moves_dirty = false;

            if (all_moves.count == 0 && !game_over && !browsing)
            {
                stop_ponder();
                game_over = true;
                if (in_check)
                {
                    game_over_is_checkmate = true;
                    game_over_msg = string(pos.side_to_move == WHITE ? "Black" : "White") + " wins by checkmate!";
                }
                else
                {
                    game_over_is_checkmate = false;
                    game_over_msg = "Stalemate — Draw!";
                }
            }
            else if (pos.halfmove_clock >= 100)
            {
                stop_ponder();
                game_over = true;
                game_over_is_checkmate = false;
                game_over_msg = "Draw by 50-move rule!";
            }
        }

        // Clocks
        if (!game_over && !browsing)
        {
            if (human_turn)
            {
                if (engine_side == BLACK)
                    white_time -= delta;
                else
                    black_time -= delta;
            }
            else if (engine_thinking || gui_pondering)
            {
                if (engine_side == WHITE)
                    white_time -= delta;
                else
                    black_time -= delta;
            }

            if (white_time <= 0)
            {
                game_over_msg = "Black wins on time!";
                game_over = true;
                game_over_is_checkmate = false;
                stop_ponder();
            }
            if (black_time <= 0)
            {
                game_over_msg = "White wins on time!";
                game_over = true;
                game_over_is_checkmate = false;
                stop_ponder();
            }
        }

        //====Keys====
        if (IsKeyPressed(KEY_F))
            flipped = !flipped;

        if (IsKeyPressed(KEY_R))
        {
            stop_ponder();
            stop_search = true;
            if (engine_thinking)
            {
                std::this_thread::sleep_for(std::chrono::milliseconds(10));
            }
            engine_thinking = engine_move_ready = false;
            stop_search = false;

            pos = parse_fen(starting_fen());
            gh.count = gh.current = 0;
            gh.positions[0] = pos;
            white_time = black_time = selected_time;
            last_move_str = "-";
            last_eval = 0.0;
            last_depth = 0;
            game_over = game_over_is_checkmate = false;
            selected = last_from = last_to = ponder_hint_sq = NO_SQUARE;
            game_history_count = 0;
            game_history[game_history_count++] = hash_position(pos);

            tt.clear();
            clear_killers();
            clear_history();
            out_of_book = false;
            ponder_best_so_far = {NO_SQUARE, NO_SQUARE, NO_PIECE_TYPE, NORMAL};

            moves_dirty = true;
            state = SELECTING_SIDE;
            EndDrawing();
            continue;
        }

        if (IsKeyPressed(KEY_LEFT) && gh.current > 0)
        {
            gh.current--;
            pos = gh.positions[gh.current];
            selected = NO_SQUARE;
            moves_dirty = true;
        }
        if (IsKeyPressed(KEY_RIGHT) && gh.current < gh.count)
        {
            gh.current++;
            pos = gh.positions[gh.current];
            selected = NO_SQUARE;
            moves_dirty = true;
        }

        browsing = (gh.current < gh.count);

        // ── DRAW ──────────────────────────────────────────────────────────
        draw_board(flipped);
        if (!browsing)
        {
            draw_last_move(last_from, last_to, flipped);
            if (gui_pondering && ponder_hint_sq != NO_SQUARE)
                draw_ponder_hint(ponder_hint_sq, flipped);
            if (!game_over && in_check)
            {
                // draw check highlight manually — no is_in_check() call here
                Bitboard kb = pos.side_to_move == WHITE ? pos.bitboards[WK] : pos.bitboards[BK];
                if (kb)
                {
                    int sx, sy;
                    sq_to_screen(static_cast<Square>(__builtin_ctzll(kb)), flipped, sx, sy);
                    DrawRectangle(sx, sy, SQUARE_SIZE, SQUARE_SIZE, HIGHLIGHT_CHECK);
                }
            }
            draw_highlights(all_moves, selected, flipped);
            if (game_over && !browsing && game_over_is_checkmate)
                draw_checkmate_kings(pos, flipped);
            draw_pieces(pos, flipped);
        }
        draw_pieces(pos, flipped);
        draw_clocks(white_time, black_time, pos.side_to_move, engine_side, flipped, (bool)gui_pondering);
        draw_info(last_depth, last_eval, last_move_str, gh.current, gh.count, (bool)gui_pondering);
        if (game_over && !browsing)
            draw_game_over(game_over_msg, game_over_is_checkmate, pos, flipped);

        // ── ENGINE THINK TRIGGER ──────────────────────────────────────────
        // Spawns the engine search on a detached thread when it is the engine's
        // turn. The thread writes the result to engine_best_move and sets
        // engine_move_ready; the main loop picks it up on the next frame without
        // blocking. A safety check re-runs legal move generation in case the
        // engine returns an illegal move (should never happen, guards against
        // rare TT corruption).
        if (!browsing && !game_over &&
            pos.side_to_move == engine_side &&
            !engine_thinking && !engine_move_ready)
        {
            engine_thinking = true;
            ponder_hint_sq = NO_SQUARE;

            double rem_ms = (engine_side == WHITE ? white_time : black_time) * 1000.0;
            Position tpos = pos;
            int tcount = game_history_count;
            uint64_t thist[1024];
            for (int i = 0; i < tcount; i++)
                thist[i] = game_history[i];

            stop_search = false;
            std::thread([&, tpos, tcount, thist, rem_ms]() mutable
                        {
                Move mv = best_move(tpos, 64, thist, tcount, (int)rem_ms, 0, false);

                if (mv.from_square != NO_SQUARE)
                {
                    UndoInfo v; make_move(tpos, mv, v);
                    bool illegal = is_in_check(tpos, tpos.side_to_move == WHITE ? BLACK : WHITE);
                    unmake_move(tpos, mv, v);
                    if (illegal)
                    {
                        MoveList safe; generate_moves(tpos, safe); filter_legal_moves(tpos, safe);
                        if (safe.count > 0) mv = safe.moves[0];
                    }
                }

                engine_best_move    = mv;
                engine_search_eval  = (evaluate(tpos) / 100.0) * (tpos.side_to_move == WHITE ? 1 : -1);
                engine_search_depth = g_last_depth;
                engine_move_ready   = true; })
                .detach();
        }

        //====ENGINE MOVE====
        if (engine_move_ready && !browsing)
        {
            engine_move_ready = false;
            engine_thinking = false;

            last_from = engine_best_move.from_square;
            last_to = engine_best_move.to_square;
            last_depth = engine_search_depth;
            last_eval = engine_search_eval;
            last_move_str = piece_to_string(engine_best_move.piecetype) + " " +
                            square_to_string(engine_best_move.from_square) +
                            square_to_string(engine_best_move.to_square);

            UndoInfo undo;
            make_move(pos, engine_best_move, undo);
            game_history[game_history_count++] = hash_position(pos);
            gh.moves[gh.count] = engine_best_move;
            gh.count++;
            gh.current = gh.count;
            gh.positions[gh.current] = pos;
            selected = NO_SQUARE;
            human_turn = true;
            moves_dirty = true; // position changed

            // After applying the engine's move, pick a ponder move (the expected
            // human reply) from the TT if available, otherwise fall back to the
            // first legal move. Apply it to a copy of the position and start the
            // background search on that copy.
            {
                Move pm = {NO_SQUARE, NO_SQUARE, NO_PIECE_TYPE, NORMAL};
                TTEntry *entry = tt.probe(pos.hash);
                if (entry && entry->best.from_square != NO_SQUARE)
                    pm = entry->best;
                else
                {
                    MoveList reply;
                    generate_moves(pos, reply);
                    filter_legal_moves(pos, reply);
                    if (reply.count > 0)
                        pm = reply.moves[0];
                }

                if (pm.from_square != NO_SQUARE)
                {
                    gui_ponder_move = pm;
                    ponder_hint_sq = pm.to_square;

                    gui_ponder_pos = pos;
                    gui_ponder_history_count = game_history_count;
                    for (int i = 0; i < game_history_count; i++)
                        gui_ponder_history[i] = game_history[i];

                    UndoInfo pundo;
                    make_move(gui_ponder_pos, pm, pundo);
                    gui_ponder_history[gui_ponder_history_count++] = hash_position(gui_ponder_pos);

                    start_ponder(gui_ponder_pos, gui_ponder_history, gui_ponder_history_count);
                }
            }
        }

        //====HUMAN MOVES====
        Colour human_side = engine_side == WHITE ? BLACK : WHITE;
        if (!browsing && !game_over &&
            pos.side_to_move == human_side && !engine_thinking)
        {
            if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON))
            {
                Square clicked = screen_to_square(GetMouseX(), GetMouseY(), flipped);
                if (clicked != NO_SQUARE)
                {
                    Bitboard hpieces = human_side == WHITE ? white_pieces(pos) : black_pieces(pos);
                    if (selected == NO_SQUARE)
                    {
                        if (square_set(hpieces, clicked))
                            selected = clicked;
                    }
                    else
                    {
                        bool moved = false;
                        for (int i = 0; i < all_moves.count; i++)
                        {
                            if (all_moves.moves[i].from_square != selected ||
                                all_moves.moves[i].to_square != clicked)
                                continue;

                            bool ponderhit = gui_pondering &&
                                             gui_ponder_move.from_square != NO_SQUARE &&
                                             all_moves.moves[i].from_square == gui_ponder_move.from_square &&
                                             all_moves.moves[i].to_square == gui_ponder_move.to_square;

                            stop_ponder();
                            ponder_hint_sq = NO_SQUARE;

                            if (ponderhit)
                            {
                                fprintf(stderr, "[GUI] ponderhit!\n");
                                fflush(stderr);
                            }

                            last_from = selected;
                            last_to = clicked;

                            UndoInfo undo;
                            make_move(pos, all_moves.moves[i], undo);
                            game_history[game_history_count++] = hash_position(pos);
                            gh.moves[gh.count] = all_moves.moves[i];
                            gh.count++;
                            gh.current = gh.count;
                            gh.positions[gh.current] = pos;

                            last_move_str = square_to_string(selected) + square_to_string(clicked);
                            selected = NO_SQUARE;
                            moved = true;
                            human_turn = false;
                            moves_dirty = true;
                            break;
                        }
                        if (!moved)
                        {
                            selected = NO_SQUARE;
                            Bitboard hp2 = human_side == WHITE ? white_pieces(pos) : black_pieces(pos);
                            if (square_set(hp2, clicked))
                                selected = clicked;
                        }
                    }
                }
            }
        }

        EndDrawing();
    }

    stop_ponder();
    unload_assets();
    CloseWindow();
}