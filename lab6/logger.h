#pragma once

#include <iostream>
#include <cstring>

struct Logger {
	void logStatusError(const std::string& operation_name, int status) {
		std::cerr << operation_name << " status = " << status << std::endl;
		std::cerr << strerror(errno) << std::endl;
	}

	void logStatus(const std::string& operation_name, int status) {
		std::cout << operation_name << " status = " << status << std::endl;
	}
};

