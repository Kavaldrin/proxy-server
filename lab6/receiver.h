#pragma once

#include <sys/socket.h>
#include <netinet/in.h>
#include <poll.h>

#include <vector>
#include <memory>
#include <optional>
#include <list>
#include <map>

#include <openssl/ssl.h>

#include "logger.h"

using socket_t = int;

class Receiver {
public:
	Receiver(std::vector<pollfd>& pollfd_list,
			 std::vector<std::pair<socket_t, sockaddr_in>>& sock_sockData,
			 std::map<int, SSL*>& sock_SSL)
			   		: pollfd_list_from_server(pollfd_list),
			   		  sock_sockData_from_server(sock_sockData),
			   		  sock_SSL_from_server(sock_SSL) {}


	std::optional<std::string> recv(int receiving_socket,
									std::vector<std::pair<int, sockaddr_in>>& sock_sockData);

	void closeSavedSockets();
	void saveSocketToClose(socket_t sock_to_close);

private:
	void print_rcvd_msg(std::vector<std::pair<socket_t, sockaddr_in>>::iterator sock_data_elem, const std::string& msg) const;
	void closeSocket(socket_t sock_to_close);


	Logger logger;
	std::list<socket_t> sockets_to_close;
	std::vector<pollfd>& pollfd_list_from_server;
	std::vector<std::pair<socket_t, sockaddr_in>>& sock_sockData_from_server;
	std::map<int, SSL*>& sock_SSL_from_server;
};