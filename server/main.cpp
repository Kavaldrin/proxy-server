#include "server.h"
#include <iostream>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <memory>

using namespace std;

constexpr int PORT = 8081;

int main() {
	Server serv(AF_INET, PORT, "127.0.0.1");

	serv.createSocket(AF_INET, SOCK_STREAM, 0);
	serv.bindSocket(sizeof(sockaddr));
	serv.listen();

	return 0;
}
