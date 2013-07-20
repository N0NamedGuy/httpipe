#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>

#include <getopt.h>

#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>

#define MAX_FILENAME 256
#define MAX_MIME 256

#define DEF_BUF_SIZE 1024
#define DEF_PORT 5000
#define DEF_VERBOSE 0
#define DEF_MIME "application/octet-stream"

#define printverb(fmt, ...) \
    do { if (g_verbose) fprintf(stderr, fmt, __VA_ARGS__); } while (0)

int g_buf_size;
int g_port;
char g_filename[MAX_FILENAME];
int g_verbose;
char g_mime[MAX_MIME];

void print_help(int argc, char** argv) {
    printf("Usage: %s [OPTIONS]\n", argv[0]);
    printf("Outputs given input to HTTP\n\n");
    printf("Options:\n");
    printf("  -f, --file=FILE\toutputs file to HTTP. If FILE is undefined,\n");
    printf("\t\t\tinput data will be read from stdin\n");
    printf("  -p, --port=PORT\tsets the port to which the program will listen\n");
    printf("\t\t\tfor incoming connections. Defaults to %d\n", DEF_PORT);
    printf("  -m, --mime=MIME\tsets the output MIME type. Defaults to %s\n", DEF_MIME);
    printf("  -v, --verbose\t\tbe verbose\n");
    printf("  -h, --help\t\tshow this help message\n\n");
    printf("This program is meant to be used as a pipe that goes through HTTP.\n");
    printf("It was conceived so it could be possible to easily transfer a disk\n");
    printf("image throught HTTP. That can be accomplished by issuing:\n\n");
    printf("  %s < /dev/sda\n\n", argv[0]);
    printf("Please be aware that doing so in an unprotected environment,\n");
    printf("(outside your home network for instance) is not recommended, due\n");
    printf("to possible security issues.\n\n");
    printf("HTTPipe home page: <https://github.com/N0NamedGuy/httpipe/>\n");
    printf("Report bugs and issues in the GitHub tracker\n");
    printf("Pull requests are welcome\n");
    printf("Original implementation by:\nDavid Serrano <david.nonamedguy@gmail.com>\n");
}

void set_options(int argc, char** argv) {
    int c;

    static struct option long_options[] = {
        {"verbose", no_argument,        0, 'v'},
        {"file",    required_argument,  0, 'f'},
        {"port",    required_argument,  0, 'p'},
        {"mime",    required_argument,  0, 'm'},
        {"help",    no_argument,        0, 'h'},
        {0, 0, 0, 0}
    };
    int option_index = 0;

    g_buf_size = DEF_BUF_SIZE;
    g_port = DEF_PORT;
    g_verbose = DEF_VERBOSE;

    while ((c = getopt_long(argc, argv, "vf:p:h",
                long_options, &option_index))) {
        if (c == -1) break;

        switch (c) {
            case 'f':
                strncpy((char*)g_filename, optarg, MAX_FILENAME);
                break;

            case 'p':
                g_port = atoi(optarg);
                break;

            case 'm':
                strncpy((char*)g_mime, optarg, MAX_MIME);
                break;

            case 'v':
                g_verbose = true;
                break;

            case 'h':
                print_help(argc, argv);
                exit(0);
                break;
        }
    }
}

int startup_server(int port) {
    int listenfd;
    struct sockaddr_in serv_addr;
    int reuseaddr_opt;

    listenfd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    reuseaddr_opt = 1;
    setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, &reuseaddr_opt, sizeof reuseaddr_opt);

    if (!listenfd) {
        perror("Couldn't create a socket");
        return -1;
    } 

    memset(&serv_addr, 0, sizeof(serv_addr)); 
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    serv_addr.sin_port = htons(port);

    if ((bind(listenfd, (struct sockaddr*)&serv_addr, sizeof(serv_addr))) == -1) {
        perror("Couldn't bind");
        return -2;
    }

    if (listen(listenfd, 0) == -1) {
        perror("Couldn't listen for incoming requests"); 
        return -3;
    }

    return listenfd;
}

bool read_request(int connfd) {
    char* buf;

    buf = malloc(g_buf_size);
    memset((void*)buf, 0, g_buf_size);

    read(connfd, buf, g_buf_size);
    
    free(buf);
    return true;
}

int waitconn(int listenfd) {
    int connfd;
    connfd = accept(listenfd, (struct sockaddr*)NULL, NULL);

    if (!read_request(connfd)) {
        fputs("Wrong request or HTTP method", stderr);
        connfd = -1;
    }

    return connfd;
}

int send_headers(int connfd) {
    char* buf;

    buf = malloc(g_buf_size);
    memset((void*)buf, 0, g_buf_size);

    snprintf((char*)buf, g_buf_size, "HTTP/1.0 %d %s\r\n", 200, "OK");
    write(connfd, buf, strlen(buf));

    snprintf((char*)buf, g_buf_size, "Content-Type: %s\r\n\r\n", g_mime);
    write(connfd, buf, strlen(buf));

    free(buf);
    return 0;
}

int send_file (int connfd, FILE* fp) {
    char* buf;
    size_t n;

    buf = malloc(g_buf_size);
    memset((void*)buf, 0, g_buf_size);

    while ((n = fread(buf, 1, g_buf_size, fp)) > 0) {
        write(connfd, buf, n);
    }

    free(buf);
    return 0;
}

int main(int argc, char** argv) {
    int listenfd;
    int connfd;
    FILE* fp;

    set_options(argc, argv);
    if (strcmp(g_filename, "")) {
        if ((fp = fopen(g_filename, "r")) == NULL) {
            perror("File not found");
            return 1;
        }
    } else {
        fp = stdin;
    }

    listenfd = startup_server(g_port);
    if (listenfd < 0) return abs(listenfd);

    connfd = waitconn(listenfd);
    if (connfd < 0) return abs(connfd);

    send_headers(connfd);
    send_file(connfd, fp);

    fclose(fp);
    close(connfd);
    close(listenfd);

    return 0;
}
