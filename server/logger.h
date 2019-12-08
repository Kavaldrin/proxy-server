
#pragma once

#include <iostream>
#include <cstring>





struct Logger {
	static void logStatusError(const std::string& operation_name, int status) {
		std::cerr << operation_name << " status = " << status << std::endl;
		std::cerr << strerror(errno) << std::endl;
	}

	static void logStatus(const std::string& operation_name, int status) {
		std::cout << operation_name << " status = " << status << std::endl;
	}

	static void _logStatusErrorWithLineAndFile(const char* file, int line, const std::string& operation_name, int status)
	{
		std::cerr << "LOGGER: ";
		std::cerr << file << " " << line << std::endl;
		logStatusError(operation_name, status);
	}

	static void _logStatusWithLineAndFile(const char* file, int line, const std::string& operation_name, int status)
	{
		std::cerr << "LOGGER: ";
		std::cerr << file << " " << line << std::endl;
		logStatus(operation_name, status);
	}



	// void print_rcvd_msg(const std::vector<std::pair<int, sockaddr_in>>::iterator sock_data_elem, const std::string& msg) const {
	// std::cout << "recv from"
	//       << " address = " << inet_ntoa(sock_data_elem->second.sin_addr)
	//   	  << " port = " << ntohs(sock_data_elem->second.sin_port)
	// 	  << " msg: "<< msg << std::endl;
	// }
};

#define LoggerLogStatusErrorWithLineAndFile(operation_name, status) \
	Logger::_logStatusErrorWithLineAndFile(__FILE__, __LINE__, operation_name, status)

#define LoggerLogStatusWithLineAndFile(operation_name, status) \
	Logger::_logStatusWithLineAndFile(__FILE__, __LINE__, operation_name, status)
