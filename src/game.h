// game.h

// Game class, contains board and manages computer players
// informs GUI class
// also responsible for history, detecting repetition, and finding game-over

#pragma once

#include "common.h"
#include "log.h"
#include "bbot.h"
#include "board.h"

namespace Bbot2 {

class Bbot;

// Game History entry

// linked list used for game history
// - in game.history to check draw by repetition for moves in playedLine
// - in Bbot TT for quick draw by rep detection in search tree
//		- a unique head is stored in every TT entry, listing every position with the same key
//		- collisions resolved by lengthening linked list
typedef struct GH {
	Key key;
	GH* next = nullptr;
	u_short occurrences = 1;

	GH(Key key_) : key(key_) {};
} GH;


// Game
class Game {
public:
	////

	//// SETTINGS ////

	// search limits
	int searchMaxTime = 1000; // milliseconds
	int searchMaxDepth = MAX_LINE_LEN; // capped by MAX_LINE_LEN

	// usually overrided by .ini in settings()


	// draw rule
	// if a position occurs x times, the game ends in a draw
	const int REPETITIONS_TO_DRAW = 3;

	////

	// board
	Board* board;
	Bbot* players[NUM_SIDES] = { nullptr, nullptr }; // Bbot* for computer, nullptr for user
	Outcome outcome = OUTCOME_NONE; // WIN_WHITE, WIN_BLACK, DRAW_BY_REP, or OUTCOME_NONE if game still ongoing

	LineVector playedLine; // canonical line of moves played in game so far
	GH* history = nullptr; // head for game history - linked list of keys appearing in played line
	int ply = 0; // count of half-moves so far

	// search info
	bool searching;
	std::string searchEval = "0";
	std::string searchPV;
	int searchDepth;
	double searchDuration;
	int searchSpeed;

	// true for one cycle if move was played
	// triggers an update for GUI
	bool movePlayed = false; 

public:
	bool initialized = false;

	Game(Board* board_);

	void settings();
	void init();
	void reset();
	void attach_board(Board* board_);
	void add_player(Bbot* comp, Side side);

	void update();

	void play_move(Move move);
	bool user_to_play();

	bool game_lost();
	bool game_over();

	void close();

private:
	void add_history();

	void __DEBUG(bool condition, std::string message);
};

} // end namespace Bbot2
