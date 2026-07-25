#include "raylib.h"
int main()
{
    InitWindow(800, 600, "test");
    while (!WindowShouldClose())
    {
        BeginDrawing();
        ClearBackground(BLACK);
        DrawText("raylib works!", 100, 100, 32, WHITE);
        EndDrawing();
    }
    CloseWindow();
}