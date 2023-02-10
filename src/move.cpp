// move.cpp

#include "move.h"

using std::string;

namespace Bbot2 {

// Move //

// default
Move::Move()
	: value(0) {}


// construct with from
Move::Move(u_short from) {
	value = from << 8;
}

// construct with from and to
Move::Move(u_short from, u_short to) {
	value = (from << 8) | to;
}

// construct with string
Move::Move(string s) {
	string files(FILE_CHARS);

	s = s.substr(1, s.size() - 1); // crop type

	size_t from = files.find(s[0]); // from col
	s = s.substr(1, s.size() - 1);

	from += BOARD_SIZE * (std::stoi(s) - 1);  // from row
	if (std::stoi(s) == BOARD_SIZE)
		s = s.substr(1, s.size() - 1);
	s = s.substr(1, s.size() - 1);

	size_t to = files.find(s[0]); // to col
	s = s.substr(1, s.size() - 1);

	to += BOARD_SIZE * (std::stoi(s) - 1);  // from row

	value = (static_cast<u_short>(from) << 8) | static_cast<u_short>(to);
}

// set from
void Move::set_from(u_short from) {
	value &= 0xFF;
	value |= (from << 8);
}

// set to
void Move::set_to(u_short to) {
	value &= 0xFF00;
	value |= to;
}

void Move::set_from(u_long from) {
	value &= 0xFF;
	value |= (static_cast<u_short>(from) << 8);
}

// set to
void Move::set_to(u_long to) {
	value &= 0xFF00;
	value |= static_cast<u_short>(to);
}

// get from
u_short Move::get_from() {
	return (value >> 8);
}

// get to
u_short Move::get_to() {
	return value & 0xFF;
}

// to string
// pointerBoard: pass from containing Bbot
string Move::to_string(Piece* pointerBoard[NUM_SQUARES]) {
	string s = "";
	int from = get_from(), to = get_to();

	if (pointerBoard[from] == nullptr) {
		s += "ERR"; // if piece not found
	} else {
		// piece char
		s += BOARD_CHARS[pointerBoard[from]->type];
	}

	// from
	s += FILE_CHARS[from % BOARD_SIZE];
	s += std::to_string(from / BOARD_SIZE + 1);

	// to
	s += FILE_CHARS[to % BOARD_SIZE];
	s += std::to_string(to / BOARD_SIZE + 1);

	return s;
}

// compare value for equality
bool Move::operator==(Move move) {
	return value == move.value;
}

} // end namespace Bbot2
