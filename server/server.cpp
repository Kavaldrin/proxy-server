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
constexpr bool ENCRYPTED = true;

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
		std::cout << " pool " << std::endl;
		int poll_result = poll(pollfd_list.data(), pollfd_list.size(), -1 /*no timeout */);

		if (poll_result == ERROR_STATUS) {
			logger.logStatusError("poll", poll_result);
			return;
		}
		

		for(auto& pollfd_element : pollfd_list) {

			//std::this_thread::sleep_for(std::chrono::milliseconds(100));
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
			std::cout <<"cos bylo?\n" << std::endl;
			pollfd_list.erase(std::remove_if(pollfd_list.begin(), pollfd_list.end(), [soc](const auto& s) { return soc == s.fd; } ));
		}
		m_socketsToRemove.clear();

		for(const auto& soc : m_socketsToAdd)
		{
			pollfd_list.push_back(soc);
		}
		m_socketsToAdd.clear();

		receiver.closeSavedSockets();
	}
}

void Server::accept() {
	sockaddr_in sender_addr_in;
	socklen_t sender_addr_len = sizeof(sockaddr_in);

	auto new_sock = ::accept(sock_receiving, reinterpret_cast<sockaddr*>(&sender_addr_in), &sender_addr_len);

	if(new_sock != ERROR_STATUS) {
		std::cout << "accept status = " << new_sock;
		//pollfd_list.push_back(pollfd{new_sock, POLLIN});
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
		// std::cout << hostaddr->h_name << std::endl;
		// std::cout << hostaddr->h_addr_list[0] << std::endl;

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
	
		int status = ::connect(serv_sock, (sockaddr*) &dest, sizeof(sockaddr));
		while(status == ERROR_STATUS)
		{
			if(errno != EINPROGRESS)
			{
				break;
			}
			status = ::connect(serv_sock, (sockaddr*) &dest, sizeof(sockaddr));
		}
		

		//LoggerLogStatusErrorWithLineAndFile("returning almost ocnnected socket", status);
		return {0 , serv_sock};
		
	}
	else {
		std::cout << "NULL\n";
		std::cout << strerror(h_errno) << std::endl;
	}

	return {ERROR_STATUS, 0};
}

void Server::recvAndSend(int receiving_socket) {
	

	//recv(receiving_socket);
}

bool Server::send(int socket) noexcept
{
	//std::this_thread::sleep_for(std::chrono::seconds(1));
	// std::cout << "send " << std::endl;
	auto [shouldChangeSocketState, shouldRemoveSocket] =  m_proxyManager.handleStoredBuffers(socket, receiver);
	if(shouldRemoveSocket)
	{
		m_socketsToRemove.push_back(socket);
	}
	return shouldChangeSocketState;
}

bool Server::recv(int receiving_socket) noexcept {
	
			//std::this_thread::sleep_for(std::chrono::seconds(1));

	auto [message, isSocketClosed] = receiver.recv(receiving_socket);	
	
	std::cout << message.size() << std::endl;


	if(message.empty())
	{
		std::cout << "empty\n";
		return true;
	}

	std::cout << "After recev" << std::endl;

	//if possible
	if(m_proxyManager.isDestination(receiving_socket) && message.empty() && isSocketClosed)
	{
		std::cout << " destroying\n";
		m_proxyManager.destroyEstablishedConnectionByDestination(receiving_socket);
		return false;
	}


	if(auto pairSocket = m_proxyManager.getSecondSocketIfEstablishedConnection(receiving_socket);
		pairSocket.has_value())
	{
		bool isMsgFromDest = m_proxyManager.isDestination(receiving_socket);

		auto endBodyParams = m_proxyManager.getEndBodyParams(receiving_socket, *pairSocket);

		std::cout << "already connected: " << receiving_socket << " " << *pairSocket << std::endl;
		LoggerLogStatusWithLineAndFile("is rcvd msg from dest", isMsgFromDest);
		if(endBodyParams) {
			LoggerLogStatusWithLineAndFile("is encrypted", (*endBodyParams)->second.isEncrypted);
			LoggerLogStatusWithLineAndFile("msgs from brow", (*endBodyParams)->second.numMessagesFromWebBrowser);
			LoggerLogStatusWithLineAndFile("msgs from serv", (*endBodyParams)->second.numMessagesFromServer);
		}

		if(not (*endBodyParams)->second.isEncrypted) {
			auto bodySize = addEndBodyMethodIfItsFirstMsgFromDest(receiving_socket,
																  *pairSocket,
																  isMsgFromDest,
																  endBodyParams,
																  message);

			setShouldCloseSocketsBcsInMsgFromDestIsEndBody(isMsgFromDest,
														   bodySize,
														   endBodyParams,
												  		   message);
		}

		std::cout << receiving_socket << " : " << pairSocket.value() << std::endl;
		m_proxyManager.addDataForDescriptor(pairSocket.value(), std::move(message));


		if(isMsgFromDest)
			m_proxyManager.incrementMessagesFromServer(receiving_socket, *pairSocket);
		else
			m_proxyManager.incrementMessagesFromWebBrowser(receiving_socket, *pairSocket);

		return true;
	}

	ParserHttp parser{ std::string_view(message.data(), message.size()) };
	if(parser.isHTTPRequest())
	{
		LoggerLogStatusWithLineAndFile("jest http", 1);
		auto[method, destination] = parser.parseStartLine();
		if(!method.has_value())
		{
			return true;
		}


		if(parser.parseMethod() == std::string("CONNECT"))
		{
			LoggerLogStatusWithLineAndFile("CONNECT METHOD", 1);
			auto [baseAddress, port] = parser.getBaseAddress(destination.value());
			if(baseAddress.has_value())
			{
				auto [status, new_sock] = connect(baseAddress.value(), port);
				if(status == ERROR_STATUS)
				{
					LoggerLogStatusErrorWithLineAndFile("CONNECT connection failed\n", status);
					return true;
				}

				//pollfd_list.push_back(pollfd{new_sock, POLLOUT});
				m_socketsToAdd.push_back( {new_sock, POLLOUT} );
				m_proxyManager.addEstablishedConnection(receiving_socket, new_sock);
				//m_proxyManager.addDataForDescriptor(new_sock, std::move(message));

				std::string okResponse = { "HTTP/1.1 200 Connection Established\r\n\r\n" };
				std::cout << okResponse << std::endl;
				m_proxyManager.addDataForDescriptor(receiving_socket, std::vector<char>(okResponse.begin(), okResponse.end()));
				m_proxyManager.createEndBodyParams(receiving_socket, new_sock, ENCRYPTED);
				return true;
			}		
		}
		else
		{
			LoggerLogStatusWithLineAndFile("NON CONNECT METHOD", 1);
			auto [baseAddress, port] = parser.getBaseAddress(destination.value());
			(void)port; 
			if(baseAddress.has_value())
			{
				std::cout << "base address: " << baseAddress.value() << " :"<< std::endl;
				auto [status, new_sock] = connect(baseAddress.value());
				if(status == ERROR_STATUS)
				{
					LoggerLogStatusErrorWithLineAndFile("NOT-CONNECT connection failed\n", status);
					return true;
				}

				//pollfd_list.push_back(pollfd{new_sock, POLLOUT});
				m_socketsToAdd.push_back( {new_sock, POLLOUT} );
				m_proxyManager.addEstablishedConnection(receiving_socket, new_sock);
				m_proxyManager.addDataForDescriptor(new_sock, std::move(message));
				m_proxyManager.createEndBodyParams(receiving_socket, new_sock, not ENCRYPTED);
				m_proxyManager.incrementMessagesFromWebBrowser(receiving_socket, new_sock);

				return true;
			}
		}
	}
	else
	{
		std::cout << "niehttp" << std::endl;
		//not implemented
		//m_proxyManager.addDataForDescriptor(receiving_socket, /*wygenerowana wiadomosc http*/);
		return true;
	}
	
	return true;
}

std::optional<int> Server::addEndBodyMethodIfItsFirstMsgFromDest(
		int receiving_socket,
		int pairSocket,
		bool isMsgFromDest,
		std::optional<std::_Rb_tree_iterator<std::pair<const std::pair<int, int>, Proxy::ProxyManager::EndBodyParameters>>> endBodyParams,
		const std::vector<char>& message) {
	std::optional<int> bodySize;

	if(isMsgFromDest) {
		LoggerLogStatusWithLineAndFile("has end body value? ", endBodyParams.has_value());

		if(endBodyParams.has_value()) {
			std::cout << "end body params for pair sockets " 
			<< (*endBodyParams)->first.first << " " << (*endBodyParams)->first.second << std::endl;

			if((*endBodyParams)->second.numMessagesFromServer == 0){
				ParserHttp parser{ std::string_view(message.data(), message.size()) };
				auto headers = parser.parse();

				auto body_it = headers->find("Body-length");
				if(body_it != headers->end())
					bodySize = std::stoi(body_it->second);

				m_proxyManager.addEndBodyMethod(receiving_socket, pairSocket, *headers);
			}
		}
	}

	return bodySize;
}

void Server::setShouldCloseSocketsBcsInMsgFromDestIsEndBody(
		bool isMsgFromDest,
		std::optional<int> bodySize,
		std::optional<std::_Rb_tree_iterator<std::pair<const std::pair<int, int>, Proxy::ProxyManager::EndBodyParameters>>> endBodyParams,
		const std::vector<char>& message) {

	// bool should_close = false;
	if(isMsgFromDest) {
		LoggerLogStatusWithLineAndFile("end body", static_cast<int>((*endBodyParams)->second.endBodyMethod));
		if((*endBodyParams)->second.endBodyMethod == Proxy::EndBodyMethod::CHUNK) {
			std::cout << "its chunked\n";
			if(message.size() >= 5) {
				std::string possible_chunk_end{message.end()-5, message.end()};
				const std::string chunk_end{"0\r\n\r\n"};
				(*endBodyParams)->second.shouldBeClosed = chunk_end == possible_chunk_end;
				if((*endBodyParams)->second.shouldBeClosed)
					std::cout << "chunk end should be closed\n";
			}
		}
		else if((*endBodyParams)->second.endBodyMethod == Proxy::EndBodyMethod::CONTENT_LENGTH) {

			if(not bodySize)
				bodySize = message.size();
			*(*endBodyParams)->second.contentLength -= *bodySize;

			if(*(*endBodyParams)->second.contentLength <= 0) {
				std::cout << "content-length should be closed\n";
				(*endBodyParams)->second.shouldBeClosed = true;
			}
		}
		else {
			std::cout << "NONE close method should be closed\n";
			(*endBodyParams)->second.shouldBeClosed = true;
		}
	}

	// return should_close;
}

void Server::closeSocketsAndCleanStructures(int receiving_socket, int pairSocket) {
	receiver.saveSocketToClose(receiving_socket);
	receiver.saveSocketToClose(pairSocket);

	if(m_proxyManager.isDestination(receiving_socket))
		m_proxyManager.destroyEstablishedConnectionByDestination(receiving_socket);
	else
		m_proxyManager.destroyEstablishedConnectionBySource(receiving_socket);
}



