#ifndef COMP_7005_PROJECT_SERVER_H
#define COMP_7005_PROJECT_SERVER_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <fcntl.h>
#include <ctype.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <sys/file.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <openssl/x509.h>
#include <openssl/pem.h>
#include <openssl/ssl.h>
#include <openssl/err.h>
#include <openssl/evp.h>
#include <assert.h>

#define DEFAULT_PORT 4000

struct options
{
    in_port_t port_in;
    int server_socket;
    char public_key[40];
    char private_key[40];
};



/**
 * Initialize default option for client.
 * Memory setting for server option struct to 0 and DEFAULT port number saved.
 *
 * @param opts client struct settings
 */
static void options_init_server(struct options *opts);


/**
 * Parse input from server IP, server port, server download directory are included.
 * Set the server option struct after parsing.
 * If there is no input, it will use DEFAULT value.
 *
 * @param argc number of argument
 * @param argv server's input
 * @param opts server option struct settings
 */
static void parse_arguments_server(int argc, char *argv[], struct options *opts);


/**
 * Initialize network settings in server from protocol, ip, port and wait for client connection.
 *
 * @param opts client option struct settings
 */
static void options_process_server(struct options *opts);


/**
 * Free variables that were used for client option struct setting.
 *
 * @param opts
 */
static void cleanup(const struct options *opts);
SSL_CTX* InitServerCTX(void);
void LoadCertificates(SSL_CTX* ctx, char* CertFile, char* KeyFile);
void ShowCerts(SSL* ssl);
void Servlet(SSL* ssl, char* client_ip);
#endif //COMP_7005_PROJECT_SERVER_H
