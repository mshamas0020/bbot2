// line.h

// Line: array of moves, size MAX_LINE_LEN
// LineVector extends Line but has no size limit

#pragma once

#include "common.h"
#include "move.h"


namespace Bbot2 {

static const int MAX_LINE_LEN = 16;

class Line {
public:

	int length = 0;
	Move moves[MAX_LINE_LEN];

	Line() = default;
	Line(std::string s);

	virtual void append(Move move);
	virtual Move get_move(int i);

	void delete_first();

	std::string to_string(Piece* pointerBoard[NUM_SQUARES]);
	std::string to_string(int startPly, Piece* pointerBoard[NUM_SQUARES]);
	std::string to_string(int startPly, int maxLength, Piece* pointerBoard[NUM_SQUARES]);

	Line& operator=(Line line);
};


// uses a vector list instead of an array
// not optimal for use within a search, but capable of holding many more moves
class LineVector : public Line {
public:
	std::vector<Move> movesVector;

	void append(Move move) override;
	Move get_move(int i) override;
};

} // end namespace Bbot2