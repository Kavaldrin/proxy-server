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

	// void print_rcvd_msg(const std::vector<std::pair<int, sockaddr_in>>::iterator sock_data_elem) const {
	// std::cout << "recv from"
	//       << " address = " << inet_ntoa(sock_data_elem->second.sin_addr)
	//   	  << " port = " << ntohs(sock_data_elem->second.sin_port)
	// 	  << std::endl;
	// }
};

