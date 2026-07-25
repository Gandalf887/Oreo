#pragma once
#include "types.h"
#include "move.h"

// the type of score stored in the table
enum TTFlag
{
    TT_EXACT, // exact score
    TT_ALPHA, // upper bound (score <= alpha)
    TT_BETA   // lower bound (score >= beta)
};

// one entry in the transposition table
struct TTEntry
{
    uint64_t hash; // zobrist hash of the posi
    int depth;     // depth this was searched to4
    int score;     // the score
    Move best;     // best move found
    TTFlag flag;   // type of score
    bool valid;    // is this entry occupied
};

// table size - it think 256MB should be enough can upgrade since 32gb ram
const int TT_SIZE = 1 << 22; // 4 millionn entries makjes 256MB

struct TranspositionTable
{
    TTEntry entries[TT_SIZE];

    void clear() // clear the table
    {
        for (int i = 0; i < TT_SIZE; i++)
        {
            entries[i].valid = false;
        }
    }

    void store(uint64_t hash, int depth, int score, Move best, TTFlag flag)
    {
        int index = hash % TT_SIZE;
        entries[index] = {hash,
                          depth,
                          score,
                          best,
                          flag,
                          true};
    }

    TTEntry *probe(uint64_t hash)
    {
        int index = hash % TT_SIZE;
        if (entries[index].valid && entries[index].hash == hash)
            return &entries[index];
        return nullptr;
    }
};

// gloabl transposition table
extern TranspositionTable tt;