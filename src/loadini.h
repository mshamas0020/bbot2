// loadini.h

#pragma once


#include "common.h"
#include "log.h"
#include "../external/SimpleIni/SimpleIni.h"

using std::string;

namespace Bbot2 {

string INI_FILE = "SETTINGS.ini";

void loadINI() {

	// load file
	CSimpleIniA ini;
	ini.SetUnicode();
	SI_Error e = ini.LoadFile(INI_FILE.c_str());

	if (e != SI_OK) throw Exception("Could not load " + INI_FILE);

	// GAME

	// user-controlled side(s)
	string s = string(ini.GetValue("GAME", "white-is"));
	S_CP1 = s == "computer";

	s = string(ini.GetValue("GAME", "black-is"));
	S_CP2 = s == "computer";

	// perspective
	s = string(ini.GetValue("GAME", "perspective"));

	S_PERSPECTIVE = AUTO;
	if (s == "white") S_PERSPECTIVE = FIXED_WHITE;
	if (s == "black") S_PERSPECTIVE = FIXED_BLACK;


	// COMPUTER-PLAYER

	// time limit
	s = string(ini.GetValue("COMPUTER_PLAYER", "time-limit"));
	S_TIME_LIMIT = std::stoi(s);

	// depth limit
	s = string(ini.GetValue("COMPUTER_PLAYER", "depth-limit"));
	S_DEPTH_LIMIT = std::stoi(s);

	// transposition table allocation
	s = string(ini.GetValue("COMPUTER_PLAYER", "transposition-table-allocation"));
	S_TT_ALLOC = std::stoi(s);

	// round to nearest 2^n
	u_long n = 1;
	while (n < S_TT_ALLOC) n <<= 1;
	S_TT_ALLOC = n;


	// LAYOUT

	s = string(ini.GetValue("LAYOUT", "screen-width"));
	S_SCREEN_WIDTH = std::stoi(s);

	s = string(ini.GetValue("LAYOUT", "screen-height"));
	S_SCREEN_HEIGHT = std::stoi(s);

	s = string(ini.GetValue("LAYOUT", "min-game-size"));
	S_MIN_GAME_SIZE = std::stoi(s);

	s = string(ini.GetValue("LAYOUT", "info-box-height"));
	S_INFO_BOX_HEIGHT = std::stoi(s);

	s = string(ini.GetValue("LAYOUT", "margin"));
	S_MARGIN = std::stoi(s);

	s = string(ini.GetValue("LAYOUT", "font-size"));
	S_FONT_SIZE = std::stoi(s);
}

} // end namespace Bbot2
