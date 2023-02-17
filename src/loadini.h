// loadini.h

#pragma once


#include "common.h"
#include "log.h"
#include "../external/SimpleIni/SimpleIni.h"

namespace Bbot2 {

void load_INI() {

	// load file
	ini.SetUnicode();
	ini.SetMultiLine(true);
	SI_Error e = ini.LoadFile(INI_FILE.c_str());

	if (e != SI_OK)
		throw Exception("Could not load " + INI_FILE);

	// transposition table allocation
	TT_ALLOC = std::stoi(std::string(ini.GetValue("COMPUTER_PLAYER", "transposition-table-allocation")));

	// round to nearest 2^n
	u_long n = 1;
	while (n < TT_ALLOC) n <<= 1;
	TT_ALLOC = n;
}

} // end namespace Bbot2
