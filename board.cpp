// board.cpp

#include "board.h"

using std::string;
using std::vector;


namespace Bbot2 {

// Board //

// constructor, default
Board::Board() {
	from_string(DEFAULT_START_POS);
}

// construct with alternate position
Board::Board(string s) {
	from_string(s);
}

// convert string to pos
// key:
	// .  ...  j10
	// :  :::  :
	// a1 ...  .
	// 'M' - mouse, 'L' - lion, 'E' - elephant, '.' - empty
	// uppercase - white, lowercase - black
	// unrecognized characters ignored
void Board::from_string(string s) {
	pieces[WHITE].clear();
	pieces[BLACK].clear();

	int k, len = 0;

	// iterate through string, find matching char from BOARD_CHARS, and create Piece
	// BOARD_CHARS = "MLEmle."
	for (char c : s) {
		for (int herd = 0; herd < NUM_HERDS + 1; herd ++) {
			if (c == BOARD_CHARS[herd]) {
				if (herd >= NUM_HERDS) {
					len ++;
					break;
				}

				k = len % BOARD_SIZE + (BOARD_SIZE - 1 - len / BOARD_SIZE) * BOARD_SIZE;
				add_piece(herd, k);
				len ++;
			}
		}

		if (len >= NUM_SQUARES)
			break;
	}

	// if not initalialized, full init is still expected later
	// if already initialized, this is all that's required
	if (initialized) {
		set_up_pieces();
		update_move_sets();
	}
}

////

// set up board elements
// gets moves, threats, etc.
void Board::init() {
	if (!Tables::initialized)
		Tables::init();

	sideToMove = 0;

	// set up watering holes
	set_up_WH();

	// set up pieces
	set_up_pieces();

	// set up zobrist key/values
	init_zobrist_values();

	// update move sets
	update_move_sets();

	// get startPointerBoard from pointerBoard
	std::copy(pointerBoard, &pointerBoard[NUM_SQUARES], startPointerBoard);
}

void Board::reset() {
	close();
	from_string(DEFAULT_START_POS);
	init();
}

////

// update move set bboards
// slower than quick_move_sets, but more complete
// i.e., clears moveBoards for illegal pieces and fully updates threats/forced
void Board::update_move_sets() {

	isSideForced = false;
	for (Side side : SIDES) {
		for (Piece* p : pieces[side]) {
			// check if threatened
			p->update_threatened();

			// update moves/if forced
			p->isForced = false;
			update_piece_moves(p);

			if (side == SIDES[sideToMove]) {
				// check if side is forced
				isSideForced |= p->isForced;
			} else {
				// clear moves for opposite side
				p->moveBoard.reset();
			}
		}
	}

	// clear if side is forced and piece is not
	if (isSideForced)
		for (Piece* p : pieces[sideToMove])
			if (!p->isForced)
				p->moveBoard.reset();
}

// update move set bboards
// less complete--doesn't clear moves for illegal pieces--but faster for move generation
void Board::quick_move_sets() {
	isSideForced = false;

	for (Piece* p : pieces[sideToMove]) {
		p->isForced = false;

		if (isSideForced && !p->isThreatened)
			continue;

		update_piece_moves(p);
		isSideForced |= p->isForced;
	}
}

////

// move piece on board and update
void Board::move_piece(Piece* p, u_short dest) {
	//__DEBUG(p == nullptr, "Could not get piece data.");

	// remove from pointer board
	pointerBoard[p->scalar] = nullptr;

	// update key
	update_zobrist_key(p, dest);

	// remove from occupancy
	remove_from_occupancy(p);

	// schedule sight updates
	schedule_sight_updates(p);

	// move
	p->move(dest);

	// add back to pointer board
	pointerBoard[dest] = p;

	// add back to occupancy
	add_to_occupancy(p);

	// schedule sight updates
	schedule_sight_updates(p);

	// update threats
	update_threats(p);

	sideToMove = !sideToMove; // flip side
}

////

// close
void Board::close() {
	// deallocate pieces
	for (Side side : SIDES)
		for (Piece* p : pieces[side])
			delete p;

	initialized = false;
}

////

// move to string - "[Type]a1a1"
string Board::move_to_string(Move move) {
	return move.to_string(pointerBoard);
}

// line to string - lists moves, separated by move numbers
string Board::line_to_string(Line line) {
	return line.to_string(ply, pointerBoard);
}

// line to string with max length
string Board::line_to_string(Line line, int maxLength) {
	return line.to_string(ply, maxLength, pointerBoard);
}

// current position to string
string Board::to_string() {
	string s = "";

	for (int i = 0; i < NUM_SQUARES; i++) {

		// row labels and left border
		if (i % BOARD_SIZE == 0) {
			char s2[7];
			sprintf_s(s2, "%2d |  ", BOARD_SIZE - i / BOARD_SIZE);
			s.append(s2);
		}

		// i to scalar pos
		int scalar = NUM_SQUARES - (i / BOARD_SIZE + 1) * BOARD_SIZE + i % BOARD_SIZE;

		// pieces or '.'
		if (pointerBoard[scalar] != nullptr) {
			s += BOARD_CHARS[pointerBoard[scalar]->herd];
			s += " ";
		} else {
			s += ". ";
		}

		// watering holes
		if (wateringHoles[scalar]) {
			s.replace(s.length() - 3, 1, "(");

			if (s[s.length() - 2] == '.')
				s.replace(s.length() - 2, 1, " ");

			s.replace(s.length() - 1, 1, ")");
		}

		// end of row
		if (i % BOARD_SIZE == BOARD_SIZE - 1)
			s += "\n";
	}

	// bottom border
	s += "      ";

	for (int i = 0; i < BOARD_SIZE; i++)
		s += "__";

	// file labels
	s += "\n      ";

	for (int i = 0; i < BOARD_SIZE; i++) {
		s += FILE_CHARS[i];
		s += " ";
	}

	s += "\n";

	return s;
}


// print current position
void Board::print() {
	__PRINT(to_string() + "\n");
}

////////////////////////////////

// allocate new piece, add to pieces
// herd: 0 = mouse, 1 = lion, 2 = elephant, (+0 if white, +3 if black)
// k: scalar position
void Board::add_piece(int herd, int k) {
	pieces[herd / NUM_TYPES].push_back(new Piece(herd, k));
}

// generate necessary bboards/values for watering holes
void Board::set_up_WH() {
	// get watering holes from string
	wateringHoles = Bitboard::from_string(WATERING_HOLES_STR);

	// get scalar values from watering holes
	for (int i = 0; i < NUM_SQUARES; i ++)
		if (wateringHoles[i])
			whScalars.push_back(i);
}

// initialize pieces, pointerBoard, and threatMaps
void Board::set_up_pieces() {
	// clear pointer board
	for (int i = 0; i < NUM_SQUARES; i ++)
		pointerBoard[i] = nullptr;

	// clear occupancy boards
	occupancy = bboard(0);
	for (Side side : SIDES)
		occupancyBySide[side] = bboard(0);

	// find siblings and threatening pieces
	for (Side side : SIDES)
		for (Piece* p : pieces[side])
			p->init(pieces, threatMaps);

	// moving all pieces to same position initializes occupancy/adjacency/threat maps
	for (Side side : SIDES)
		for (Piece* p : pieces[side])
			move_piece(p, p->scalar);
}

////

// check what pieces will need to have their sight updated on next move-gen
// this checks all pieces to see if they share a row/col or diag/antidiag with the piece being moved
// if this is the case, sight needs to be updated
// this must be called before and the piece has move to its destination
void Board::schedule_sight_updates(Piece* p) {

	for (Side side : SIDES) {
		for (Piece* q : pieces[side]) {
			int m = 0;
			if (q->movesLikeRook && (p->row == q->row || p->col == q->col))
				q->schedSightUpdate = true;

			if (q->movesLikeBishop && (p->diag == q->diag || p->antidiag == q->antidiag))
				q->schedSightUpdate = true;
		}
	}
}

// Get pseudo-legal moves of piece
// (i.e., all squares that can be seen and reached by a piece--
// may not be legal if other pieces are threatened)

// rowSight/fileSight lookup provides quick move gen
// see below for an example of a bishop searching a diagonal on F5
//
	//	initial position		masked to diagonal		reduced to row
	//	. . . . 1 . . . . .		. . . . . . . . . .		. . . . . . . . . .
	//	1 . . 1 . 1 1 . . .		1 . . . . . . . . .		. . . . . . . . . .
	//	. . . . . . . . . .		. . . . . . . . . .		. . . . . . . . . .
	//	. . 1 . 1 . . . . .		. . 1 . . . . . . .		. . . . . . . . . .
	//	. . . . . . . . . .  >	. . . . . . . . . .  >	. . . . . . . . . .  >
	//	. . . .[1]. 1 . . .  >	. . . .[1]. . . . .  >	. . . . . . . . . .  >
	//	. . . . . . . . . .		. . . . . . . . . .		. . . . . . . . . .
	//	. . . . . . . . . .		. . . . . . . . . .		. . . . . . . . . .
	//	. . . 1 1 . . . . .		. . . . . . . . . .		. . . . . . . . . .
	//	. . . . 1 1 . . . .		. . . . . . . . . .		1 . 1 .[1]. . . . .

	//	lookup rowSight			mask w/ diagTable		this is merged w/ other
	//	. . . 1 . 1 1 1 1 1		. . . . . . . . . .		lines for moveBoard, before
	//	. . . 1 . 1 1 1 1 1		. . . . . . . . . .		threatened squares are
	//	. . . 1 . 1 1 1 1 1		. . . . . . . . . .		subtracted
	//	. . . 1 . 1 1 1 1 1		. . . . . . . . . .
	//	. . . 1 . 1 1 1 1 1	 >	. . . 1 . . . . . .
	//	. . . 1 . 1 1 1 1 1	 >	. . . . . . . . . .
	//	. . . 1 . 1 1 1 1 1		. . . . . 1 . . . .
	//	. . . 1 . 1 1 1 1 1		. . . . . . 1 . . .
	//	. . . 1 . 1 1 1 1 1		. . . . . . . 1 . .
	//	. . . 1 . 1 1 1 1 1		. . . . . . . . 1 .

void Board::update_piece_sight(Piece* p) {
	// get position of piece as int 0 -> NUM_SQUARES - 1
	int k = p->scalar;
	p->sightBoard.reset();

	// mouse or elephant
	if (p->movesLikeRook) {
		// get occupancy masked to relevant row and file
		bboard row = occupancy & Tables::rowTable[k];
		bboard file = occupancy & Tables::fileTable[k];

		// transform both to first row
		Bitboard::collapse_to_row(&row);
		Bitboard::collapse_to_file(&file);
		Bitboard::file_to_row(&file);

		// use data from row/file occupancies as index of sight table
		// second index is position of piece within row/file
		row = Tables::rowSight[row.to_ulong()][k % BOARD_SIZE];
		file = Tables::fileSight[file.to_ulong()][k / BOARD_SIZE];

		// mask back to relevant row/file
		row &= Tables::rowTable[k];
		file &= Tables::fileTable[k];

		// add to move set
		p->sightBoard |= row;
		p->sightBoard |= file;
	}

	// lion or elephant (bishops)
	if (p->movesLikeBishop) {
		// get occupancy masked to relevant diag
		bboard diag = occupancy & Tables::diagTable[k];
		bboard antidiag = occupancy & Tables::antidiagTable[k];

		// transform both to first row
		Bitboard::collapse_to_row(&diag);
		Bitboard::collapse_to_row(&antidiag);

		// use data from diag occupancies as index of sight table
		// second index is position of piece within diag
		diag = Tables::rowSight[*reinterpret_cast<unsigned int*>(&diag)][k % BOARD_SIZE];
		antidiag = Tables::rowSight[*reinterpret_cast<unsigned int*>(&antidiag)][k % BOARD_SIZE];

		// mask back to relevant diag
		diag &= Tables::diagTable[k];
		antidiag &= Tables::antidiagTable[k];

		// add to move set
		p->sightBoard |= diag;
		p->sightBoard |= antidiag;
	}
}


void Board::update_piece_moves(Piece* p) {
	if (p->schedSightUpdate) {
		update_piece_sight(p);
		p->schedSightUpdate = false;
	}

	p->moveBoard = p->sightBoard;

	// manage threats
	if (p->isThreatened) {
		// if threatened and able to move, a move is forced
		if ((p->moveBoard & ~*(p->scaredByMap)).any()) {
			p->moveBoard &= ~*(p->scaredByMap);
			p->isForced = true;
		} else {
			// if unable to move from threat, a move is not forced
			p->isForced = false;
		}
	} else {
		// remove threatening squares
		p->moveBoard &= ~*(p->scaredByMap);
		p->isForced = false;
	}
}

////

// remove from occupancy maps
void Board::remove_from_occupancy(Piece* p) {
	occupancy ^= p->pos;
	occupancyBySide[p->side] ^= p->pos;
}

// add to occupancy maps
void Board::add_to_occupancy(Piece* p) {
	occupancy |= p->pos;
	occupancyBySide[p->side] |= p->pos;
}

// update threat map and potentially threatened pieces
void Board::update_threats(Piece* p) {
	// get adjacent squares
	p->adjacent = Tables::adjacencyTable[p->scalar];

	// update threat map of herd
	*p->herdMap = p->adjacent;
	for (Piece* q : p->siblings)
		*p->herdMap |= q->adjacent;


	p->update_threatened();

	// check whether pieces scared by p are threatened
	for (Piece* q : p->scares)
		q->update_threatened();
}

////

// initialize all values/tables used for zobrist key
void Board::init_zobrist_values() {

	CHANGE_SIDE = Bitboard::random();
	key = Key(0);
	hash = 0;

	// init zobrist 
	for (Side side : SIDES)
		for (Piece* p : pieces[side])
			key ^= Tables::zobristTable[p->herd][p->scalar];

	// set up hash
	KEY_MASK = Key(TT_ALLOC - 1);
	hash = key_to_hash(key);
}

// update zobrist key
// must be called before piece has moved
void Board::update_zobrist_key(Piece* p, u_short dest) {
	//__DEBUG(p == nullptr, "ERROR - pointerBoard[scalar] was nullptr");

	// remove from key
	key ^= Tables::zobristTable[p->herd][p->scalar];

	// add to key
	key ^= Tables::zobristTable[p->herd][dest];

	// next side to play
	key ^= CHANGE_SIDE;

	// masked key reduction for tt index
	hash = key_to_hash(key);
}

u_long Board::key_to_hash(Key key_) {
	return (KEY_MASK & key_).to_ulong();
}

////

// print info about current state, used in debug
void Board::__DEBUG(bool condition, string message) {

	if constexpr (DEBUG) {
		if (!condition)
			return;

		__PRINT("DEBUG:\n");

		print();

		__PRINT("PIECES:\n");

		for (Side side : SIDES)
			for (Piece* p : pieces[side])
				__PRINT(p->status() + "\n");

		__PRINT("\n");
	}

	if (condition)
		throw Exception(message);
}

} // end namespace Bbot2