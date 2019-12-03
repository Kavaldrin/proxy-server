#include "server.h" 

#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>

#include <iostream>
#include <string>
#include <algorithm>
#include <array>

#include "parserHttp.h"
#include "httpResponseBuilder.h"


constexpr int ERROR_STATUS = -1;
constexpr unsigned BUFFER_SIZE = 200;

Server::Server(sa_family_t sin_family,
			   in_port_t sin_port,
  			   const char* sin_addr,
  			   CTX_unique& ctx) 
			   : context(ctx) {
	address.sin_family = sin_family;
	address.sin_port = htons(sin_port);
	address.sin_addr = in_addr{ inet_addr(sin_addr) };
}

void Server::createSocket(int domain, int type, int protocol) {
	sock_receiving = socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK, 0);
	if (sock_receiving == ERROR_STATUS) {
		logger.logStatusError("socket", sock_receiving);
		return;
	}

	int optval = 1;
    setsockopt(sock_receiving, SOL_SOCKET, SO_REUSEPORT, &optval, sizeof(optval));

	pollfd_list.push_back(pollfd{sock_receiving, POLLIN});
	sock_sockData.push_back({sock_receiving, address});


	logger.logStatus("socket", sock_receiving);
}

void Server::bindSocket(socklen_t address_len) {
	auto result = bind(sock_receiving, reinterpret_cast<const sockaddr*>(&address), address_len);

	if (result == ERROR_STATUS) {
		logger.logStatusError("bind", result);
		perror("bind");
		return;
	}

	std::cout << "bind status = " << result << std::endl;
}

void Server::listen() {
	auto listen_status = ::listen(sock_receiving, backlog);

	if (listen_status == ERROR_STATUS) {
		logger.logStatusError("listen", listen_status);
		return;
	}

	std::cout << "listen status = " << listen_status << std::endl;

	startPoll();
}

void Server::startPoll() {
	while(1) {
		int poll_result = poll(pollfd_list.data(), pollfd_list.size(), -1);

		if (poll_result == ERROR_STATUS) {
			logger.logStatusError("poll", poll_result);
			return;
		}

		for(const auto& pollfd_element : pollfd_list) {

			if (pollfd_element.revents & POLLIN) {

				if(pollfd_element.fd == sock_receiving) {
					std::cout << "new accept\n";
					accept();
				}
				else {
					std::cout << "recv and send\n";
					recvAndSend(pollfd_element.fd);
				}
			}
		}

		receiver.closeSavedSockets();
	}
}

void Server::accept() {
	sockaddr_in sender_addr_in;
	socklen_t sender_addr_len = sizeof(sockaddr_in);

	auto new_sock = ::accept(sock_receiving, reinterpret_cast<sockaddr*>(&sender_addr_in), &sender_addr_len);

	if(new_sock != ERROR_STATUS) {
		std::cout << "accept status = " << new_sock;
		pollfd_list.push_back(pollfd{new_sock, POLLIN});
		sock_sockData.push_back({new_sock, sender_addr_in});
	}
	else {
		logger.logStatusError("accept", new_sock);
		return;
	}

	std::cout << " address = " << inet_ntoa(sender_addr_in.sin_addr)
		  	  << " port = " << ntohs(sender_addr_in.sin_port)
		  	  << " protocol = " << sender_addr_in.sin_family << std::endl;

	ssl_accept(new_sock);
}

void Server::ssl_accept(int new_sock)
{
    SSL* ssl = SSL_new(context.get());
	SSL_set_fd(ssl, new_sock);

	sock_SSL.insert({new_sock, ssl});
	std::cout << "ssl inserting at " << new_sock << " value " << sock_SSL.at(new_sock) << std::endl;

	auto ssl_accept_status = SSL_accept(ssl);
	if(ssl_accept_status <= 0) {
		std::cout << "ssl_accept_status: " << ssl_accept_status << std::endl;
		ERR_print_errors_fp(stderr);
		
		switch(auto status = SSL_get_error(ssl, ssl_accept_status)) {
		case SSL_ERROR_NONE:
			std::cout << "SSL_ERROR_NONE\n";
			break;
		case SSL_ERROR_ZERO_RETURN:
			std::cout << "SSL_ERROR_ZERO_RETURN\n";
			break;
		case SSL_ERROR_WANT_READ:
			std::cout << "SSL_ERROR_WANT_READ\n";
			break;
		case SSL_ERROR_WANT_WRITE:
			std::cout << "SSL_ERROR_WANT_WRITE\n";
			break;
		case SSL_ERROR_WANT_CONNECT:
			std::cout << "SSL_ERROR_WANT_CONNECT\n";
			break;
		case SSL_ERROR_WANT_ACCEPT:
			std::cout << "SSL_ERROR_WANT_ACCEPT\n";
			break;
		case SSL_ERROR_WANT_X509_LOOKUP:
			std::cout << "SSL_ERROR_WANT_X509_LOOKUP\n";
			break;
		case SSL_ERROR_WANT_ASYNC:
			std::cout << "SSL_ERROR_WANT_ASYNC\n";
			break;
		case SSL_ERROR_WANT_ASYNC_JOB:
			std::cout << "SSL_ERROR_WANT_ASYNC_JOB\n";
			break;
		case SSL_ERROR_WANT_CLIENT_HELLO_CB:
			std::cout << "SSL_ERROR_WANT_CLIENT_HELLO_CB\n";
			break;
		case SSL_ERROR_SYSCALL:
			std::cout << "SSL_ERROR_SYSCALL\n";
			break;
		case SSL_ERROR_SSL:
			std::cout << "SSL_ERROR_SSL\n";
			break;
		default:
			std::cout << "ssl_accept default: "<< status << std::endl;
		}

	}
}

void Server::recvAndSend(int receiving_socket) {
	auto resp = recv(receiving_socket);	
	send(receiving_socket, resp); 
}

std::optional<std::string> Server::recv(int receiving_socket) {
	auto msg = receiver.recv(receiving_socket, sock_sockData);
	if(msg) {
		ParserHttp prs{*msg};
		auto resp = prs.parse();

		return resp;
	}
	return std::nullopt;
}

void Server::send(int receiving_socket, const std::optional<std::string>& buffer) {
	auto response = HttpResponseBuilder(buffer).build();
	// auto send_status = ::send(receiving_socket, response.c_str(), response.length(), 0);
	auto send_status = SSL_write(sock_SSL.at(receiving_socket), response.c_str(), response.length());

	if (send_status == ERROR_STATUS) {
		logger.logStatusError("send", send_status);
		return;
	}

	logger.logStatus("send", send_status);
	receiver.saveSocketToClose(receiving_socket);
}

