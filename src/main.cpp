// main.cpp

#include <cstdlib>
#include <ctime>

#include "common.h"
#include "gui.h"
#include "loadini.h"
#include "log.h"

namespace Bbot2 {

void play() {

	// initialize

	// .ini
	loadINI();
	__LOG_VERBOSE(INI_FILE + " loaded");

	// bitboard
	Bitboard::init();
	__LOG_VERBOSE("Bitboard values generated");

	// move-gen
	Tables::init();
	__LOG_VERBOSE("Move-gen tables generated");

	// board
	Board board;
	board.init();
	__LOG_VERBOSE("Board initialized");

	// game
	Game game(&board);

	// add computers
	Bbot comp[NUM_SIDES] = { Bbot(&game), Bbot(&game) };

	if (S_CP1) {
		comp[WHITE].init();
		game.add_player(&comp[WHITE], WHITE);
		__LOG_VERBOSE("CP1 initialized");
	}

	if (S_CP2) {
		comp[BLACK].init();
		game.add_player(&comp[BLACK], BLACK);
		__LOG_VERBOSE("CP2 initialized");
	}

	// GUI
	GUI graphics(&game);
	graphics.init();
	__LOG_VERBOSE("GUI initialized");
	__LOG_VERBOSE("\n");


	// main loop
	try {
		while (!graphics.loop_end()) {
			game.update();
			graphics.update();
			graphics.draw();
		}
	} catch (Exception e) {
		e.print();
	}

	// close

	// GUI
	graphics.close();


	// computers
	if (S_CP1) {
		comp[WHITE].close();
		__LOG_VERBOSE("CP1 closed");
	}

	if (S_CP2) {
		comp[BLACK].close();
		__LOG_VERBOSE("CP2 closed");
	}

	// board
	board.close();
	__LOG_VERBOSE("Board closed");

	// move-gen
	Tables::close();
	__LOG_VERBOSE("Move-gen tables deallocated");
}

} // end namespace Bbot2

int main(int argc, char* args[]) {
	Bbot2::play();

	return 0;
}
