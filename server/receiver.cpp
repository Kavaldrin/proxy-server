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

			return { std::vector<char>{}, true };

		}
		else
		{
			std::copy_n(buffer.begin(), recv_status, std::back_inserter(msg));
		}

		if (recv_status < BUFFER_SIZE)
			break;
	}

	return { std::move(msg), false };
}

