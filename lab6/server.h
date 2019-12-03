#pragma once

#include <sys/socket.h>
#include <netinet/in.h>
#include <poll.h>

#include <vector>
#include <memory>
#include <optional>
#include <map>

#include "logger.h"
#include "receiver.h"

#include <openssl/ssl.h>
#include <openssl/err.h>

using SSL_unique =  std::unique_ptr<SSL, decltype(&SSL_free)>;
using CTX_unique =  std::unique_ptr<SSL_CTX, decltype(&SSL_CTX_free)>;

class Server {
public:
	Server(sa_family_t sin_family,
		   in_port_t sin_port,
  		   const char* sin_addr,
  		   CTX_unique& ctx);

	void createSocket(int domain, int type, int protocol);
	void bindSocket(socklen_t address_len);
	void listen();

private:
	void accept();
	void ssl_accept(int new_sock);
	void startPoll();
	void recvAndSend(int receiving_socket);
	std::optional<std::string> recv(int receiving_socket);
	void send(int receiving_socket, const std::optional<std::string>& buffer);

	int sock_receiving;
	std::vector<pollfd> pollfd_list;
	std::vector<std::pair<int, sockaddr_in>> sock_sockData;
	std::map<int, SSL*> sock_SSL;
	int backlog = SOMAXCONN;
	sockaddr_in address;
	Logger logger;
	Receiver receiver{pollfd_list, sock_sockData, sock_SSL};
	CTX_unique& context;
};

