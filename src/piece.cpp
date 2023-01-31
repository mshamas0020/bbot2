// piece.cpp

#include "piece.h"

using std::string;
using std::vector;

namespace Bbot2 {

// Piece //
	
// default
Piece::Piece() {
	move(0);
	moveBoard = bboard(0);
}

// piece
// herd_: 0 = mouse, 1 = lion, 2 = elephant, (+0 if white, +3 if black)
// k: scalar position
Piece::Piece(int herd_, int k)
	: herd(herd_) {
	type = PIECE_TYPES[herd % NUM_TYPES];
	side = SIDES[herd / NUM_TYPES];

	movesLikeRook = type != 1;
	movesLikeBishop = type != 0;

	move(k);

	isThreatened = false;
	isForced = false;

	moveBoard = bboard(0);
}

////

// find necessary pointers
// pieces and threatMaps passed from containing Board class
void Piece::init(vector<Piece*> pieces[NUM_SIDES], bboard threatMaps[NUM_HERDS]) {
	// herds
	int scaresHerd = (type + 2) % NUM_TYPES + (side ? 0 : NUM_TYPES);
	int scaredByHerd = (type + 1) % NUM_TYPES + (side ? 0 : NUM_TYPES);

	// find siblings
	siblings.clear();
	for (Piece* p : pieces[side])
		// same herd, but not the same object
		if (herd == p->herd && this != p)
			siblings.push_back(p);

	// find pieces threatened by this piece
	scares.clear();
	for (Piece* p : pieces[!side])
		if (p->herd == scaresHerd)
			scares.push_back(p);

	// find threatMaps
	herdMap = &threatMaps[herd];
	scaredByMap = &threatMaps[scaredByHerd];
	scaresMap = &threatMaps[scaresHerd];
}

////

// move to scalar square
void Piece::move(u_short dest) {
	scalar = dest; // scalar
	pos = Bitboard::SQUARES[scalar]; // bboard

	row = dest / BOARD_SIZE;
	col = dest % BOARD_SIZE;
	diag = row - col;
	antidiag = row + col;
}

// update isThreatened state
void Piece::update_threatened() {
	isThreatened = scaredByMap->test(scalar);
}

////

// to string
string Piece::to_string() {
	string s = "";
	s += BOARD_CHARS[type]; // type
	s += FILE_CHARS[scalar % BOARD_SIZE]; // col
	s += std::to_string(scalar / BOARD_SIZE + 1); // row
	return s;
}

// dump info
string Piece::status() {
	return "[" + to_string() + ": isThreatened = " + std::to_string(isThreatened) + ", isForced = " + std::to_string(isForced) + "]";
}

} // end namespace Bbot2