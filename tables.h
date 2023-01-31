// tables.h

// lookup tables used for quick move generation
// zobrist table used for hash key

#pragma once

#include "common.h"
#include "log.h"
#include "bitboard.h"


namespace Bbot2 {

namespace Tables {
	inline bool initialized = false;

	// lookup tables used to find line masks
	inline bboard rowTable[NUM_SQUARES];
	inline bboard fileTable[NUM_SQUARES];
	inline bboard diagTable[NUM_SQUARES];
	inline bboard antidiagTable[NUM_SQUARES];

	// randomly generated xor boards for [i][] piece herd in [][j] pos
	// sums to board.key
	inline Key zobristTable[NUM_HERDS][NUM_SQUARES];
	inline Key zobristMirror[NUM_HERDS][NUM_SQUARES]; // mirrored across center files

	// lookup tables to find piece moves on a line
	// sight[occupancy of line][position of piece on line]
	// size of [OCC_SIZE][BOARD_SIZE] on heap
	inline const int OCC_SIZE = 1 << BOARD_SIZE;
	inline bboard** rowSight;
	inline bboard** fileSight;

	// lookup table to find adjacent squares
	inline bboard adjacencyTable[NUM_SQUARES];

	void init();

	void gen_rook_tables();
	void gen_bishop_tables();
	void gen_sight_tables();
	void gen_adjacency_table();

	void gen_zobrist_table();

	void close();

} // end namespace Tables

} // end namespace Bbot2