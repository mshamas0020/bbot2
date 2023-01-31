// bitboard.h

// utilities for bitboard representation
// each bit (0-99) represents a square of the board
// least sig bit (b[0]) = a1, most sig bit (b[99]) = j10
// 90 . . . . . . . . 99
//  . . . . . . . . . .
//  . . . . . . . . . .
//  . . . . . . . . . .
//  . . . . . . . . . .
//  . . . . . . . . . .
//  . . . . . . . . . .
//  . . . . . . . . . .
// 10 . . . . . . . . .
//  0 1 . . . . . . . 9
// 
// used extensively for piece position, occupancy, quick move generation, and more
// implemented with std::bitset<100>

// commonly, if a square of the board is referred to instead by its index,
// this will be called a scalar value (k), also 0-99

#pragma once


#include "common.h"
#include "log.h"
#include <intrin.h>
#include <iostream>
#include <cstdlib>
#include <string>
#include <functional>

namespace Bbot2 {

namespace Bitboard {
	inline bboard ROW_1;
	inline bboard FILE_A;
	inline bboard SQUARES[NUM_SQUARES];

	void init();

	bboard from_ullong(unsigned __int64 v1, unsigned __int64 v2);
	bboard from_string(std::string s);
	bboard square(int n);
	bboard random();

	bool scan_forward(u_long* result, bboard* p);
	bool scan_reverse(u_long* result, bboard* p);

	void collapse_to_row(bboard* p);
	void collapse_to_file(bboard* p);
	void file_to_row(bboard* p);
	void stretch_row(bboard* p);
	void stretch_file(bboard* p);

	std::string to_hex(bboard b);
	std::string to_string(bboard b);
	void print(bboard b);
}

} // end namespace Bbot2