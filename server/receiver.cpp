#include "receiver.h"

#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>

#include <iostream>
#include <string>
#include <algorithm>
#include <array>

constexpr int ERROR_STATUS = -1;
constexpr int CLOSE_STATUS = 0;
constexpr unsigned BUFFER_SIZE = 8196;


std::pair< std::vector<char>, bool> Receiver::recv(socket_t receiving_socket) {
	
	std::vector<char>  buffer{}; 
	buffer.resize(BUFFER_SIZE);
	std::vector<char> msg;

	while(true) {
		auto recv_status = ::recv(receiving_socket, buffer.data(), BUFFER_SIZE, MSG_DONTWAIT);

		if (recv_status == ERROR_STATUS) {
			LoggerLogStatusErrorWithLineAndFile("recv err", recv_status);

			if(errno == EAGAIN or errno == EWOULDBLOCK) {
				LoggerLogStatusErrorWithLineAndFile("EAGAIN or EWOULDBLOCK receving", recv_status);
				break;
			}

			return { std::vector<char>{}, true };
		}
		else if (recv_status == CLOSE_STATUS) {
			//LoggerLogStatusErrorWithLineAndFile("recv", recv_status);

			return { std::vector<char>{}, true };
		
		}
		else
		{
			std::copy_n(buffer.begin(), recv_status, std::back_inserter(msg));
		}

		if (recv_status < BUFFER_SIZE) // TODO: should respond with too large payload
			break;
	}

	// print_rcvd_msg(sock_data_elem, msg.data());


	return { std::move(msg), false };
}

// void Receiver::print_rcvd_msg(const std::vector<std::pair<socket_t, sockaddr_in>>::iterator sock_data_elem, const std::string& msg) const {
// 	std::cout << "recv from"
// 	      << " address = " << inet_ntoa(sock_data_elem->second.sin_addr)
// 	  	  << " port = " << ntohs(sock_data_elem->second.sin_port)
// 		  << " msg:\n"<< msg << std::endl;
// }
