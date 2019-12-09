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

	while(1) {
		auto recv_status = ::recv(receiving_socket, buffer.data(), BUFFER_SIZE, MSG_DONTWAIT);

		if (recv_status == ERROR_STATUS) {
			LoggerLogStatusErrorWithLineAndFile("recv err", recv_status);

			if(errno == EAGAIN or errno == EWOULDBLOCK) {
				LoggerLogStatusErrorWithLineAndFile("EAGAIN or EWOULDBLOCK receving", recv_status);
				break;
			}
			saveSocketToClose(receiving_socket);
			return { std::vector<char>{}, true };
		}
		else if (recv_status == CLOSE_STATUS) {
			LoggerLogStatusErrorWithLineAndFile("recv", recv_status);

			if(msg.empty()) {
				saveSocketToClose(receiving_socket);
				return { std::vector<char>{}, true };
			}
			std::cout << "close_status" << std::endl;
			break;
		}
		else
		{
			std::copy_n(buffer.begin(), recv_status, std::back_inserter(msg));
		}

		//auto string_end = std::find(buffer.begin(), buffer.end(), '\0');
		//std::string msg_part{buffer.begin(), string_end};

		if (recv_status < BUFFER_SIZE) // TODO: should respond with too large payload
			break;
	}

	//auto sock_data_elem = std::find_if(sock_sockData_from_server.begin(), sock_sockData_from_server.end(), [receiving_socket](const auto& el) { return el.first == receiving_socket; });
	//print_rcvd_msg(sock_data_elem, msg.data());

	return { std::move(msg), false };
}

void Receiver::print_rcvd_msg(const std::vector<std::pair<socket_t, sockaddr_in>>::iterator sock_data_elem, const std::string& msg) const {
	std::cout << "recv from"
	      << " address = " << inet_ntoa(sock_data_elem->second.sin_addr)
	  	  << " port = " << ntohs(sock_data_elem->second.sin_port)
		  << " msg:\n"<< msg << std::endl;
}

void Receiver::saveSocketToClose(socket_t sock_to_close) {
	sockets_to_close.push_back(sock_to_close);
}

void Receiver::closeSocket(socket_t sock_to_close) {
	LoggerLogStatusWithLineAndFile("socket " + std::to_string(sock_to_close) + " close", CLOSE_STATUS);
	close(sock_to_close);

	auto pollfd_elem = std::find_if(pollfd_list_from_server.begin(), pollfd_list_from_server.end(), [sock_to_close](const auto& el) { return el.fd == sock_to_close; });
	pollfd_list_from_server.erase(pollfd_elem);

	auto sock_data_elem = std::find_if(sock_sockData_from_server.begin(), sock_sockData_from_server.end(), [sock_to_close](const auto& el) { return el.first == sock_to_close; });
	sock_sockData_from_server.erase(sock_data_elem);
}

void Receiver::closeSavedSockets() {
	sockets_to_close.unique();

	for(auto sock : sockets_to_close) {
		closeSocket(sock);
	}

	sockets_to_close.clear();
}