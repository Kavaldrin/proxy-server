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

#include "httpResponseBuilder.h"
#include <thread>
#include <chrono>
#include <tuple>

//TO DO
//naprawic zamykanie socketow


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

		std::this_thread::sleep_for(std::chrono::milliseconds(100));

		int poll_result = poll(pollfd_list.data(), pollfd_list.size(), -1 /*no timeout */);

		if (poll_result == ERROR_STATUS) {
			logger.logStatusError("poll", poll_result);
			return;
		}		

		// for(auto& pollfd_element : pollfd_list)
		// {
		// 	std::cout << pollfd_element.fd << " ";
		// }
		// std::cout << std::endl;


		for(auto& pollfd_element : pollfd_list) {

			if (pollfd_element.revents & POLLIN) {

				if(pollfd_element.fd == sock_receiving)
				{
					accept();
				}
				else
				{
					if(recv(pollfd_element.fd))
					{
						pollfd_element.events = POLLOUT;
					}
				}
			}
			else if(pollfd_element.revents & POLLOUT)
			{
				if(send(pollfd_element.fd))
				{
					pollfd_element.events = POLLIN;
				}
			}

		}

		for(const auto& soc : m_socketsToRemove)
		{
			std::cout <<"closing socket " << soc  << std::endl;
			pollfd_list.erase(std::remove_if(pollfd_list.begin(), pollfd_list.end(), [soc](const auto& s) { return soc == s.fd; } ));
		}
		m_socketsToRemove.clear();

		for(const auto& soc : m_socketsToAdd)
		{
			pollfd_list.push_back(soc);
		}
		m_socketsToAdd.clear();

	}
}

void Server::accept() {
	sockaddr_in sender_addr_in;
	socklen_t sender_addr_len = sizeof(sockaddr_in);

	auto new_sock = ::accept(sock_receiving, reinterpret_cast<sockaddr*>(&sender_addr_in), &sender_addr_len);

	if(new_sock != ERROR_STATUS) {
		std::cout << "accept status = " << new_sock;
		m_socketsToAdd.push_back( pollfd{new_sock, POLLIN} );
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

std::pair<int, int> Server::connect(std::string destination, std::optional<std::string> port)
{
	std::cout << destination << std::endl;
	auto hostaddr = gethostbyname(destination.c_str());
	if(hostaddr != NULL)
	{
		struct in_addr **addr_list;
		addr_list = (struct in_addr **)hostaddr->h_addr_list;
		char* ipAddr = inet_ntoa(*addr_list[0]);
		std::cout << ipAddr << std::endl;

		sockaddr_in dest;
		dest.sin_family = AF_INET;

		if(port.has_value())
		{
			dest.sin_port = htons(std::stoi(port.value()));
		}
		else
		{
			dest.sin_port = htons(80);
		}

		dest.sin_addr.s_addr = inet_addr(ipAddr);

		int optval = 1;
		auto serv_sock = socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK, 0);
	    setsockopt(serv_sock, SOL_SOCKET, SO_REUSEPORT, &optval, sizeof(optval));
	
		::connect(serv_sock, (sockaddr*) &dest, sizeof(sockaddr));

		return { errno == EINPROGRESS ? 0 : -1 , serv_sock};
		
	}
	else {
		std::cout << "NULL\n";
		std::cout << strerror(h_errno) << std::endl;
	}

	return {ERROR_STATUS, 0};
}

void Server::recvAndSend(int receiving_socket) {
	

}

bool Server::send(int socket) noexcept
{

	auto [shouldChangeSocketState, shouldRemoveSocket] =  m_proxyManager.handleStoredBuffers(socket);
	if(shouldRemoveSocket)
	{
		m_socketsToRemove.push_back(socket);
	}
	return shouldChangeSocketState;
}

bool Server::recv(int receiving_socket) noexcept {
	

	auto [message, isSocketClosed] = receiver.recv(receiving_socket);	
	

	if(m_proxyManager.isDestination(receiving_socket) && message.empty() && isSocketClosed)
	{
		m_proxyManager.destroyEstablishedConnectionByDestination(receiving_socket);
		m_socketsToRemove.push_back(receiving_socket);
		return false;
	}
	else if(m_proxyManager.isSource(receiving_socket) && message.empty() && isSocketClosed)
	{
		m_proxyManager.destroyEstablishedConnectionBySource(receiving_socket);
		m_socketsToRemove.push_back(receiving_socket);
		return false;
	}
	else if(message.empty())
	{
		m_socketsToRemove.push_back(receiving_socket);
		return false;
	}

	if(auto pairSocket = m_proxyManager.getSecondSocketIfEstablishedConnection(receiving_socket);
		pairSocket.has_value())
	{
		std::cout << pairSocket.value() << "<->" << receiving_socket << std::endl;
		m_proxyManager.addDataForDescriptor(pairSocket.value(), std::move(message));

		if(auto pollfdPos = std::find_if(pollfd_list.begin(), pollfd_list.end(), [&](auto& el)
			{ return el.fd == pairSocket.value(); }); pollfdPos != pollfd_list.end())
		{
			pollfdPos->events = POLLOUT;
		}

		return false;
	}

	ParserHttp parser{ std::string_view(message.data(), message.size()) };
	if(parser.isHTTPRequest())
	{
		auto[method, destination] = parser.parseStartLine();
		if(!method.has_value())
		{
			//header jest inwalida, to jakis moze bad request
			return true;
		}

		if(parser.parseMethod() == std::string("CONNECT"))
		{
			auto [baseAddress, port] = parser.getBaseAddress(destination.value());
			if(baseAddress.has_value())
			{
				auto [status, new_sock] = connect(baseAddress.value(), port);
				if(status == ERROR_STATUS)
				{
					LoggerLogStatusErrorWithLineAndFile("CONNECT connection failed\n", status);
					return false;
				}

				m_socketsToAdd.push_back( {new_sock, 0} );
				m_proxyManager.addEstablishedConnection(receiving_socket, new_sock);

				std::string okResponse = { "HTTP/1.1 200 Connection Established\r\n\r\n" };
				std::cout << okResponse << std::endl;
				m_proxyManager.addDataForDescriptor(receiving_socket, std::vector<char>(okResponse.begin(), okResponse.end()));

				if(auto pollfdPos = std::find_if(pollfd_list.begin(), pollfd_list.end(), [&](auto& el)
					{ return el.fd == receiving_socket; }); pollfdPos != pollfd_list.end())
				{
					pollfdPos->events = POLLOUT;
				}

				return false;
			}
			
		}
		else
		{
			auto [baseAddress, port] = parser.getBaseAddress(destination.value());
			(void)port; //shut up gcc
			if(baseAddress.has_value())
			{
				auto [status, new_sock] = connect(baseAddress.value());
				if(status == ERROR_STATUS)
				{
					LoggerLogStatusErrorWithLineAndFile("NOT-CONNECT connection failed\n", status);
					return false;
				}

				m_socketsToAdd.push_back( {new_sock, POLLOUT} );
				m_proxyManager.addEstablishedConnection(receiving_socket, new_sock);
				m_proxyManager.addDataForDescriptor(new_sock, std::move(message));
				return false;
			}
		}
	}
	else
	{
		//not implemented
		//m_proxyManager.addDataForDescriptor(receiving_socket, /*wygenerowana wiadomosc http*/);
		return true;
	}
	
	return false;
}

