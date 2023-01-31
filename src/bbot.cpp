// bbot.cpp

#include "bbot.h"

using std::string;
using std::vector;
using std::format;
using std::abs;

namespace Bbot2 {

// Bbot //

// constructor
Bbot::Bbot(Game* game_) 
	: game(game_) {

	board = game->board;
	pieces = board->pieces;
	pointerBoard = board->pointerBoard;
}

////

// initialize board if needed, then generate other values, set up first key/hash
void Bbot::init() {
	if (!board->initialized)
		board->init();

	// allocate TT
	transpositionTable = new TT[TT_ALLOC];

	// initialize eval boards
	init_eval_boards();

	// remember starting position
	gh_store();

	initialized = true;
}

// if board is to be closed and Bbot will persist, call this before board->close()
void Bbot::close_game() {
	gh_clear_played();
}

// reset when passed a new board
void Bbot::open_game(Game* game_) {
	game = game_;
	board = game->board;
	pieces = board->pieces;
	pointerBoard = board->pointerBoard;

	PV.length = 0;
	eval = 0;

	gh_store();
}

////

// search with iterative deepening
// bounded by maximum time (ms) and/or maximum depth
// returns true while searching, false when done
bool Bbot::search(int maxTime, int maxDepth) {

	// if not searching, reset values and start search
	if (!searching) {
		startClock = std::clock();
		allottedTime = CLOCKS_PER_SEC * maxTime / 1000; // ms to clocks

		searchDepth = 0;
		eval = 0;

		// log values
		nodesVisited = 0;
		ttHits = 0;
		ttWrites = 0;
		ttUpdates = 0;
		ttOverwrites = 0;

		searching = true;

		__LOG(format("[{}]: SEARCHING...", SIDE_NAMES[board->sideToMove]));
	}

	// check for search exit
	if (search_exit(searchDepth >= std::min(maxDepth, MAX_LINE_LEN), "MAX DEPTH REACHED"))
		return false;

	if (search_exit(is_mate_eval(eval), "FORCED WIN/LOSS FOUND"))
		return false;

	// search to depth of next iterative deepening
	search_fixed_depth(searchDepth + 1);

	// update moves after search tree traversal
	board->update_move_sets();

	// check time
	if (search_exit(search_clock() > allottedTime && searchDepth > 0, "TIME EXCEEDED"))
		return false;

	// log
	__LOG(format("   DEPTH {} (EVAL {}): {} ({} ms)", searchDepth, search_eval(), search_PV(), searchDuration * 1000));

	return true;
}

// abort search on next loop
// will log as "TIME EXCEEDED"
void Bbot::search_abort() {
	allottedTime = 0;
}

////

// needs to be called after board->play_move
void Bbot::on_move_played(Move move) {

	// store move in game history
	gh_store();

	// if move is in PV, shift PV forward by one
	// otherwise, clear
	if (PV.length == 0)
		return;

	if (move == PV.moves[0]) {
		PV.delete_first();
	} else {
		PV.length = 0;
	}
}

////

// get eval string
string Bbot::search_eval() {

	// white to mate
	if (eval >= EVAL_WIN - MAX_LINE_LEN) {
		return format("+M{}", (EVAL_WIN - eval + 1) / NUM_SIDES);
	}

	// black to mate
	if (eval <= -EVAL_WIN + MAX_LINE_LEN)
		return format("-M{}", (EVAL_WIN + eval + 1) / NUM_SIDES);

	// non-mate eval
	// 1.0 = +1 watering hole advantage

	return format("{:+.3f}", (float) eval / ON_WH_SCORE[0]);
}

// get PV string
string Bbot::search_PV() {
	return board->line_to_string(PV);
}

// get if search is ongoing
bool Bbot::search_ongoing() {
	return searching;
}

// get max depth reached by most recent successful search
int Bbot::search_depth() {
	return searchDepth;
}

// get time taken by most recent successful search
double Bbot::search_duration() {
	return searchDuration;
}

// get move suggested by engine
// only call after successful search
Move Bbot::suggested_move() {
	return PV.moves[0];
}

////

// close
// clear game history and de-allocate transposition table
void Bbot::close() {
	if (!initialized)
		return;

	// clear remaining game history
	gh_clear_played();

	// de-allocate TT
	delete[] transpositionTable;
}

////////////////////////////////

// store transposition table entry if greater priority
// depth: depth searched
// flag: is value from exact, static, alpha, or beta search cutoff
// value: position eval
// move: best move - relevant if flag is FLAG_EXACT or FLAG_BETA
void Bbot::tt_store(u_short depth, Flag_TT flag, int value, Move move) {

	// previous entry in transposition table
	TT* entry = tt_current();

	// do not overwrite if previous entry is exact and new entry is not
	if (entry->flag == FLAG_EXACT && flag != FLAG_EXACT)
		return;

	// do not overwrite if previous entry has a higher priority (determined by depth and recency)
	if (entry->flag != FLAG_EMPTY && entry->depth + entry->foundAt > depth + game->ply)
		return;

	// log values
	if constexpr (LOG && LOG_VERBOSE) {
		ttWrites ++;
		if (entry->flag == FLAG_EMPTY) {
			ttEntries ++;
		} else if (entry->key == board->key) {
			ttUpdates ++;
		} else {
			ttOverwrites ++;
		}
	}

	// overwrite otherwise
	entry->key = board->key;
	entry->depth = depth;
	entry->foundAt = game->ply;
	entry->flag = flag;
	entry->value = value;
	entry->move = move;

	// account for mating distance
	if (entry->value > EVAL_WIN - MAX_LINE_LEN)
		entry->value += rootDist;

	if (entry->value < -EVAL_WIN + MAX_LINE_LEN)
		entry->value -= rootDist;
}

// if a matching hash key is found in the transposition table
// and if TT entry has a satisfactory depth
// use TT information as opposed to a re-search
int Bbot::tt_lookup(u_short depth, int alpha, int beta) {

	// current entry
	TT* entry = tt_current();

	// if key doesn't match, return no-value flag
	if (entry->key != board->key)
		return VALUE_UNKNOWN;

	if constexpr (LOG && LOG_VERBOSE)
		ttHits ++;

	// if satisfactory depth, use value
	if (entry->depth >= depth) {

		// if flag is exact, use value verbatim
		if (entry->flag == FLAG_EXACT) {
			// account for mating distance
			if (entry->value > EVAL_WIN - MAX_LINE_LEN)
				return entry->value - rootDist;

			if (entry->value < -EVAL_WIN + MAX_LINE_LEN)
				return entry->value + rootDist;

			return entry->value;
		}

		if (entry->flag == FLAG_STATIC)
			return entry->value;

		// if flag is alpha and search tree has seen better, give a fail-low result
		if (entry->flag == FLAG_ALPHA && entry->value <= alpha)
			return alpha;

		// if flag is beta and search tree has seen better for opponent, give a fail-high result
		if (entry->flag == FLAG_BETA && entry->value >= beta)
			return beta;
	}
		
	// if not satisfactory depth or missing the cutoff, use previously-found best move to start search
	// this improves move-ordering
	if (entry->flag == FLAG_EXACT || entry->flag == FLAG_BETA) {
		add_move(entry->move);
	}

	return VALUE_UNKNOWN;
}

// get current transposition table entry
TT* Bbot::tt_current() {
	return &transpositionTable[board->hash];
}

// print TT entry
void Bbot::tt_print(TT* entry) {

	string s = "KEY: " + std::to_string(board->hash);
	s += "\nCOMPLETE: " + Bitboard::to_hex(board->key) + "\n";
	
	s += to_string() + "\n"; // game pos


	if (entry->key == board->key) {
		s += "DEPTH: " + std::to_string(entry->depth);
		s += "\nFOUND AT GAME PLY: " + std::to_string(entry->foundAt);
		s += "\nFLAG: ";

		switch (entry->flag) {
			case FLAG_EXACT: s += "EXACT"; break;
			case FLAG_STATIC: s += "STATIC"; break;
			case FLAG_ALPHA: s += "ALPHA"; break;
			case FLAG_BETA: s += "BETA"; break;
		}

		s += "\nVALUE: " + std::to_string(entry->value);
		s += "\nMOVE: " + entry->move.to_string(pointerBoard);
	} else {
		s += "MISS";
	}

	__PRINT(s + "\n");
}

////

// game history - used to quickly detect draws by repetition

// store current key in linked list
// head of list is stored in key's TT hash entry
void Bbot::gh_store() {
	GH* gh = transpositionTable[board->hash].history;
	
	// if list is empty, create new head
	if (gh == nullptr) {
		transpositionTable[board->hash].history = new GH(board->key);
		return;
	}

	// otherwise, find end of list
	while (gh->next != nullptr)
		gh = gh->next;

	// allocate new entry at end of list
	gh->next = new GH(board->key);
}

// remove current key from memory
void Bbot::gh_remove() {
	GH* gh = transpositionTable[board->hash].history;

	if (gh == nullptr)
		return;

	// delete if key matches head
	if (gh->key == board->key) {
		// connect TT entry's GH head to next node
		transpositionTable[board->hash].history = gh->next;
		delete gh;
		return;
	}

	// scan list
	while (gh != nullptr) {

		// when match is found
		if (gh->next->key == board->key) {
			// connect previous node to next node
			GH* old = gh->next;
			gh->next = gh->next->next;

			// delete node
			delete old;
			return;
		}

		gh = gh->next;
	}
}

// returns true if position has been previously seen in game history or current line
bool Bbot::gh_match() {
	// return false if in initial search position
	if (rootDist == 0)
		return false;

	GH* gh = transpositionTable[board->hash].history;

	// scan list
	while (gh != nullptr) {
		// true if match
		if (gh->key == board->key)
			return true;

		gh = gh->next;
	}

	return false;
}

// GH items in search tree are freed within the tree
// those added by played moves need to be de-allocated on close
void Bbot::gh_clear_played() {

	GH* gh = game->history;
	Key temp = board->key;

	while (gh != nullptr) {
		board->key = gh->key;
		board->hash = board->key_to_hash(gh->key);
		gh_remove();
		gh = gh->next;
	}

	board->key = temp;
	board->hash = board->key_to_hash(temp);
}

////

// for all pieces, expand moveBoard bboards and add to the move list in staged order
// stages:
// 1. moves to watering holes
// 2. moves threatening opponent
// 3. moves to watering hole row/col/diag
// 4. all remaining moves
void Bbot::add_legal_moves() {
	bboard stagedmoveBoard;
	Move move;
	u_long scalar;
	
	// quickly update legal move sets
	// doesn't properly clear forced pieces or opposite-colour pieces, but this is caught inside the loop
	board->quick_move_sets();

	for (int stage = 0; stage < 4; stage ++) {
		for (Piece* p : pieces[board->sideToMove]) {

			// if any piece is forced, only include forced pieces
			if (board->isSideForced && !p->isForced)
				continue;

			// move set to be filtered
			stagedmoveBoard = p->moveBoard;

			// set 'from' in recorded move to current piece scalar
			move.set_from(p->scalar);

			// filter staged move set with stage's bboard
			// then remove from original move set
			if (stage == 0) {
				// stage 1 - watering holes
				stagedmoveBoard &= board->wateringHoles;
				p->moveBoard ^= stagedmoveBoard;

			} else if (stage == 1) {
				// stage 2 - threats
				stagedmoveBoard &= *(p->scaresMap);
				p->moveBoard ^= stagedmoveBoard;

			} else if (stage == 2) {
				// stage 3 - wh row/col/diags
				if (p->type == MOUSE) {
					stagedmoveBoard &= whRowCol; // mice have row/col prioritized
				} else if (p->type == LION) {
					stagedmoveBoard &= whDiag[2]; // lions have diags prioritized
				} else {
					stagedmoveBoard &= whAllLines; // elephants have both prioritized
				}

				p->moveBoard ^= stagedmoveBoard;
			}

			// stage 4 - all remaining moves (no filter)

			// serialize move set
			// scanning in direction most likely to return a quick success
			if (board->sideToMove == WHITE) {
				// if white, scan forward
				while (Bitboard::scan_forward(&scalar, &stagedmoveBoard)) {
					stagedmoveBoard ^= Bitboard::SQUARES[scalar]; // remove found bit
					move.set_to(scalar); // set destination
					add_move(move); // add move
				}

			} else {
				// if black, scan reverse
				while (Bitboard::scan_reverse(&scalar, &stagedmoveBoard)) {
					stagedmoveBoard ^= Bitboard::SQUARES[scalar]; // remove found bit
					move.set_to(scalar); // set destination
					add_move(move); // add move
				}
			}
		}
	}
}

// add move to pile
void Bbot::add_move(Move move) {
	moveList[toGenerate ++] = move;
}

// move piece on board
void Bbot::make_move(Move move) {
	int from = move.get_from();
	int to = move.get_to();
	Piece* p = pointerBoard[from];

	//__DEBUG(p == nullptr, "Move " + move.to_string(pointerBoard) + " had no piece at origin.");

	// update board
	board->move_piece(p, to);

	rootDist ++;
}

// make_move with 'from' and 'to' reversed
void Bbot::unmake_move(Move move) {
	int from = move.get_from();
	int to = move.get_to();
	Piece* p = pointerBoard[to];

	// update board
	board->move_piece(p, from);

	rootDist --;
}

// traverse line forwards
void Bbot::traverse_forwards(Line* line, int dist) {
	for (int i = 0; i < dist; i ++)
		make_move(line->moves[i]);
}

// traverse line backwards
void Bbot::traverse_backwards(Line* line, int dist) {
	for (int i = dist - 1; i >= 0; i --)
		unmake_move(line->moves[i]);
}

////

// SEARCH
// start with a PV traversal to get an aspiration window estimate
// with this window, begin recursive alphabeta search, depth-first to the given depth
// depth may be exceeded if a good TT node is hit
// this is called repeatedly with increasing depth for iterative deepening
void Bbot::search_fixed_depth(int depth) {

	// set values
	toGenerate = 0;
	toPlay = 0;
	rootDist = 0;
	int value = 0;

	// alpha/beta bounds
	int alpha = -INT_MAX;
	int beta = INT_MAX;

	 // if PV exists from previous loop, traverse
	 // find best move after a depth-1 search from end of PV
	 // this provides an estimated eval useful for setting the aspiration window
	if (PV.length >= depth - 1 && depth > 1) {

		// traverse PV
		traverse_forwards(&PV, depth - 1);

		// depth-1 search
		Line branch;
		value = search_alphabeta(1, alpha, beta, &branch);

		if (!(depth % 2))
			value = -value;

		alpha = value - ASPIRATION_WINDOW;
		beta = value + ASPIRATION_WINDOW;

		// traverse back to root
		traverse_backwards(&PV, depth - 1);

		// after unmade moves, full move set update is required
		board->update_move_sets();
	}

	
	// first search, with aspiration window
	Line PVtemp;
	value = search_alphabeta(depth, alpha, beta, &PVtemp);

	// exit if search aborted
	if (abs(value) == SEARCH_ABORTED)
		return;

	// if value falls outside aspiration window, a re-search is required
	// we know that the search failed in one direction or another, so we can search just below or just above our previous bounds
	// i.e., if we fail low on [alpha, beta], search [-infinity, alpha]
	// if we fail high, search [beta, infinity]

	// search fails low
	if (value <= alpha) {
		__LOG_VERBOSE(format("      FAIL LOW on [{}, {}]", alpha, beta));

		beta = alpha;
		alpha = -INT_MAX;
		PVtemp.length = 0;

		board->update_move_sets();
		value = search_alphabeta(depth, alpha, beta, &PVtemp);

	} else if (value >= beta) {
		
		// search fails high
		__LOG_VERBOSE(format("      FAIL HIGH on [{}, {}]", alpha, beta));

		alpha = beta;
		beta = INT_MAX;
		PVtemp.length = 0;

		board->update_move_sets();
		value = search_alphabeta(depth, alpha, beta, &PVtemp);
	}

	// exit if search aborted
	if (abs(value) == SEARCH_ABORTED)
		return;

	// rarely, a search may fail a second time
	if (value <= alpha || value >= beta) {
		__LOG_VERBOSE(value <= alpha, format("      2ND FAIL - LOW on [{}, {}]", alpha, beta));
		__LOG_VERBOSE(value >= beta, format("      2ND FAIL - HIGH on [{}, {}]", alpha, beta));

		board->update_move_sets();
		PVtemp.length = 0;
		value = search_alphabeta(depth, -INT_MAX, INT_MAX, &PVtemp);
	}

	// exit if search aborted
	if (abs(value) == SEARCH_ABORTED)
		return;

	__DEBUG(PVtemp.length == 0 && PV.length == 0, "ERROR - PV and PVtemp were length 0");

	// copy PV if newer PV found
	if (PVtemp.length > 0)
		PV = PVtemp;

	// extend PV using TT
	// - traverse PV and check resulting TT entry
	// - if entry is exact and a high enough depth, it is satisfactory as a PV node
	// - add to PV and repeat
	// if successful, this can extend PVs past expected depth
	traverse_forwards(&PV, PV.length);
	TT* entry = tt_current();

	// check if TT entry is useful
	while (entry->key == board->key && entry->flag == FLAG_EXACT && entry->depth >= depth - PV.length && PV.length < MAX_LINE_LEN) {
		//__DEBUG(pointerBoard[entry->move.get_from()] == nullptr, "Move " + entry->move.to_string(pointerBoard) + " had no piece at origin.");

		// add to PV
		PV.append(entry->move);
		make_move(entry->move);
		entry = tt_current();
	}

	// traverse back to root
	traverse_backwards(&PV, PV.length);

	// set eval
	if (abs(value) != SEARCH_ABORTED)
		eval = board->sideToMove ? -value : value;

	// set depth
	searchDepth = std::max(depth, (int) tt_current()->depth);

	// set duration
	searchDuration = (double) search_clock() / CLOCKS_PER_SEC;
}

// SEARCH TREE
// negamax alphabeta search with transposition tables
// depth: distance to terminal node
// alpha: fail-low cutoff. value of best line considered so far. this value must be exceeded for a new move to be considered
// beta: fail-high cutoff. if this value is exceeded, the opponent is unlikely to choose this branch, so this branch is discarded
// line: pointer to current branch, used for recording PV
int Bbot::search_alphabeta(int depth, int alpha, int beta, Line* line) {

	// if allotted time is exceeded, return unknown value flag
	if (search_clock() > allottedTime && searchDepth > 0)
		return SEARCH_ABORTED;

	if constexpr (LOG)
		nodesVisited ++;

	// set values
	int value;
	Flag_TT flag = FLAG_ALPHA; // if no flag is set, then all moves must have failed low and an ALPHA flag is stored in TT
	Line branch;
	Move move;

	// check if game lost
	if (game->game_lost())
		return -EVAL_WIN + rootDist;

	// check draw by repetition
	if (gh_match())
		return rootDist % 2 ? -EVAL_DRAW : EVAL_DRAW;

	// check tt
	// if value is returned, use instead of current search
	if ((value = tt_lookup(depth, alpha, beta)) != VALUE_UNKNOWN) {

		// if move exists, add to line
		if (tt_current()->flag == FLAG_EXACT) {
			line->moves[0] = tt_current()->move;
			line->length = 1;
		}

		return value;
	}

	// if depth 0, do static eval
	if (depth == 0) {
		value = evaluate();

		tt_store(depth, FLAG_STATIC, value, 0);

		return value;
	}

	// store position in TT game history
	gh_store();

	// add all legal moves to moveList
	add_legal_moves();

	int temp1, temp2;

	// loop until no more moves to play
	while (toPlay < toGenerate) {

		// clear branch
		branch.length = 0;

		// remember current moveList indices
		temp1 = toPlay;
		temp2 = toGenerate;

		// make move
		make_move(moveList[toPlay]);

		// search children
		toPlay = toGenerate;

		// recursive alphabeta search
		value = -search_alphabeta(depth - 1, -beta, -alpha, &branch);

		// undo move
		toPlay = temp1;
		toGenerate = temp2;
		unmake_move(moveList[toPlay]);

		// if search aborted (time exceeded) then exit
		if (value == -SEARCH_ABORTED) {
			gh_remove();
			return SEARCH_ABORTED;
		}

		// fails high
		if (value >= beta) {
			// store move in TT
			tt_store(depth, FLAG_BETA, beta, moveList[toPlay]);

			// remove position from game history
			gh_remove();

			// fail high cutoff
			return beta;
		}

		// succeeds high
		// if value lies between alpha and beta, set new alpha and record move
		if (value > alpha) {
			alpha = value;

			// last/most successful move will be stored with an EXACT flag in TT
			flag = FLAG_EXACT;
			move = moveList[toPlay];

			// copy successful branch to PV line
			line->moves[0] = move;
			memcpy(line->moves + 1, branch.moves, branch.length * sizeof(Move));
			line->length = branch.length + 1;
		}

		// iterate to next move
		toPlay ++;
	}

	// if flag is alpha, failed low
	// if flag is exact, succeeded high
	tt_store(depth, flag, alpha, move);

	// remove position from game history
	gh_remove();

	// if failed low, alpha is returned
	// if succeeded high, alpha stores the best value
	return alpha;
}

// time since search initiated
std::clock_t Bbot::search_clock() {
	return std::clock() - startClock;
}

// check condition and end search if true
bool Bbot::search_exit(bool condition, string message) {
	if (!condition)
		return false;

	__LOG("   SEARCH EXITED - " + message);
	__LOG_VERBOSE(format("      {} nodes visited", nodesVisited));
	__LOG_VERBOSE(format("      {} hits, {} writes, {} updates, {} overwrites", ttHits, ttWrites, ttUpdates, ttOverwrites));
	__LOG_VERBOSE(format("      TABLE: [{} / {}] ({:.2f}% Full)", ttEntries, TT_ALLOC, (float) ttEntries / TT_ALLOC * 100));

	//__PRINT_CHART();

	searching = false;

	return true;
}

////

// set up various bboards used for scoring and move ordering
void Bbot::init_eval_boards() {
	for (int i = 0; i < NUM_TILE_GROUPS; i ++)
		tileGroups[i] = Bitboard::from_string(TILE_GROUP_STR[i]);

	whRowCol = Bitboard::from_string(WH_ROW_COL_STR);
	whDiag[0] = Bitboard::from_string(WH_DIAG_STR[0]);
	whDiag[1] = Bitboard::from_string(WH_DIAG_STR[1]);
	whDiag[2] = whDiag[0] | whDiag[1];
	whAllLines = whRowCol | whDiag[2];

	// create valueTable from scoring parameters
	for (int i = 0; i < NUM_SQUARES; i ++) {
		for (int j = 0; j < NUM_TILE_GROUPS; j ++)
			if (tileGroups[j][i])
				for (PieceType t : PIECE_TYPES)
					valueTable[t][i] = TILE_GROUP_SCORE[t];

		if (whRowCol[i]) {
			valueTable[MOUSE][i] += ON_WH_ROW_COL_SCORE;
			valueTable[ELEPHANT][i] += ON_WH_ROW_COL_SCORE;
		}

		if (whDiag[1][i]) {
			valueTable[LION][i] += ON_WH_DIAG_SCORE[1];
			valueTable[ELEPHANT][i] += ON_WH_DIAG_SCORE[1];
		}
		
		if (whDiag[0][i]) {
			valueTable[LION][i] += ON_WH_DIAG_SCORE[0];
			valueTable[ELEPHANT][i] += ON_WH_DIAG_SCORE[0];
		}

		if (board->wateringHoles[i])
			for (PieceType t : PIECE_TYPES)
				valueTable[t][i] = ON_WH_SCORE[0];
	}
}

// STATIC EVALUATION
// evaluates position at terminal node
// - considers valueTables and piece threats
// - positive if favouring side to move
// - equal and opposite scoring is calculated for opponent, so score may be negative or 0 (if equal)

int Bbot::evaluate() {

	// white pieces
	int value = 0;
	for (Piece* p : pieces[WHITE]) {
		value += valueTable[p->type][p->scalar];
		if (p->isThreatened)
			value -= THREAT_SCORE;
	}

	// black pieces
	for (Piece* p : pieces[BLACK]) {
		value -= valueTable[p->type][p->scalar];
		if (p->isThreatened)
			value += THREAT_SCORE;
	}

	value = (board->sideToMove ? -value : value);

	// side to move typically has an advantage, so adding a small value minimizes score oscillation
	if (!is_mate_eval(value))
		value += TO_MOVE_SCORE;

	return value;
}

// true if value represents a forced mate for either colour
bool Bbot::is_mate_eval(int value) {
	return abs(value) > EVAL_WIN - MAX_LINE_LEN;
}
	
////

// current position to string
string Bbot::to_string() {
	return board->to_string();
}

// print
void Bbot::print() {
	board->print();
}

////

// add time since last clock to time distribution array at i, used for optimization
void Bbot::__CHART_TIME(int index) {
	timeDistribution[index] += std::clock() - lastClock;
	lastClock = std::clock();
}

// print time distribution
void Bbot::__PRINT_CHART() {
	__LOG("TIME CHART:");

	for (int i = 0; i < timeDistCats.size(); i ++)
		__LOG(format("   {}: {:1f} ms", timeDistCats[i], (double) timeDistribution[i] / CLOCKS_PER_SEC * 1000));
}

// print info about current state, used in debug
void Bbot::__DEBUG(bool condition, string message) {
	if constexpr (DEBUG)
		if (condition)
			__PRINT("PV: " + board->line_to_string(PV) + "\n");

	if (board)
		board->__DEBUG(condition, message);
}

} // end namespace Bbot2