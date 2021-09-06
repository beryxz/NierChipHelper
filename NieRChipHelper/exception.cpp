#include "pch.h"
#include "exception.h"

int filterException(int code, PEXCEPTION_POINTERS ex) {
	std::cout << "Filtering " << std::hex << code << std::endl;
	return EXCEPTION_EXECUTE_HANDLER;
}
