// piece.h

// stores information about each piece
// this includes identifying values, state values, moves,
// and multiple pointers to quickly access relevant pieces/bboards

#pragma once

#include "common.h"
#include "bitboard.h"

namespace Bbot2 {

class Piece {
public:
	PieceType type; // 0 = mouse, 1 = lion, 2 = elephant
	bboard pos; // bitboard - single 1-bit is pos
	u_short scalar = 0; // scalar pos - 0 -> NUM_SQUARES - 1

	Side side; // 0 = white, 1 = black
	bool movesLikeRook; // mice and elephants
	bool movesLikeBishop; // lions and elephants

	int row = 0; // used to identify whether pieces share a line of sight
	int col = 0;
	int diag = 0;
	int antidiag = 0;
	
	int herd; // unique index for group sharing side and type:		MW, LW, EW, MB, LB, EB
	bboard* herdMap; // threat map of own herd
	bboard* scaredByMap; // threat map of herd that scares piece:	LB, EB, MB, LW, EW, MW
	bboard* scaresMap; // threat map of herd that piece scares:		EB, MB, LB, EW, MW, LW
	bboard adjacent; // all squares one king's move from piece
	bool isThreatened; // is adjacent to a threatening piece
	bool isForced; // has a legal move to escape threat

	bool schedSightUpdate = true; // if true, sight will be re-checked next time moveBoard is generated
	bboard sightBoard;
	bboard moveBoard; // set of moves (pseudo-legal inside search, legal outside search)

	std::vector<Piece*> scares; // points to pieces that this piece scares
	std::vector<Piece*> siblings; // points to pieces that shares herd

	Piece();
	Piece(int herd_, int k);

	void init(std::vector<Piece*> pieces[NUM_SIDES], bboard threatMaps[NUM_HERDS]);

	void move(u_short dest);
	void update_threatened();

	std::string to_string();
	std::string status();
};

} // end namespace Bbot2