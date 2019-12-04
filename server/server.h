#pragma once

#include <sys/socket.h>
#include <netinet/in.h>
#include <poll.h>

#include <vector>
#include <memory>
#include <optional>

#include "parserHttp.h"
#include "logger.h"
#include "receiver.h"

class Server {
public:
	Server(sa_family_t sin_family,
		   in_port_t sin_port,
  		   const char* sin_addr);

	void createSocket(int domain, int type, int protocol);
	void bindSocket(socklen_t address_len);
	void listen();

private:
	void accept();
	void startPoll();
	void recvAndSend(int receiving_socket);
	std::optional<HttpRequest_t> recv(int receiving_socket);
	void send(int receiving_socket, const std::optional<HttpRequest_t>& buffer);

	int sock_receiving;
	std::vector<pollfd> pollfd_list;
	std::vector<std::pair<int, sockaddr_in>> sock_sockData;
	int backlog = SOMAXCONN;
	sockaddr_in address;
	Logger logger;
	Receiver receiver{pollfd_list, sock_sockData};
};

