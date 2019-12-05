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
#include "ProxyManager.hpp"

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
	bool recv(int socket) noexcept;
	void send(int receiving_socket, const std::optional<HttpRequest_t>& buffer);
	int connect(std::string destination);

	int sock_receiving;
	std::vector<pollfd> pollfd_list;
	std::vector<std::pair<int, sockaddr_in>> sock_sockData;
	Proxy::ProxyManager m_proxyManager;
	int backlog = SOMAXCONN;
	sockaddr_in address;
	Logger logger;
	Receiver receiver{pollfd_list, sock_sockData};
};

