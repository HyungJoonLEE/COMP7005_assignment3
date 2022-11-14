#include "conversion.h"
#include "server.h"
#include "error.h"
#include "common.h"

#define FAIL    -1

int main(int argc, char *argv[]) {
    struct options opts;
    struct sockaddr_in client_address;
    int client_socket;
    char client_ip[16] = {0};
    int client_address_size = sizeof(struct sockaddr_in);
    SSL_CTX *ctx;

    options_init_server(&opts);
    parse_arguments_server(argc, argv, &opts);
    options_process_server(&opts);

    SSL_library_init();
    ctx = InitServerCTX();
    LoadCertificates(ctx,  opts.public_key,  opts.private_key);

    while (1) {
        SSL *ssl;
        client_socket = accept(opts.server_socket, (struct sockaddr*)&client_address, &client_address_size);  /* accept connection as usual */
        printf("Connection: ip = [ %s ] : port = %d\n",inet_ntoa(client_address.sin_addr), ntohs(client_address.sin_port));
        strcpy(client_ip, inet_ntoa(client_address.sin_addr));
        ssl = SSL_new(ctx);              /* get new SSL state with context */
        SSL_set_fd(ssl, client_socket);      /* set connection socket to SSL state */
        Servlet(ssl, client_ip);
    }
    cleanup(&opts);
    SSL_CTX_free(ctx);
    return EXIT_SUCCESS;
}

static void options_init_server(struct options *opts) {
    memset(opts, 0, sizeof(struct options));
    opts->port_in = DEFAULT_PORT;
    strcpy(opts->public_key, "cert.pem");
    strcpy(opts->private_key, "key.pem");
}


static void parse_arguments_server(int argc, char *argv[], struct options *opts)
{
    int c;

    while((c = getopt(argc, argv, ":p:c:k:")) != -1)   // NOLINT(concurrency-mt-unsafe)
    {
        switch(c)
        {
            case 'p':
            {
                opts->port_in = parse_port(optarg, 10); // NOLINT(cppcoreguidelines-avoid-magic-numbers,readability-magic-numbers)
                break;
            }
            case 'c':
            {
                memset(opts->public_key, 0, sizeof(char) * 20);
                strcpy(opts->public_key, optarg); // NOLINT(cppcoreguidelines-avoid-magic-numbers,readability-magic-numbers)
                break;
            }
            case 'k':
            {
                memset(opts->private_key, 0, sizeof(char) * 20);
                strcpy(opts->private_key, optarg); // NOLINT(cppcoreguidelines-avoid-magic-numbers,readability-magic-numbers)                printf("this is public = %s\n", opts->public_key);
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


void Servlet(SSL* ssl, char* client_ip) { /* Serve the connection -- threadable */
    char buffer[256] = {0};
    int sd, received_bytes;

    if ( SSL_accept(ssl) == FAIL )     /* do SSL-protocol accept */
        ERR_print_errors_fp(stderr);
    else {
        while(1) {
            received_bytes = SSL_read(ssl, buffer, sizeof(buffer)); /*   */
            if (received_bytes > 0) {
                buffer[received_bytes] = 0;
                printf("%s: %s\n", client_ip, buffer);

                for (int i = 0; i < (int) strlen(buffer); i++) {
                    buffer[i] = (char) toupper(buffer[i]);
                }
                SSL_write(ssl, buffer, sizeof(buffer)); /* send reply */
                memset(buffer, 0, sizeof(char) * 256);
            }
            else {
                ERR_print_errors_fp(stderr);
                break;
            }
        }
    }
    sd = SSL_get_fd(ssl);       /* get socket connection */
    SSL_free(ssl);         /* release SSL state */
    close(sd);          /* close connection */
}
