#pragma once

#include <sys/socket.h>
#include <poll.h>
#include <netinet/in.h>

#include <vector>
#include <memory>
#include <list>
#include <optional>

#include "logger.h"

using socket_t = int;

class Receiver {
public:
	Receiver(std::vector<pollfd>& pollfd_list,
		 std::vector<std::pair<socket_t, sockaddr_in>>& sock_sockData)
		   		: pollfd_list_from_server(pollfd_list),
		   		  sock_sockData_from_server(sock_sockData)
		   		   {}

	std::pair < std::vector<char>, bool > recv(socket_t receiving_socket);

	void closeSavedSockets();
	void saveSocketToClose(socket_t sock_to_close);
private:
	void print_rcvd_msg(std::vector<std::pair<socket_t, sockaddr_in>>::iterator sock_data_elem, const std::string& msg) const;
	void closeSocket(socket_t sock_to_close);

	std::list<socket_t> sockets_to_close;
	std::vector<pollfd>& pollfd_list_from_server;
	std::vector<std::pair<socket_t, sockaddr_in>>& sock_sockData_from_server;

};