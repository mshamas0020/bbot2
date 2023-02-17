// common.h

// includes, types, constants used by most classes

#pragma once

#include <iostream>
#include <vector>
#include <string>
#include <algorithm>
#include <format>
#include <bitset>
#include "../external/SimpleIni/SimpleIni.h"

namespace Bbot2 {

//// CONFIG SETTINGS ////

// log
const bool LOG = false; // print basic info
const bool LOG_VERBOSE = false; // print more info
const bool DEBUG = true; // when throwing an exception, print information that may be helpful

// .ini
inline std::string INI_FILE = "SETTINGS.ini";

////

// simpleINI instance shared by most files
inline CSimpleIniA ini;

// size of hash table. primarily used as a transposition table, but also contains game history 
// larger values give better performance at a higher memory demand
// must be form 2^n for hashing purposes
// shared by Bbot (for TT) and Board (for Zobrist keys). important that this is kept global
inline unsigned long TT_ALLOC;

////


//// DO NOT EDIT ////

// types

typedef unsigned long u_long;
typedef unsigned short u_short;
typedef unsigned __int8 u_byte;

typedef std::bitset<100> bboard;
typedef bboard Key;

// constants

const int BOARD_SIZE = 10; // length and width of board
const int NUM_SIDES = 2; // white and black
const int NUM_TYPES = 3; // mouse, lion, elephant
const int PIECES_PER_HERD = 2; // herd - shares side and type

constexpr int NUM_SQUARES = BOARD_SIZE * BOARD_SIZE; // total squares on board
const int NUM_WH = 4; // number of watering holes
const int NUM_WH_TO_WIN = 3; // number of watering holes to win

constexpr int NUM_HERDS = NUM_SIDES * NUM_TYPES;
constexpr int PIECES_PER_SIDE = PIECES_PER_HERD * NUM_TYPES;
constexpr int NUM_PIECES = PIECES_PER_SIDE * NUM_SIDES; // total


// Sides
enum Side : bool { WHITE = false, BLACK = true };
const Side SIDES[NUM_SIDES] = { WHITE, BLACK };
const std::string SIDE_NAMES[NUM_SIDES] = { "WHITE", "BLACK" };

// PieceTypes
enum PieceType : int { MOUSE = 0, LION = 1, ELEPHANT = 2 };
const PieceType PIECE_TYPES[NUM_TYPES] = { MOUSE, LION, ELEPHANT };

// Outcomes
// DRAW_REPETITION - a position has occurred in the played line more than REPETITIONS_TO_DRAW times
// DRAW_MOVE_LIMIT - used in testing to declare a draw if a move limit has been reached
enum Outcome : int { OUTCOME_NONE, WIN_WHITE, WIN_BLACK, DRAW_REPETITION, DRAW_MOVE_LIMIT };

// used for string representation of board
const char FILE_CHARS[BOARD_SIZE + 1] = "abcdefghij";
const char BOARD_CHARS[NUM_HERDS + 2] = "MLEmle.";

} // end namespace Bbot2
