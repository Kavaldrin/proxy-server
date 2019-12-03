#include "receiver.h"

#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>

#include <openssl/ssl.h>

#include <iostream>
#include <string>
#include <algorithm>
#include <array>

constexpr int ERROR_STATUS = -1;
constexpr int CLOSE_STATUS = 0;
constexpr unsigned BUFFER_SIZE = 10;


std::optional<std::string> Receiver::recv(socket_t receiving_socket,
										  std::vector<std::pair<int, sockaddr_in>>& sock_sockData) {
	std::optional<std::string> msg{};

	while(1) {
		std::array<char, BUFFER_SIZE> buffer;
		// auto recv_status = ::recv(receiving_socket, buffer.data(), buffer.max_size(), MSG_DONTWAIT);
		auto recv_status = SSL_read(sock_SSL_from_server.at(receiving_socket), buffer.data(), buffer.max_size());

		if (recv_status == ERROR_STATUS) {
			logger.logStatusError("recv err", recv_status);

			if(errno == EAGAIN or errno == EWOULDBLOCK) {
				break;
			}
			saveSocketToClose(receiving_socket);
			return std::nullopt;
		}
		if (recv_status == CLOSE_STATUS) {
			logger.logStatusError("recv", recv_status);

			if(not msg) {
				saveSocketToClose(receiving_socket);
				return std::nullopt;
			}

			break;
		}

		auto string_end = std::find(buffer.begin(), buffer.end(), '\0');
		std::string msg_part{buffer.begin(), string_end};
		(*msg) += msg_part;

		if (not SSL_has_pending(sock_SSL_from_server.at(receiving_socket)))
			break;
	}


	auto sock_data_elem = std::find_if(sock_sockData_from_server.begin(), sock_sockData_from_server.end(), [receiving_socket](const auto& el) { return el.first == receiving_socket; });
	print_rcvd_msg(sock_data_elem, (*msg));

	return *msg;
}

void Receiver::saveSocketToClose(socket_t sock_to_close) {
	sockets_to_close.push_back(sock_to_close);
}

void Receiver::closeSocket(socket_t sock_to_close) {
	logger.logStatus("socket " + std::to_string(sock_to_close) + " close", CLOSE_STATUS);
	close(sock_to_close);

	SSL_free(sock_SSL_from_server.at(sock_to_close));
	sock_SSL_from_server.erase(sock_to_close);

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

void Receiver::print_rcvd_msg(const std::vector<std::pair<int, sockaddr_in>>::iterator sock_data_elem, const std::string& msg) const {
	std::cout << "recv from"
	      << " address = " << inet_ntoa(sock_data_elem->second.sin_addr)
	  	  << " port = " << ntohs(sock_data_elem->second.sin_port) << std::endl;
}