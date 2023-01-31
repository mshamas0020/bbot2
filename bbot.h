// bbot.h

// a chess variant engine built to play Barca, a board game by Andrew Caldwell
// this engine uses bitboard-lookup move generation, a negamax alphabeta search tree, and iterative deepening
// within the tree, it utilizes a transposition table with no hash collision resolution
// there is collision resolution exclusively for a game history linked list within the TT to detect draws
// many chess programming techniques are adapted to fit to the alternate rule-set and 10x10 board

// TODO:
// Automated parameter tuning
// Multi-threading (Lazy SMP)
// Opening book

#pragma once

#include "common.h"
#include "log.h"
#include "board.h"
#include "game.h"
#include "piece.h"
#include "move.h"
#include "line.h"
#include "bitboard.h"
#include <ctime>


namespace Bbot2 {

class Game;

struct GH;

enum Flag_TT: u_byte { FLAG_EMPTY, FLAG_EXACT, FLAG_STATIC, FLAG_ALPHA, FLAG_BETA };

// transposition table entry
// hash table using zobrist key as key and modulus for hash function
// no hash collision
typedef struct TT {
	Key key;
	u_short depth = 0;
	Flag_TT flag = FLAG_EMPTY; // EMPTY, EXACT, STATIC, ALPHA, or BETA
	int value; // evaluation
	Move move; // recommended move
	u_short foundAt = 0; // start ply of search at which entry was stored. used to factor recency 
	GH* history = nullptr; // gh head
} TT;

// Bbot
class Bbot {
	////

	//// SCORING PARAMETERS ////

	int EVAL_WIN = 1000000; // win
	int EVAL_DRAW = -9999; // draw

	static const int NUM_TILE_GROUPS = 4;
	int TILE_GROUP_SCORE[NUM_TILE_GROUPS] = { 500, 1000, 4000, 5000 }; // score for tile groups, defined below

	int ON_WH_ROW_COL_SCORE = 2500; // for watering hole row or column
	int ON_WH_DIAG_SCORE[2] = { 1000, 2500 }; // for watering hole diagonals, [0] sees 1 wh, [1] sees 2 wh
	int ON_WH_SCORE[NUM_WH] = { 20000, 50000, EVAL_WIN, EVAL_WIN }; // [0] if side is on 1 wh, [1] if on 2

	int THREAT_SCORE = 5000; // subtracted for every piece side has threatened

	int TO_MOVE_SCORE = 1000; // to minimize score oscillation, this advantage is given for the player to move

	//// SEARCH PARAMETERS ////

	int ASPIRATION_WINDOW = 5000; // the width of bounds for the first search
	// a narrow window is initially faster, but more likely to fail, requiring a re-search

	//// MEMORY ////

	// size of FILO array of moves to play within search
	// entries are quickly rewritten and stay low, only reach higher indices at higher depths
	// does not impact performance, but will cause errors if set too low
	static const int MOVE_LIST_ALLOC = 2048;

	////



	// scorings are added/subtracted for every time the occupancy bboard intersects with any of the groups below
	// strings are converted to bboards on init
	std::string TILE_GROUP_STR[NUM_TILE_GROUPS] = {
		"1 1 1 1 1 1 1 1 1 1"
		"1 . . . . . . . . 1"
		"1 . . . . . . . . 1"
		"1 . . . . . . . . 1"
		"1 . . . . . . . . 1"
		"1 . . . . . . . . 1"
		"1 . . . . . . . . 1"
		"1 . . . . . . . . 1"
		"1 . . . . . . . . 1"
		"1 1 1 1 1 1 1 1 1 1",

		". . . . . . . . . ."
		". 1 1 1 1 1 1 1 1 ."
		". 1 . . . . . . 1 ."
		". 1 . . . . . . 1 ."
		". 1 . . . . . . 1 ."
		". 1 . . . . . . 1 ."
		". 1 . . . . . . 1 ."
		". 1 . . . . . . 1 ."
		". 1 1 1 1 1 1 1 1 ."
		". . . . . . . . . .",

		". . . . . . . . . ."
		". . . . . . . . . ."
		". . 1 1 1 1 1 1 . ."
		". . 1 . . . . 1 . ."
		". . 1 . . . . 1 . ."
		". . 1 . . . . 1 . ."
		". . 1 . . . . 1 . ."
		". . 1 1 1 1 1 1 . ."
		". . . . . . . . . ."
		". . . . . . . . . .",

		". . . . . . . . . ."
		". . . . . . . . . ."
		". . . . . . . . . ."
		". . . . 1 1 . . . ."
		". . . 1 1 1 1 . . ."
		". . . 1 1 1 1 . . ."
		". . . . 1 1 . . . ."
		". . . . . . . . . ."
		". . . . . . . . . ."
		". . . . . . . . . ."
	};

	std::string WH_ROW_COL_STR =
		". . . 1 . . 1 . . ."
		". . . 1 . . 1 . . ."
		". . . 1 . . 1 . . ."
		"1 1 1 . 1 1 . 1 1 1"
		". . . 1 . . 1 . . ."
		". . . 1 . . 1 . . ."
		"1 1 1 . 1 1 . 1 1 1"
		". . . 1 . . 1 . . ."
		". . . 1 . . 1 . . ."
		". . . 1 . . 1 . . .";

	std::string WH_DIAG_STR[2] = {
		". . . 1 . . 1 . . ."
		". . . . 1 1 . . . ."
		". . . . 1 1 . . . ."
		"1 . . . . . . . . 1"
		". 1 1 . . . . 1 1 ."
		". 1 1 . . . . 1 1 ."
		"1 . . . . . . . . 1"
		". . . . 1 1 . . . ."
		". . . . 1 1 . . . ."
		". . . 1 . . 1 . . .",

		"1 . . . . . . . . 1"
		". 1 . . . . . . 1 ."
		". . 1 . . . . 1 . ."
		". . . . . . . . . ."
		". . . . 1 1 . . . ."
		". . . . 1 1 . . . ."
		". . . . . . . . . ."
		". . 1 . . . . 1 . ."
		". 1 . . . . . . 1 ."
		"1 . . . . . . . . 1"
	};

	// bboards for strings above
	// some of these are also used for move prioritization
	bboard tileGroups[NUM_TILE_GROUPS]; // concentric rings. proximity to center means a higher score
	bboard whRowCol; // all squares that see a wh on a row/col
	bboard whDiag[3]; // on a diagonal, [0] sees one wh, [1] sees two wh, and [2] = [0] | [1]
	bboard whAllLines; // all squares that see a wh on their row/col/diag
	int valueTable[NUM_TYPES][NUM_SQUARES]; // piece/square value table used for evaluation

	Game* game; // attached game
	Board* board; // attached board, used to read position and manipulate pieces during search
	std::vector<Piece*>* pieces; // from board - consecutive piece array
	Piece** pointerBoard; // from board - array of pointers where a piece is indexed by its scalar, nullptr if square is empty

	TT* transpositionTable; // hash table, size of TT_ALLOC

	Move moveList[MOVE_LIST_ALLOC]; // FILO array used to store all moves to be played
	int toGenerate = 0; // index of next move to be generated
	int toPlay = 0; // index of next move to be played
	int rootDist = 0; // distance from root. increments up within search tree while depth decrements

	const int VALUE_UNKNOWN = INT_MAX - 1; // flag for no TT lookup value
	const int SEARCH_ABORTED = INT_MAX - 2; // flag for aborted search in alphabeta

	Line PV; // principal variation
	int eval; // final evaluation

	int searching = false; // is search ongoing
	int searchDepth = 0; // max depth successfully reached in current search
	double searchDuration; // total time taken by most recent search

	std::clock_t startClock; // set when search is started
	std::clock_t allottedTime; // time limit for search

	// __CHART_TIME
	std::clock_t timeDistribution[5] = { 0, 0, 0, 0, 0 };
	std::clock_t lastClock;
	std::vector<std::string> timeDistCats{ "MOVE GEN", "STATIC EVAL", "TT", "GH", "MOVES" };



	// log values
	int nodesVisited = 0;
	int ttEntries = 0;
	int ttWrites = 0;
	int ttHits = 0;
	int ttUpdates = 0;
	int ttOverwrites = 0;

public:
	bool initialized = false;

	Bbot(Game* game_);

	void init();
	void close_game();
	void open_game(Game* game_);

	bool search(int maxTime, int maxDepth);
	void search_abort();

	void on_move_played(Move move);

	std::string search_eval();
	std::string search_PV();
	bool search_ongoing();
	int search_depth();
	double search_duration();
	Move suggested_move();

	void close();

private:

	void tt_store(u_short depth, Flag_TT flag, int value, Move move);
	int tt_lookup(u_short depth, int alpha, int beta);
	TT* tt_current();
	void tt_print(TT* entry);

	void gh_store();
	void gh_remove();
	bool gh_match();
	void gh_clear_played();

	void add_legal_moves();
	void add_move(Move move);
	void make_move(Move move);
	void unmake_move(Move move);
	void traverse_forwards(Line* line, int dist);
	void traverse_backwards(Line* line, int dist);
	
	void search_fixed_depth(int depth);
	int search_alphabeta(int depth, int alpha, int beta, Line* line);
	std::clock_t search_clock();
	bool search_exit(bool case_, std::string message);

	void init_eval_boards();
	int evaluate();
	bool is_mate_eval(int value);

	std::string to_string();
	void print();

	void __CHART_TIME(int index);
	void __PRINT_CHART();
	void __DEBUG(bool case_, std::string message);

};

} // end namespace Bbot2