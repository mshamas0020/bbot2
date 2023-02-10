// line.cpp

#include "line.h"

using std::string;

namespace Bbot2 {

// Line //
Line::Line(string s) {
	string t;

	for (size_t i = 0; i < s.size(); i ++) {
		for (int j = 0; j < NUM_HERDS; j ++) {
			if (s[i] == BOARD_CHARS[j]) {
				t = s.substr(i);
				t = t.substr(0, t.find(' '));

				append(Move(t));
				i += t.size();
				break;
			}
		}
	}
}

////

// append move
void Line::append(Move move) {
	moves[length] = move;
	length ++;
}

// get move at index i
// this is only called internally for compatibility with LineVector subclass
Move Line::get_move(int i) {
	return moves[i];
}

// delete first move
void Line::delete_first() {
	for (int i = 0; i < length - 1; i ++)
		moves[i] = moves[i + 1];

	length --;
}

// assignment, copy moves and length
Line& Line::operator=(Line line) {
	for (int i = 0; i < line.length; i ++)
		moves[i] = line.moves[i];

	length = line.length;

	return *this;
}

// line to string
// full length of line
// beginning of game
string Line::to_string(Piece* pointerBoard[NUM_SQUARES]) {
	return to_string(0, pointerBoard);
}

// line to string
// full length of line
string Line::to_string(int startPly, Piece* pointerBoard[NUM_SQUARES]) {
	return to_string(startPly, length, pointerBoard);
}

// line to string with given max length
// follows the given line using a temporary pointer board to track piece movement
// startPly - used for move numbering. use INT_MAX for no move numbers
string Line::to_string(int startPly, int maxLength, Piece* pointerBoard[NUM_SQUARES]) {

	maxLength = std::min(maxLength, length);
	string s = "";

	// copy real pointerBoard to temp
	Piece* tempPointerBoard[NUM_SQUARES];
	std::copy(pointerBoard, &pointerBoard[NUM_SQUARES], tempPointerBoard);

	for (int i = 0; i < maxLength; i ++) {

		if (startPly < INT_MAX) {
			// move number if starting with black
			if (i == 0 && startPly % NUM_SIDES > 0) {
				s += std::to_string((i + startPly) / NUM_SIDES + 1) + "... ";
			}

			// label move number when white is playing
			if ((i + startPly) % NUM_SIDES == 0) {
				s += std::to_string((i + startPly) / NUM_SIDES + 1) + ". ";
			}
		}

		

		// move string
		s += get_move(i).to_string(tempPointerBoard);

		// move piece on temp pb
		tempPointerBoard[get_move(i).get_to()] = tempPointerBoard[get_move(i).get_from()];
		tempPointerBoard[get_move(i).get_from()] = nullptr;

		if (i < length - 1)
			s += " ";
	}

	return s;
}

////////////////////////////////

// LineVector //

void LineVector::append(Move move) {
	movesVector.push_back(move);
	length ++;
}

// get move at index i
Move LineVector::get_move(int i) {
	return movesVector[i];
}

} // end namespace Bbot2
