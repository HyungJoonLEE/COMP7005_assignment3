#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <netinet/in.h>
#include <sys/time.h>
#include <unistd.h>
#include <arpa/inet.h>
#include "client.h"
#include "conversion.h"
#include "error.h"




int main(int argc, char *argv[]) {
    int max_socket_num; // IMPORTANT Don't forget to set +1
    char buffer[255] = {0};
    fd_set read_fds;


    struct options opts;
    options_init(&opts);
    parse_arguments(argc, argv, &opts);
    opts.server_socket = options_process(&opts);
    if (opts.server_socket == -1) {
        printf("Connect() fail");
    }

    max_socket_num = opts.server_socket + 1;

    FD_ZERO(&read_fds);
    while (1) {
        FD_SET(0, &read_fds);
        FD_SET(opts.server_socket, &read_fds);
        if (select(max_socket_num, &read_fds, NULL, NULL, NULL) < 0) {
            printf("select fail");
            exit(1);
        }

        if (FD_ISSET(opts.server_socket, &read_fds)) {
            ssize_t received_data_size;
            if ((received_data_size = read(opts.server_socket, buffer, sizeof(buffer))) > 0) {
                buffer[received_data_size] = '\0';
                printf("%s", buffer);
            }
        }

        if (FD_ISSET(0, &read_fds)) {
            if (fgets(buffer, sizeof(buffer), stdin)) {
                if (write(opts.server_socket, buffer, sizeof(buffer)) < 0)
                    printf("Nothing to write()\n");
            }
        }
    }
    cleanup(&opts);
    return EXIT_SUCCESS;
}

static void options_init(struct options *opts)
{
    memset(opts, 0, sizeof(struct options));
    opts->fd_in    = STDIN_FILENO;
    opts->fd_out   = STDOUT_FILENO;
    opts->port_out = DEFAULT_PORT;
}


static void parse_arguments(int argc, char *argv[], struct options *opts)
{
    int c;

    while((c = getopt(argc, argv, ":s:p:")) != -1)   // NOLINT(concurrency-mt-unsafe)
    {
        switch(c)
        {
            case 's':
            {
                opts->ip_out = optarg;
                break;
            }
            case 'p':
            {
                opts->port_out = parse_port(optarg,
                                            10); // NOLINT(cppcoreguidelines-avoid-magic-numbers,readability-magic-numbers)
                break;
            }
            case ':':
            {
                fatal_message(__FILE__, __func__, __LINE__, "\"Option requires an operand\"", 5); // NOLINT(cppcoreguidelines-avoid-magic-numbers,readability-magic-numbers)
                break;
            }
            case '?':
            {
                fatal_message(__FILE__, __func__, __LINE__, "Unknown",
                              6); // NOLINT(cppcoreguidelines-avoid-magic-numbers,readability-magic-numbers)
            }
            default:
            {
                assert("should not get here");
            };
        }
    }
}


static int options_process(struct options *opts) {
    ssize_t server_connection_test_fd;
    int result;
    char message[50] = {0};
    if(opts->file_name)
    {
        opts->fd_in = open(opts->file_name, O_RDONLY);

        if(opts->fd_in == -1)
        {
            fatal_errno(__FILE__, __func__ , __LINE__, errno, 2);
        }
    }


    if(opts->ip_out)
    {
        struct sockaddr_in server_addr;

        opts->server_socket = socket(AF_INET, SOCK_STREAM, 0);

        if(opts->server_socket == -1)
        {
            fatal_errno(__FILE__, __func__ , __LINE__, errno, 2);
        }

        server_addr.sin_family = AF_INET;
        server_addr.sin_port = htons(opts->port_out);
        server_addr.sin_addr.s_addr = inet_addr(opts->ip_out);

        if(server_addr.sin_addr.s_addr ==  (in_addr_t)-1)
        {
            fatal_errno(__FILE__, __func__ , __LINE__, errno, 2);
        }

        result = connect(opts->server_socket, (struct sockaddr *)&server_addr, sizeof(struct sockaddr_in));

        if(result == -1)
        {
            fatal_errno(__FILE__, __func__ , __LINE__, errno, 2);
        }
        server_connection_test_fd = read(opts->server_socket, message, sizeof(message));
        if(server_connection_test_fd == -1) {
            printf("You are not connected to server\n");
        }
        printf("[ SERVER ]: %s \n", message);
    }
    return opts->server_socket;
}


static void cleanup(const struct options *opts)
{
    if(opts->ip_out)
    {
        close(opts->fd_out);
    }
    for (int i = 0; i < opts->file_count; i++) {
        free(opts->file_arr[i]);
    }
}

