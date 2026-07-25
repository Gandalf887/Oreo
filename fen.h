#pragma once
#include "position.h"
#include <string>

// ============================================================
// PROCEDURES & FUNCTIONS
// ============================================================

Position parse_fen(const std::string &fen);
std::string starting_fen(); 