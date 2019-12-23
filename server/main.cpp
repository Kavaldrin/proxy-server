#include "server.h"
#include <iostream>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <memory>
#include <csignal>

using namespace std;

constexpr int PORT = 8080;

int main(int argc, char const *argv[]) {
	std::string account;
	if(argc >= 2)
		account = std::string{argv[1]};
	else account = std::string{"11111111111111111111111160"};
	signal(SIGPIPE, SIG_IGN);

	Server serv(AF_INET, PORT, "127.0.0.1", account);

	serv.createSocket(AF_INET, SOCK_STREAM, 0);
	serv.bindSocket(sizeof(sockaddr));
	serv.listen();

	return 0;
}
