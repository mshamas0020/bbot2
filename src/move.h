// move.h

// condense 'from' and 'to' scalar squares into a two byte value
// move does not include any information about the piece it is moving
// therefore, must be responsible in maintaining an accurate board
// because playing a move with an empty 'from' square will break

#pragma once

#include "common.h"
#include "piece.h"

namespace Bbot2 {

class Move {
public:
	u_short value;

	Move();
	Move(u_short from_);
	Move(u_short from_, u_short to_);
	Move(std::string s);

	void set_from(u_short from_);
	void set_to(u_short to_);
	void set_from(u_long from_);
	void set_to(u_long to_);

	u_short get_from();
	u_short get_to();

	std::string to_string(Piece* pointerBoard[NUM_SQUARES]);

	//Move& operator=(Move move);
	bool operator==(Move move);
};

} // end namespace Bbot2
