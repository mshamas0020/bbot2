// game.cpp

#pragma once

#include "game.h"

using std::string;

namespace Bbot2 {

// constructor
Game::Game(Board* board_)
	: board(board_) {};

////

// get settings from .ini
void Game::settings() {
	searchMaxTime = std::stoi(string(ini.GetValue("COMPUTER_PLAYER", "time-limit")));
	searchMaxDepth = std::stoi(string(ini.GetValue("COMPUTER_PLAYER", "depth-limit")));
}

// init
void Game::init() {
	if (!board->initialized)
		board->init();

	
	ply = 0;
	outcome = OUTCOME_NONE;
	playedLine.movesVector.clear();
	playedLine.length = 0;
	history = nullptr;
	movePlayed = false;

	string searchEval = "0";
	string searchPV = "";
	int searchDepth = 0;
	double searchDuration = 0;
	int searchSpeed = 0;

	add_history();

	initialized = true;
}

// reset
void Game::reset() {
	for (Side side : SIDES)
		if (players[side] != nullptr)
			players[side]->soft_close();
		
	close();
	board->reset();
	init();
}

// attach new board and reset necessary values
void Game::attach_board(Board* board_) {
	board = board_;
	outcome = OUTCOME_NONE;

	for (Side side : SIDES) {
		if (players[side] != nullptr)
			players[side]->close();

		players[side] = nullptr;
	}
}

// attach Bbot to either white or black. if none assigned, a user player is the default
void Game::add_player(Bbot* comp, Side side) {
	players[side] = comp;
}

////

// main update
void Game::update() {

	if (game_over() || user_to_play())
		return;

	// search
	Bbot* comp = players[board->sideToMove];
	comp->search(searchMaxTime, searchMaxDepth);

	// play move is search has exited
	if (!comp->search_ongoing())
		play_move(comp->suggested_move());

	// update search values
	// to be read by GUI
	searching = comp->search_ongoing();
	searchEval = comp->search_eval();
	searchPV = comp->search_PV();
	searchDepth = comp->search_depth();
	searchDuration = comp->search_duration();
	searchSpeed = comp->search_speed();
}

////

// play move on board, then update players
void Game::play_move(Move move) {

	movePlayed = false;

	// get piece
	Piece* p = board->pointerBoard[move.get_from()];

	__DEBUG(!user_to_play() && p == nullptr, "Computer tried to move with no piece at origin: " + board->move_to_string(move));
	__DEBUG(!user_to_play() && !p->moveBoard[move.get_to()], "Computer tried to play illegal move: " + board->move_to_string(move));

	// exit if invalid 'from'
	if (p == nullptr)
		return;

	// check if 'to' is found in legal move set
	if (!p->moveBoard[move.get_to()])
		return;

	__LOG(format("[{}]: {}", SIDE_NAMES[board->sideToMove], board->move_to_string(move)));

	// play move on board
	board->move_piece(p, move.get_to());

	// add to played line
	playedLine.append(move);

	// increment move count
	ply ++;

	// add game history
	add_history();

	// check if lost
	if (game_lost())
		outcome = board->sideToMove ? WIN_WHITE : WIN_BLACK;

	// let comps know
	for (Side side : SIDES)
		if (players[side] != nullptr)
			players[side]->on_move_played(move);

	// update legal moves
	board->update_move_sets();

	// if game over, clear moves
	if (game_over())
		for (Side side : SIDES)
			for (Piece* p : board->pieces[side])
				p->moveBoard.reset();

	movePlayed = true;
}

// if no assigned computer this turn, assume user is playing
bool Game::user_to_play() {
	return players[board->sideToMove] == nullptr;
}

////

// check if side to move has already lost
bool Game::game_lost() {
	return (board->occupancyBySide[!board->sideToMove] & board->wateringHoles).count() >= NUM_WH_TO_WIN;
}

// check if game is over
bool Game::game_over() {
	return outcome != OUTCOME_NONE;
}

////

void Game::close() {

	// deallocate game history
	GH* gh = history;
	GH* next;

	// scan through list and delete
	while (gh != nullptr) {
		next = gh->next;
		delete gh;
		gh = next;
	}

	history = nullptr;
	initialized = false;
}

////////////////

// add a position key to the game history linked list
// if key already exists, it will increment its entry
// if a position occurs enough times, the game ends in a draw
void Game::add_history() {
	GH* gh = history;

	// if head doesn't exist, create head
	if (gh == nullptr) {
		history = new GH(board->key);
		return;
	}

	// scan list
	while (gh->next != nullptr) {

		// if found, increment occurrences
		if (gh->key == board->key) {
			gh->occurrences ++;

			// if too many occurrences, end in draw
			if (gh->occurrences >= REPETITIONS_TO_DRAW)
				outcome = DRAW_REPETITION;

			return;
		}

		gh = gh->next;
	}

	// add to end of list if not found
	gh->next = new GH(board->key);
}

////

void Game::__DEBUG(bool condition, std::string message) {
	if constexpr (DEBUG)
		if (condition)
			__PRINT("PLAYED LINE: " + playedLine.to_string(board->startPointerBoard) + "\n");

	if (board)
		board->__DEBUG(condition, message);
}

} // end namespace Bbot2
