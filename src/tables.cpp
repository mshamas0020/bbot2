// tables.cpp

#pragma once

#include "tables.h"

namespace Bbot2 {

namespace Tables {

// allocate, generate all tables
void init() {
	// allocate sight tables
	rowSight = new bboard * [OCC_SIZE];
	fileSight = new bboard * [OCC_SIZE];

	for (int i = 0; i < OCC_SIZE; i ++) {
		rowSight[i] = new bboard[BOARD_SIZE];
		fileSight[i] = new bboard[BOARD_SIZE];
	}

	// generate tables
	gen_rook_tables();
	gen_bishop_tables();
	gen_sight_tables();
	gen_adjacency_table();

	gen_zobrist_table();

	initialized = true;
}

// lookup tables rows and files, indexed by scalar position
// table size: 100 * 16 bytes = 1.6 KB each
void gen_rook_tables() {
	bboard row, file;

	// rows
	for (int i = 0; i < BOARD_SIZE; i ++) {
		// create row
		row = Bitboard::ROW_1;
		row <<= i * BOARD_SIZE;

		// put into intersecting rowTable entry
		for (int j = 0; j < BOARD_SIZE; j ++)
			rowTable[i * BOARD_SIZE + j] = row;
	}

	// files
	for (int i = 0; i < BOARD_SIZE; i ++) {
		// create file
		file = Bitboard::FILE_A;
		file <<= i;

		// put into intersecting fileTable entry
		for (int j = 0; j < BOARD_SIZE; j ++)
			fileTable[j * BOARD_SIZE + i] = file;
	}
}

// lookup tables for bishop lines assuming empty board
// diagonal (/), antidiagonal (\)
// table size: 100 * 16 bytes = 1.6 KB each
void gen_bishop_tables() {

	// create diags/antidiags
	bboard diags[BOARD_SIZE * 2 - 1];
	bboard antidiags[BOARD_SIZE * 2 - 1];

	for (int i = 0; i < BOARD_SIZE * 2 - 1; i ++) {
		// diags
		// indexes tracing the perimeter top left -> bottom left -> bottom right
		int n = i < BOARD_SIZE ? (BOARD_SIZE - i - 1) * BOARD_SIZE : i - BOARD_SIZE + 1;
		diags[i].set(n, 1);

		// go up and right by one, repeat until edge is hit
		while (n < BOARD_SIZE * (BOARD_SIZE - 1) && n % BOARD_SIZE != BOARD_SIZE - 1) {
			n += BOARD_SIZE + 1;
			diags[i].set(n, 1);
		}

		// antidiags
		// indexes tracing the perimeter bottom left -> bottom right -> top right
		n = i < BOARD_SIZE ? i : (i - BOARD_SIZE + 1) * BOARD_SIZE + BOARD_SIZE - 1;
		antidiags[i].set(n, 1);

		// go up and left by one, repeat until edge is hit
		while (n < BOARD_SIZE * (BOARD_SIZE - 1) && n % BOARD_SIZE != 0) {
			n += BOARD_SIZE - 1;
			antidiags[i].set(n, 1);
		}
	}

	// for each scalar position, assign diagTable/antidiagTable[k] the diag/antidiag that it intersects with
	int x, y;
	for (int i = 0; i < NUM_SQUARES; i ++) {
		x = i % BOARD_SIZE;
		y = i / BOARD_SIZE;

		diagTable[i] = diags[x - y + BOARD_SIZE - 1];
		antidiagTable[i] = antidiags[x + y];
	}
}

// lookup tables for sight of pieces
// - rows/diags/antidiags all map to rowSight, files use fileSight
// - first index is mask of relevant occupancy line, transformed to first row
// - secondary index is position of piece on the line
// - trace the squares in either direction of the piece to give sight
// - sight row is then stretched to full board
// - must be masked again to give specific line
// - check Board::update_piece_sight to see how this is used
// table size: 1024 * 10 * 16 bytes = 163.8 KB each
void gen_sight_tables() {
	for (int occ = 0; occ < OCC_SIZE; occ ++) {
		// edge bits can be discarded. if they are unseen, an occupancy mask will remove them
		std::bitset<10> occBin(occ);

		for (int i = 0; i < BOARD_SIZE; i ++) {

			// don't generate sight for unoccupied squares
			if (!occBin[i])
				continue;

			// cannot see own square
			rowSight[occ][i].set(i, 0);
			fileSight[occ][i].set(i * BOARD_SIZE, 0);

			// trace left, stopping at first occupied square
			int r = i - 1;

			while (r >= 0) {
				if (occBin[r])
					break;

				rowSight[occ][i].set(r, 1);
				fileSight[occ][i].set(r * BOARD_SIZE, 1);
				r --;
			}

			// trace right
			r = i + 1;

			while (r < BOARD_SIZE) {
				if (occBin[r])
					break;

				rowSight[occ][i].set(r, 1);
				fileSight[occ][i].set(r * BOARD_SIZE, 1);
				r ++;
			}

			Bitboard::stretch_row(&rowSight[occ][i]);
			Bitboard::stretch_file(&fileSight[occ][i]);
		}
	}
}


// table of all possible sets of adjacent squares, indexed by scalar square
// used for threat maps
// table size: 100 * 16 bytes = 1.6 KB
void gen_adjacency_table() {

	int x, y;
	for (int i = 0; i < NUM_SQUARES; i ++) {
		// create bboard
		bboard b(0);
		x = i % BOARD_SIZE;
		y = i / BOARD_SIZE;

		// set adjacent bits in all directions, unless over edge of board
		if (y > 0 && x > 0)
			b.set(i - BOARD_SIZE - 1); //NW

		if (y > 0)
			b.set(i - BOARD_SIZE); //N

		if (y > 0 && x < BOARD_SIZE - 1)
			b.set(i - BOARD_SIZE + 1); //NE

		if (x > 0)
			b.set(i - 1); //W

		if (x < BOARD_SIZE - 1)
			b.set(i + 1); //E

		if (y < BOARD_SIZE - 1 && x > 0)
			b.set(i + BOARD_SIZE - 1); //SW

		if (y < BOARD_SIZE - 1)
			b.set(i + BOARD_SIZE); //S

		if (y < BOARD_SIZE - 1 && x < BOARD_SIZE - 1)
			b.set(i + BOARD_SIZE + 1); //SE

		// assign
		adjacencyTable[i] = b;
	}
}

void close() {
	// deallocate sight tables
	for (int i = 0; i < OCC_SIZE; i ++) {
		delete[] rowSight[i];
		delete[] fileSight[i];
	}

	delete[] rowSight;
	delete[] fileSight;
}

// generate table of xor boards used to make zobrist key
// table size: 6 * 100 * 16 bytes = 9.6 KB
void gen_zobrist_table() {
	for (int i = 0; i < NUM_HERDS; i ++) {
		for (int j = 0; j < NUM_SQUARES; j ++) {
			zobristTable[i][j] = Bitboard::random();
		}
	}
}

} // end namespace Tables

} // end namespace Bbot2