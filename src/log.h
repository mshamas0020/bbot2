// log.h

// log and log verbose functions
// Exception class
// switches are in common.h

#pragma once

#include "common.h"
#include <source_location>

namespace Bbot2 {

void __PRINT(std::string message);
void __LOG(std::string message);
bool __LOG(bool condition, std::string message);
void __LOG_VERBOSE(std::string message);
bool __LOG_VERBOSE(bool condition, std::string message);

class Exception : public std::exception {
public:
	std::string message;
	std::source_location location;

	// VS is tempermental with source_location::current() as a default argument
	// this constructor cannot be declared beforehand
	// and calling this as Exception e("") breaks, while Exception e = Exception("") works just fine
	Exception(std::string message_, std::source_location location_ = std::source_location::current())
		: message(message_), location(location_) {}

	void print();
};

} // end namespace Bbot2