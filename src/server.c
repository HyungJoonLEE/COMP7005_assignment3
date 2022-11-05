#include "conversion.h"
#include "server.h"
#include "error.h"
#include "common.h"

char client_ip[16];

int main(int argc, char *argv[]) {
    struct options opts;
    struct sockaddr_in client_address;
    int client_socket;
    int max_socket_num; // IMPORTANT Don't forget to set +1
    char buffer[256] = {0};
    char client_msg[256] = {0};
    int client_address_size = sizeof(struct sockaddr_in);
    ssize_t received_data;
    fd_set read_fds; // fd_set chasing reading status
    SSL_CTX *ctx;
    SSL *ssl;



    SSL_library_init();
    ctx = InitServerCTX();
    LoadCertificates(ctx, PUBLIC_KEY, PRIVATE_KEY);
    ShowCerts(ssl);
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
            received_data = read(opts.client_socket[0], client_msg, sizeof(client_msg));
            if (received_data <= 0) {
                remove_client(&opts, 0);
                continue;
            }
            client_msg[received_data] = '\0';
            sprintf(buffer, "[ %s ]: ", client_ip);
            strcat(buffer, client_msg);
            printf("%s", buffer);

            for (int i = 0; i < strlen(client_msg); i++) {
                client_msg[i] = (char) toupper(client_msg[i]);
            }


            write(opts.client_socket[0], client_msg, sizeof(client_msg));
            memset(client_msg, 0, sizeof(char) * 256);
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
    strcpy(client_ip, buffer);
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
    int max_socket_num = opts->server_socket;
    for (int i = 0; i < opts->client_count; i++)
        if (opts->client_socket[i] > max_socket_num)
            max_socket_num = opts->client_socket[i];

    return max_socket_num;
}


static void cleanup(const struct options *opts) {
    close(opts->server_socket);
}


SSL_CTX* InitServerCTX() {
    SSL_METHOD *method;
    SSL_CTX *ctx;

    OpenSSL_add_all_algorithms();  /* load & register all cryptos, etc. */
    SSL_load_error_strings();   /* load all error messages */
    method = TLSv1_2_server_method();  /* create new server-method instance */
    ctx = SSL_CTX_new(method);   /* create new context from method */
    if ( ctx == NULL )
    {
        ERR_print_errors_fp(stderr);
        abort();
    }
    return ctx;
}


void LoadCertificates(SSL_CTX* ctx, char* public_key, char* private_key) {
    /* set the local certificate from public_key */
    if ( SSL_CTX_use_certificate_file(ctx, public_key, SSL_FILETYPE_PEM) <= 0 )
    {
        ERR_print_errors_fp(stderr);
        abort();
    }
    /* set the private key (maybe the same as public_key) */
    if ( SSL_CTX_use_PrivateKey_file(ctx, private_key, SSL_FILETYPE_PEM) <= 0 )
    {
        ERR_print_errors_fp(stderr);
        abort();
    }
    /* verify private key */
    if ( !SSL_CTX_check_private_key(ctx) )
    {
        fprintf(stderr, "Private key does not match the public certificate\n");
        abort();
    }
}


void ShowCerts(SSL* ssl) {
    X509 *cert;
    char *line;

    cert = SSL_get_peer_certificate(ssl); /* Get certificates (if available) */
    if ( cert != NULL )
    {
        printf("Server certificates:\n");
        line = X509_NAME_oneline(X509_get_subject_name(cert), 0, 0);
        printf("Subject: %s\n", line);
        free(line);
        line = X509_NAME_oneline(X509_get_issuer_name(cert), 0, 0);
        printf("Issuer: %s\n", line);
        free(line);
        X509_free(cert);
    }
    else
        printf("No certificates.\n");
}