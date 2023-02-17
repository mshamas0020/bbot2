// board.h

// class used to describe barca board state
// manipulated by/informs Game and Bbot

#pragma once

#include "common.h"
#include "log.h"
#include "piece.h"
#include "move.h"
#include "line.h"
#include "tables.h"
#include "bitboard.h"


namespace Bbot2 {

class Board {
public:
	// Starting position
	// 'M/m' - mouse (rook), 'L/l' - lion (bishop), 'E/e' - elephant (queen), '.' - empty square
	// Uppercase pieces are white.Lowercase is black.
	// Lower left corner is a1.Upper right is j10.
	std::string DEFAULT_START_POS =
		". . . . e e . . . ."
		". . . l m m l . . ."
		". . . . . . . . . ."
		". . . . . . . . . ."
		". . . . . . . . . ."
		". . . . . . . . . ."
		". . . . . . . . . ."
		". . . . . . . . . ."
		". . . L M M L . . ."
		". . . . E E . . . .";

	// usually overrided by .ini in settings()

private:
	std::string WATERING_HOLES_STR = // watering holes
		". . . . . . . . . ."
		". . . . . . . . . ."
		". . . . . . . . . ."
		". . . 1 . . 1 . . ."
		". . . . . . . . . ."
		". . . . . . . . . ."
		". . . 1 . . 1 . . ."
		". . . . . . . . . ."
		". . . . . . . . . ."
		". . . . . . . . . .";

public:
	bool initialized = false;

	bboard wateringHoles; // bboard representation of wh
	std::vector<int> whScalars; // scalar list of wh

	std::vector<Piece*> pieces[NUM_SIDES]; // array of Piece vectors, arranged by [side][piece]
	Piece* pointerBoard[NUM_SQUARES]; // array where the pointer to a piece is placed at its scalar's index
	Piece* startPointerBoard[NUM_SQUARES];; // starting position of pointerBoard - remains unchanged

	Key CHANGE_SIDE;
	Key key; // full zobrist key used for tt hash
	u_long hash; // reduced key for tt index
	Key KEY_MASK; // masks key to size usable for transpositionTable

	bool isSideForced = false; // true when one or more piece of current side is threatened and it has a legal move - this move is then forced
	int ply = 0; // moves played in game so far, in plies
	bool sideToMove = 0; // 0 = white, 1 = black

	bboard occupancy; // occupancy of all pieces
	bboard occupancyBySide[NUM_SIDES]; // occupancy of white [0] and black [1]

private:
	bboard threatMaps[NUM_HERDS]; // union of adjacent squares for each herd

public:
	void from_string(std::string s);

	void settings();
	void init();
	void reset();

	void update_move_sets();
	void quick_move_sets();

	void move_piece(Piece* p, u_short dest);

	u_long key_to_hash(Key key_);

	void close();

	std::string move_to_string(Move move);
	std::string line_to_string(Line line);
	std::string line_to_string(Line line, int maxLength);
	std::string to_string();
	void print();

	void __DEBUG(bool condition, std::string message);

private:
	void add_piece(int herd, int k);
	void init_WH();
	void init_pieces();

	void schedule_sight_updates(Piece* p);
	void update_piece_sight(Piece* p);
	void update_piece_moves(Piece* p);

	void remove_from_occupancy(Piece* p);
	void add_to_occupancy(Piece* p);
	void update_threats(Piece* p);

	void init_zobrist_values();
	void update_zobrist_key(Piece* p, u_short dest);
};

} // end namespace Bbot2
