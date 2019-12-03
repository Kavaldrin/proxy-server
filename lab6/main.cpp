#include "server.h"
#include <iostream>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <memory>

#include <unistd.h>
#include <openssl/ssl.h>
#include <openssl/err.h>

using namespace std;

constexpr int PORT = 65280;

void init_openssl()
{ 
    SSL_load_error_strings();	
    OpenSSL_add_ssl_algorithms();
}

void cleanup_openssl()
{
    EVP_cleanup();
}

CTX_unique create_context()
{
    const SSL_METHOD *method;
    method = SSLv23_server_method();

    CTX_unique ctx{SSL_CTX_new(method), SSL_CTX_free};
    if (!ctx) {
		perror("Unable to create SSL context");
		ERR_print_errors_fp(stderr);
		exit(EXIT_FAILURE);
    }

    return ctx;
}

void configure_context(CTX_unique& ctx)
{
    SSL_CTX_set_ecdh_auto(ctx, 1);

    /* Set the key and cert */
    if (SSL_CTX_use_certificate_file(ctx.get(), "server.crt", SSL_FILETYPE_PEM) <= 0) {
        ERR_print_errors_fp(stderr);
		exit(EXIT_FAILURE);
    }

    if (SSL_CTX_use_PrivateKey_file(ctx.get(), "server.key", SSL_FILETYPE_PEM) <= 0 ) {
        ERR_print_errors_fp(stderr);
		exit(EXIT_FAILURE);
    }
}

int main() {
	init_openssl();
	auto ctx = create_context();
	configure_context(ctx);

	Server serv(AF_INET, PORT, "127.0.0.1", ctx);

	serv.createSocket(AF_INET, SOCK_STREAM, 0);
	serv.bindSocket(sizeof(sockaddr));
	serv.listen();

	cleanup_openssl();

	return 0;
}
