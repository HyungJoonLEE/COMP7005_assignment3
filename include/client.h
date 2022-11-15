#ifndef ASSIGNMENT1_CLIENT_H
#define ASSIGNMENT1_CLIENT_H

#include <arpa/inet.h>
#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <dirent.h>
#include <openssl/x509.h>
#include <openssl/pem.h>
#include <openssl/ssl.h>
#include <openssl/err.h>
#include <openssl/evp.h>


struct options
{
    char *ip_out;
    in_port_t port_out;
    int fd_in;
    int fd_out;
    int server_socket;
    char file[40];
};



/**
 * Initialize default option for client
 * STDIN, STDOUT and port number saved
 *
 * @param opts client struct settings
 */
static void options_init(struct options *opts);


/**
 * Parse input from user server IP, server port, client text files are included.
 * Set the client option struct after parsing.
 * If there is no input, it will use DEFAULT value.
 *
 * @param argc number of argument
 * @param argv client's input
 * @param opts client option struct settings
 */
static void parse_arguments(int argc, char *argv[], struct options *opts);


/**
 * Initialize connection with server.
 * After successful connection, client will get message from server to confirm it is connected.
 *
 * @param opts client option struct settings
 */
static int options_process(struct options *opts);


/**
 * Free variables that were used for client option struct setting.
 *
 * @param opts client option struct settings
 */
static void cleanup(const struct options *opts);


/**
 * OpenSSL keeps an internal table of digest algorithms and ciphers. It uses this table to lookup ciphers via functions
 *
 * @return SSL_CTX
 */
SSL_CTX* InitCTX(void);


/**
 * Get certificates (if available) other than that print no certificates
 *
 * @param ssl SSL struct
 */
void ShowCerts(SSL* ssl);


/**
 * Send a file using SSL_write and return file size
 *
 * @param opts client struct
 * @param ssl SSL struct
 * @return size of file
 */
int send_file(struct options *opts, SSL* ssl);



#endif //ASSIGNMENT1_CLIENT_H
