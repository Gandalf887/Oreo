Oreo is my UCI-compatible chess engine written in C++ using a bitboard-based board representation. 
It supports both a terminal UCI mode (for use with chess GUIs like Cutechess or Arena as well as online chess platforms like Lichess) and a native graphical interface I built with Raylib. 
The engine plays at a competitive amateur level, featuring iterative deepening alpha-beta search, a Zobrist-hashed transposition table, an opening book, and real-time pondering. 
Pretty cool stuff :D

Download the source files folder and unzip it, make sure each file is in the same folder. 
Open the files in your IDE and then open the terminal. Run the following command to compile the code: 
clang++ main.cpp fen.cpp display.cpp movegen.cpp makeunmake.cpp legal.cpp eval.cpp search.cpp zobrist.cpp gui.cpp transposition.cpp uci.cpp book.cpp -o Oreo1.16.1.9.exe -L"C:/Users/"YourName"/.raylib-5.0_win64_mingw-w64/lib" -I"C:/Users/"YourName"/.raylib-5.0_win64_mingw-w64/include" -lraylib -lopengl32 -lgdi32 -lwinmm
Then run the file using: ./Oreo.1.16.1.9

1.16.1.9 is the version toggled for GUI usage (simply by commenting in the main.cpp file).
1.16.1.10 is the version toggled for UCI usage.

Enjoy!
