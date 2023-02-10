// bitboard.cpp

#include "bitboard.h"

using std::string;
using std::format;


namespace Bbot2 {

namespace Bitboard {

// initialize values
void init() {

	// first row
	ROW_1 = from_ullong(0, (unsigned __int64) pow(2, BOARD_SIZE) - 1);

	// first file
	FILE_A = square(0);
	for (int i = 1; i < BOARD_SIZE; i++) {
		FILE_A |= FILE_A << BOARD_SIZE;
	}

	// all single-bit bitboards
	for (int i = 0; i < NUM_SQUARES; i ++) {
		SQUARES[i] = square(i);
	}
}

////

// create bitboard from two int64s
// first 28 bits of v1 are ignored
// i.e., max value is 0x0000000FFFFFFFFF, 0xFFFFFFFFFFFFFFFF
bboard from_ullong(unsigned __int64 v1, unsigned __int64 v2) {
	bboard b(v1);
	b <<= 64;
	b |= v2;
	return b;
}

// create bitboard from string
// '1' = 1, '.' = 0, all unrecognized chars ignored
bboard from_string(string s) {
	bboard b(0);
	int len = 0;

	// iteration through string
	for (char c : s) {
		// 0, no assignment needed
		if (c == '.')
			len ++;

		// 1, assign
		if (c == '1') {
			b.set(NUM_SQUARES - (len / BOARD_SIZE + 1) * BOARD_SIZE + len % BOARD_SIZE, 1);
			len ++;
		}

		// end of bboard
		if (len >= NUM_SQUARES)
			break;
	}

	return b;
}

// create single-bit bitboard using index of bit (scalar)
// n: 0 -> NUM_SQUARES - 1
bboard square(int n) {
	bboard b(0);
	b.set(n, 1);
	return b;
}

// random bitboard
bboard random() {
	int w = 15;
	
	// RAND_MAX is typically 15-bits long (0x7FFF)
	// if not, calculate MSB

	if (RAND_MAX != (1 << w) - 1) {
		int r = RAND_MAX;
		w = 1;
		while (r >>= 1) w ++;
	}

	bboard b(rand());

	// add rand(), shift, and repeat
	for (int i = w; i < NUM_SQUARES; i += w) {
		b <<= w;
		b |= rand();
	}

	return b;
}

////

// get least significant non-zero bit
// returns true if bit is found
bool scan_forward(u_long* result, bboard* b) {
	unsigned __int64* n = reinterpret_cast<unsigned __int64*>(b);

	// if first 64 bits are empty, scan the next 64
	if (_BitScanForward64(result, *n)) {
		return true;
	} else if (_BitScanForward64(result, *(n + 1))) {
		*result += 64;
		return true;
	}

	return false;
}

// get most significant non-zero bit
// returns true if bit is found
bool scan_reverse(u_long* result, bboard* b) {
	unsigned __int64* n = reinterpret_cast<unsigned __int64*>(b);

	// if first 64 bits are empty, scan the next 64
	if (_BitScanReverse64(result, *(n + 1))) {
		*result += 64;
		return true;
	} else if (_BitScanReverse64(result, *n)) {
		return true;
	}

	return false;
}

// any occupancy in a file will put a 1 in its intersection with the first row
// all other rows masked out
void collapse_to_row(bboard* b) {
	*b |= *b >> 5 * BOARD_SIZE;
	*b |= *b >> 2 * BOARD_SIZE;
	*b |= *b >> BOARD_SIZE;
	*b |= *b >> BOARD_SIZE;
	*b &= ROW_1;
}

// any occupancy in a row will put a 1 in its intersection with the first column
// all other files masked out
void collapse_to_file(bboard* b) {
	*b |= *b >> 5;
	*b |= *b >> 2;
	*b |= *b >> 1;
	*b |= *b >> 1;
	*b &= FILE_A;
}

// transforms first file to first row
// assumes input has only first row occupied
void file_to_row(bboard* b) {
	*b |= *b >> BOARD_SIZE - 1;
	*b |= *b >> (BOARD_SIZE - 1) * 2;
	*b |= *b >> (BOARD_SIZE - 1) * 4;
	*b |= *b >> (BOARD_SIZE - 1) * 2;
	*b &= ROW_1;
}

// copy first row into all other rows
// assumes input has only first row occupied
void stretch_row(bboard* b) {
	*b |= *b << 5 * BOARD_SIZE;
	*b |= *b << 2 * BOARD_SIZE;
	*b |= *b << BOARD_SIZE;
	*b |= *b << BOARD_SIZE;
}

// copy first file into all other files
// assumes input has only first file occupied
void stretch_file(bboard* b) {
	*b |= *b << 5;
	*b |= *b << 2;
	*b |= *b << 1;
	*b |= *b << 1;
}
	
// convert bitboard to hex representation, returned as string
string to_hex(bboard b) {
	return format("{:#9x}{:16x}", (b >> 64).to_ullong(), (b & bboard(0xFFFFFFFFFFFFFFFF)).to_ullong());
}
	
// convert bitboard to string, "1" = 1, "." = 0
string to_string(bboard b) {
	string s = "";

	for (int i = 0; i < NUM_SQUARES; i++) {

		// row labels
		if (i % BOARD_SIZE == 0) {
			char s2[7];
			sprintf_s(s2, "%2d |  ", BOARD_SIZE - i / BOARD_SIZE);
			s.append(s2);
		}

		// bit to char
		s.append(b[NUM_SQUARES - (i / BOARD_SIZE + 1) * BOARD_SIZE + i % BOARD_SIZE] ? "1 " : ". ");

		// end of row
		if (i % BOARD_SIZE == BOARD_SIZE - 1)
			s.append("\n");
	}

	// file labels
	s += "      ";

	for (int i = 0; i < BOARD_SIZE; i++)
		s += "__";

	s += "\n      ";

	for (int i = 0; i < BOARD_SIZE; i++) {
		s += FILE_CHARS[i];
		s += " ";
	}

	return s;
}

// print
void print(bboard b) {
	__PRINT(to_string(b) + "\n");
}

} // end namespace Bitboard

} // end namespace Bbot2
