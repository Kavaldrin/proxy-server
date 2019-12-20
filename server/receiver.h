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

	std::pair < std::vector<char>, bool > recv(socket_t receiving_socket);

private:
	void print_rcvd_msg(std::vector<std::pair<socket_t, sockaddr_in>>::iterator sock_data_elem, const std::string& msg) const;


};