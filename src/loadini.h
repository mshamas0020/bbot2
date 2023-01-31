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

	// USER INTERFACE

	// user-controlled side(s)
	string s = string(ini.GetValue("USER_INTERFACE", "white-is"));
	CP1 = s == "computer";

	s = string(ini.GetValue("USER_INTERFACE", "black-is"));
	CP2 = s == "computer";

	// perspective
	s = string(ini.GetValue("USER_INTERFACE", "perspective"));

	PERSPECTIVE = AUTO;
	if (s == "white") PERSPECTIVE = FIXED_WHITE;
	if (s == "black") PERSPECTIVE = FIXED_BLACK;


	// COMPUTER-PLAYER

	// time limit
	s = string(ini.GetValue("COMPUTER_PLAYER", "time-limit"));
	TIME_LIMIT = std::stoi(s);

	// depth limit
	s = string(ini.GetValue("COMPUTER_PLAYER", "depth-limit"));
	DEPTH_LIMIT = std::stoi(s);

	// transposition table allocation
	s = string(ini.GetValue("COMPUTER_PLAYER", "transposition-table-allocation"));
	TT_ALLOC = std::stoi(s);

	// round to nearest 2^n
	u_long n = 1;
	while (n < TT_ALLOC) n <<= 1;
	TT_ALLOC = n;
}

} // end namespace Bbot2