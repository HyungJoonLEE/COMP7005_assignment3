#include "common.h"
#include "client.h"
#include "conversion.h"
#include "error.h"


#define FAIL -1


int main(int argc, char *argv[]) {
    char buffer[256] = {0};
    int received_bytes;
    SSL_CTX *ctx;
    SSL *ssl;
    struct options opts;


    SSL_library_init();
    ctx = InitCTX();
    options_init(&opts);
    parse_arguments(argc, argv, &opts);
    opts.server_socket = options_process(&opts);
    if (opts.server_socket == -1) {
        printf("Connect() fail");
    }


    ssl = SSL_new(ctx);      /* create new SSL connection state */
    SSL_set_fd(ssl, opts.server_socket);    /* attach the socket descriptor */
    if (SSL_connect(ssl) == FAIL )   /* perform the connection */
        ERR_print_errors_fp(stderr);
    else {
        printf("Connected with %s encryption\n", SSL_get_cipher(ssl));
        ShowCerts(ssl);        /* get any certs */
        if (fgets(buffer, sizeof(buffer), stdin)) {
            if (SSL_write(ssl, buffer, sizeof(buffer)) < 0) {
                printf("Nothing to write()\n");
            }
        }
        memset(buffer, 0, sizeof(char) * 256);
        received_bytes = SSL_read(ssl, buffer, sizeof(buffer)); /* get reply & decrypt */
        buffer[received_bytes] = 0;
        printf("Server: %s\n", buffer);
        SSL_free(ssl);        /* release connection state */
    }
    close(opts.server_socket);         /* close socket */
    SSL_CTX_free(ctx);        /* release context */
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
    int result;

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
    }
    return opts->server_socket;
}


static void cleanup(const struct options *opts)
{
    if(opts->ip_out)
    {
        close(opts->fd_out);
    }
}


SSL_CTX* InitCTX(void)
{   SSL_METHOD *method;
    SSL_CTX *ctx;

    OpenSSL_add_all_algorithms();  /* Load cryptos, et.al. */
    SSL_load_error_strings();   /* Bring in and register error messages */
    method = TLSv1_2_client_method();  /* Create new client-method instance */
    ctx = SSL_CTX_new(method);   /* Create new context */
    if ( ctx == NULL )
    {
        ERR_print_errors_fp(stderr);
        abort();
    }
    return ctx;
}

void ShowCerts(SSL* ssl)
{   X509 *cert;
    char *line;

    cert = SSL_get_peer_certificate(ssl); /* get the server's certificate */
    if ( cert != NULL )
    {
        printf("Server certificates:\n");
        line = X509_NAME_oneline(X509_get_subject_name(cert), 0, 0);
        printf("Subject: %s\n", line);
        free(line);       /* free the malloc'ed string */
        line = X509_NAME_oneline(X509_get_issuer_name(cert), 0, 0);
        printf("Issuer: %s\n", line);
        free(line);       /* free the malloc'ed string */
        X509_free(cert);     /* free the malloc'ed certificate copy */
    }
    else
        printf("Info: No client certificates configured.\n");
}