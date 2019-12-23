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
  		   const char* sin_addr,
		   std::string account_number);

	void createSocket(int domain, int type, int protocol);
	void bindSocket(socklen_t address_len);
	void listen();

private:
	void accept();
	void startPoll();
	void recvAndSend(int receiving_socket);
	bool recv(int socket) noexcept;
	bool send(int socket) noexcept;
	std::pair<int, int> connect(std::string destination, std::optional<std::string> = {});

	std::string m_account_number;
	int sock_receiving;
	std::vector<pollfd> pollfd_list;
	std::vector<std::pair<int, sockaddr_in>> sock_sockData;
	Proxy::ProxyManager m_proxyManager;
	std::vector<int> m_socketsToRemove;
	std::vector<pollfd> m_socketsToAdd;
	int backlog = SOMAXCONN;
	sockaddr_in address;
	Logger logger;
	Receiver receiver;
};

