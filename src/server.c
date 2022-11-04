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
#include "conversion.h"
#include "server.h"
#include "error.h"
#include "common.h"


const char *CONNECTION_SUCCESS = "Successfully connected to the server\n"; // when client connected server send this

int main(int argc, char *argv[]) {
    struct options opts;
    struct sockaddr_in client_address;
    int client_socket;
    int max_socket_num; // IMPORTANT Don't forget to set +1
    char buffer[256] = {0};
    char cpy_buffer[256] = {0};
    int client_address_size = sizeof(struct sockaddr_in);
    ssize_t received_data;
    fd_set read_fds; // fd_set chasing reading status


    options_init_server(&opts);
    parse_arguments_server(argc, argv, &opts);
    options_process_server(&opts);

    while (1) {
        FD_ZERO(&read_fds);
        FD_SET(opts.server_socket, &read_fds);
        for (int i = 0; i < opts.client_count; i++) {
            FD_SET(opts.client_socket[i], &read_fds);
        }
        max_socket_num = get_max_socket_number(&opts) + 1;
        printf("wait for client\n");
        if (select(max_socket_num, &read_fds, NULL, NULL, NULL) < 0) {
            printf("select() error");
            exit(1);
        }

        if (FD_ISSET(opts.server_socket, &read_fds)) {
            client_socket = accept(opts.server_socket, (struct sockaddr *)&client_address, &client_address_size);
            if (client_socket == -1) {
                printf("accept() error");
                exit(1);
            }

            add_new_client(&opts, client_socket, &client_address);
            write(client_socket, CONNECTION_SUCCESS, strlen(CONNECTION_SUCCESS));
            printf("Successfully added client_fd to client_socket[%d]\n", opts.client_count - 1);
        }

        // Receive packet from client A
        if (FD_ISSET(opts.client_socket[0], &read_fds)) {
            received_data = read(opts.client_socket[0], buffer, sizeof(buffer));
            if (received_data <= 0) {
                remove_client(&opts, 0);
                continue;
            }
            buffer[received_data] = '\0';

            strcpy(cpy_buffer, buffer);
            for (int i = 0; i < strlen(cpy_buffer); i++) {
                cpy_buffer[i] = (char) toupper(buffer[i]);
            }

            printf("cpy_buffer = %s\n", cpy_buffer);
            write(opts.client_socket[0], cpy_buffer, sizeof(cpy_buffer));
            memset(cpy_buffer, 0, sizeof(char) * 256);
            printf("%s", buffer);
        }
    }
    cleanup(&opts);
    return EXIT_SUCCESS;
}

static void options_init_server(struct options *opts) {
    memset(opts, 0, sizeof(struct options));
    opts->port_in = DEFAULT_PORT;
}


static void parse_arguments_server(int argc, char *argv[], struct options *opts)
{
    int c;

    while((c = getopt(argc, argv, ":p:")) != -1)   // NOLINT(concurrency-mt-unsafe)
    {
        switch(c)
        {
            case 'p':
            {
                opts->port_in = parse_port(optarg, 10); // NOLINT(cppcoreguidelines-avoid-magic-numbers,readability-magic-numbers)
                break;
            }
            case ':':
            {
                fatal_message(__FILE__, __func__ , __LINE__, "\"Option requires an operand\"", 5); // NOLINT(cppcoreguidelines-avoid-magic-numbers,readability-magic-numbers)
                break;
            }
            case '?':
            {
                fatal_message(__FILE__, __func__ , __LINE__, "Unknown", 6); // NOLINT(cppcoreguidelines-avoid-magic-numbers,readability-magic-numbers)
            }
            default:
            {
                assert("should not get here");
            };
        }
    }
}


static void options_process_server(struct options *opts) {
    struct sockaddr_in server_address;
    int option = TRUE;


    for (int i = 0; i < 2; i++) {
        opts->client_socket[i] = 0;
    }


    opts->server_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (opts->server_socket == -1) {
        printf("socket() ERROR\n");
        exit(1);
    }

    server_address.sin_family = AF_INET;
    server_address.sin_port = htons(opts->port_in);
    server_address.sin_addr.s_addr = htonl(INADDR_ANY);

    if (server_address.sin_addr.s_addr == (in_addr_t) -1) {
        fatal_errno(__FILE__, __func__, __LINE__, errno, 2);
    }

    option = 1;
    setsockopt(opts->server_socket, SOL_SOCKET, SO_REUSEADDR, &option, sizeof(option));


    if (bind(opts->server_socket, (struct sockaddr *) &server_address, sizeof(struct sockaddr_in)) == -1) {
        printf("bind() ERROR\n");
        exit(1);
    }


    if (listen(opts->server_socket, 1) == -1) {
        printf("listen() ERROR\n");
        exit(1);
    }
}


void add_new_client(struct options *opts, int client_socket, struct sockaddr_in *new_client_address) {
    char buffer[20];

    inet_ntop(AF_INET, &new_client_address->sin_addr, buffer, sizeof(buffer));
    printf("New client: [ %s ]\n", buffer);

    opts->client_socket[opts->client_count] = client_socket;
    opts->client_count++;
}


void remove_client(struct options *opts, int client_socket) {
    close(opts->client_socket[client_socket]);

    if (client_socket != opts->client_count - 1)
        opts->client_socket[client_socket] = opts->client_socket[opts->client_count - 1];

    opts->client_count--;
    printf("Current client count = %d\n", opts->client_count);
}

// Finding maximum socket number
int get_max_socket_number(struct options *opts) {
    // Minimum socket number start with server socket(opts->server_socket)
    int max = opts->server_socket;
    int i;

    for (i = 0; i < opts->client_count; i++)
        if (opts->client_socket[i] > max)
            max = opts->client_socket[i];

    return max;
}


static void cleanup(const struct options *opts) {
    close(opts->server_socket);
}
