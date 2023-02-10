// log.cpp

#pragma once

#include "log.h"

using std::string;
using std::format;

namespace Bbot2 {

void __PRINT(string message) {
	std::cout << message;
}

void __LOG(string message) {
	if constexpr (LOG)
		__PRINT(message + "\n");
}

bool __LOG(bool condition, string message) {
	if constexpr (LOG)
		if (condition)
			__PRINT(message + "\n");

	return condition;
}

void __LOG_VERBOSE(string message) {
	if constexpr (LOG && LOG_VERBOSE)
		__PRINT(message + "\n");
}

bool __LOG_VERBOSE(bool condition, string message) {
	if constexpr (LOG && LOG_VERBOSE)
		if (condition)
			__PRINT(message + "\n");

	return condition;
}

void Exception::print() {
	__PRINT(format("ERROR: {}\n{}({}:{}) in {}\n", message, location.file_name(), location.line(), location.column(), location.function_name()));
}

} // end namespace Bbot2
