#include "server.h" 

#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <netdb.h>

#include <boost/algorithm/string.hpp>

#include <iostream>
#include <string>
#include <algorithm>
#include <array>

#include "getTime.h"
#include "httpResponseBuilder.h"


constexpr int ERROR_STATUS = -1;
constexpr unsigned BUFFER_SIZE = 200;

Server::Server(sa_family_t sin_family,
			   in_port_t sin_port,
  			   const char* sin_addr) {
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

				if(pollfd_element.fd == sock_receiving)
					accept();
				else
					recvAndSend(pollfd_element.fd);
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
}

void Server::recvAndSend(int receiving_socket) {
	auto resp = recv(receiving_socket);	
	send(receiving_socket, resp); 
}

std::optional<HttpRequest_t> Server::recv(int receiving_socket) {
	auto msg = receiver.recv(receiving_socket);
	if(msg) {
		ParserHttp prs{*msg};
		auto resp = prs.parse();

		return resp;
	}
	return std::nullopt;
}

void Server::send(int receiving_socket, const std::optional<HttpRequest_t>& request) {
	std::cout << "send\n";
	if(request->find("METHOD")->second == "GET") {
		std::vector<std::string> host;

		auto host_name = request->find("Host");
		std::cout << host_name->second << std::endl;
		if(host_name != request->end()){
			boost::split(host, host_name->second, boost::is_any_of(":"));
			std::cout << host[0]<< std::endl;
			if(host.size() == 1)
				host.push_back("80");

			std::cout << "koniec: " << host[0] << " " << host[1] << std::endl;
		}
		else std::cout << "no host name\n";

		auto hostaddr = gethostbyname(host[0].c_str());
		if(hostaddr == NULL){
			std::cout << "NULL\n";
			std::cout << strerror(h_errno) << std::endl;
		}

		auto serv_sock = socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK, 0);
		if (serv_sock == ERROR_STATUS) {
			logger.logStatusError("socket", serv_sock);
			return;
		}

		int optval = 1;
	    setsockopt(serv_sock, SOL_SOCKET, SO_REUSEPORT, &optval, sizeof(optval));
		// auto target = *(sockaddr_in*)(info->ai_addr);

		sockaddr_in address;
	    address.sin_family = AF_INET;
		address.sin_port = htons(std::stoi(host[1]));
		address.sin_addr.s_addr = *(long *)(hostaddr->h_addr_list[0]);
		std::cout << "recv from"
	    << " address = " << inet_ntoa(address.sin_addr)
	    << " port = " << ntohs(address.sin_port)
	    << " family = " << (address.sin_family)
		<< std::endl;

		while(1){
		    auto connect_status = ::connect(serv_sock, reinterpret_cast<const sockaddr*>(&(address)), sizeof(address));

			if (connect_status == -1) {
				logger.logStatusError("connect", connect_status);
				if(errno != EINPROGRESS and errno != EALREADY)
					return;
			}
			else {
				pollfd_list.push_back(pollfd{serv_sock, POLLIN});
				sock_sockData.push_back({serv_sock, address}); 
				break;
			}
		}
		auto msg = request->find("MSG")->second;

		auto send_status = ::send(serv_sock, msg.c_str(), msg.length(), 0);

		if (send_status == -1) {
			logger.logStatusError("send", send_status);
			return;
		}
		logger.logStatus("send", send_status);

		while(1)
		{
			std::array<char, 8196> buffer;
			int recv_status;
			std::optional<std::string> response;
			while(1) {
				response = receiver.recv(serv_sock);
				if(response != std::nullopt) {
					std::cout << "is nullopt " << *response << std::endl;
					break;
				}
			}

			std::cout << *response <<std::endl;

			send_status = ::send(receiving_socket, response->c_str(), response->length(), 0);
			logger.logStatus("send in loop", send_status);

			std::string possible_chunk_end{response->end()-5, response->end()};
			std::cout << possible_chunk_end << std::endl;
			const std::string chunk_end{"0\r\n\r\n"};
			if(chunk_end == chunk_end) {
				std::cout << "chunk end" << std::endl;
				receiver.saveSocketToClose(receiving_socket);
				receiver.saveSocketToClose(serv_sock);

				return;
			}
		}
		// receiver.saveSocketToClose(receiving_socket);
		// ::close(serv_sock);

		return;
	}

	// auto response = HttpResponseBuilder(request->find("PATH")->second).build();
	// std::cout << "send\n" << response;

	// auto send_status = ::send(receiving_socket, response.c_str(), response.length(), 0);

	// if (send_status == ERROR_STATUS) {
	// 	logger.logStatusError("send", send_status);
	// 	return;
	// }

	// logger.logStatus("send", send_status);
	// receiver.saveSocketToClose(receiving_socket);
}

