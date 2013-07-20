#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <time.h>

#include <getopt.h>

#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>

#define MAX_FILENAME 256
#define MAX_MIME 256

#define DEF_BUF_SIZE 1024*1024
#define DEF_PORT 5000
#define DEF_VERBOSE false
#define DEF_SILENT false
#define DEF_MIME "application/octet-stream"

#define printverb(fmt, ...) \
    do { if (g_verbose) fprintf(stderr, fmt, __VA_ARGS__); } while (0)

#define printerr(str) \
    do { if (!g_silent) perror(str); } while (0)

int g_buf_size;
int g_port;
char g_filename[MAX_FILENAME];
bool g_verbose;
bool g_silent;
char g_mime[MAX_MIME];

void print_help(int argc, char** argv) {
    printf("Usage: %s [OPTIONS]\n", argv[0]);
    printf("Outputs given input to HTTP\n\n");
    printf("Options:\n");
    printf("  -f, --file=FILE\toutputs file to HTTP. If FILE is undefined,\n");
    printf("\t\t\tinput data will be read from stdin\n");
    printf("  -p, --port=PORT\tsets the port to which the program\n");
    printf("\t\t\twill listen for incoming connections.\n");
    printf("\t\t\tDefaults to %d\n", DEF_PORT);
    printf("  -m, --mime=MIME\tsets the output MIME type.\n");
    printf("\t\t\tDefaults to %s\n", DEF_MIME);
    printf("  -v, --verbose\t\tbe verbose (can track sending speed)\n");
    printf("  -s, --silent\t\tbe silent (no output at all)\n");
    printf("  -h, --help\t\tshows this help message\n\n");
    printf("This program is meant to be used as an HTTP pipe.\n");
    printf("It was conceived so it could be possible to easily transfer\n");
    printf("a disk image throught HTTP.\n");
    printf("That can be accomplished by issuing:\n");
    printf("  cat /dev/sda | gzip -9 | %s\n\n", argv[0]);
    printf("And on the client:\n");
    printf("  curl http://example.com/image.img.gz -O\n\n");
    printf("Please be aware that doing so in an unprotected environment,\n");
    printf("(outside your home network for instance) is not recommended,\n");
    printf("due to possible security issues.\n\n");
    printf("HTTPipe home page: <https://github.com/N0NamedGuy/httpipe/>\n");
    printf("Report bugs and issues in the GitHub tracker\n");
    printf("Pull requests are welcome\n");
    printf("Original implementation by:\n");
    printf("David Serrano <david.nonamedguy@gmail.com>\n");
}

void set_options(int argc, char** argv) {
    int c;

    static struct option long_options[] = {
        {"verbose", no_argument,        0, 'v'},
        {"silent",  no_argument,        0, 's'},
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

    while ((c = getopt_long(argc, argv, "vsf:p:m:h",
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
            
            case 's':
                g_silent = true;
                break;

            case 'h':
                print_help(argc, argv);
                exit(0);
                break;
        }
    }

    if (g_silent && g_verbose) {
        printverb("%s\n", "Can't be silent and verbose at the same time.");
        exit(1);
    }
}

int startup_server(int port) {
    int listenfd;
    struct sockaddr_in serv_addr;
    int reuseaddr_opt;

    /* Gets hold of a socket for ourselves */
    listenfd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    /* Import lines of code, so we can avoid the TIME_WAIT limbo */
    reuseaddr_opt = 1;
    setsockopt(listenfd, SOL_SOCKET,
            SO_REUSEADDR, &reuseaddr_opt, sizeof reuseaddr_opt);

    if (!listenfd) {
        printerr("Couldn't create a socket");
        return -1;
    } 

    memset(&serv_addr, 0, sizeof(serv_addr)); 
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    serv_addr.sin_port = htons(port);

    if ((bind(
                    listenfd,
                    (struct sockaddr*)&serv_addr,
                    sizeof(serv_addr))) == -1) {
        printerr("Couldn't bind");
        return -2;
    }

    if (listen(listenfd, 0) == -1) {
        printerr("Couldn't listen for incoming requests"); 
        return -3;
    }

    return listenfd;
}

bool read_request(int connfd) {
    char* buf;

    buf = malloc(g_buf_size);
    memset((void*)buf, 0, g_buf_size);

    /* We just read incoming HTTP headers and such */
    /* For now, we just don't care about anything */
    read(connfd, buf, g_buf_size);
    
    free(buf);
    return true;
}

int waitconn(int listenfd) {
    int connfd;
    struct sockaddr_in other_addr;
    static char addr[INET_ADDRSTRLEN];
    socklen_t addr_size;

    printverb("Waiting for incoming connection on port %d\n", g_port);
    connfd = accept(listenfd, (struct sockaddr*)&other_addr, &addr_size);

    /* Tell us who is connecting */ 
    if (g_verbose) {
        getpeername(connfd, (struct sockaddr*)&other_addr, &addr_size);
        inet_ntop(AF_INET, &(other_addr.sin_addr), addr, INET_ADDRSTRLEN);
        printverb("Accepting connection from %s\n", addr);
    }

    /* We read the request, and act accordingly */
    if (!read_request(connfd)) {
        if (!g_silent) fputs("Wrong request or HTTP method", stderr);
        connfd = -1;
    }

    return connfd;
}

int send_headers(int connfd) {
    char* buf;

    buf = malloc(g_buf_size);
    memset((void*)buf, 0, g_buf_size);

    /* We're dumb, so we tell we only accept HTTP/1.0 */
    snprintf((char*)buf, g_buf_size, "HTTP/1.0 %d %s\r\n", 200, "OK");
    write(connfd, buf, strlen(buf));

    /* Set the MIME type to what we want it to be */
    snprintf((char*)buf, g_buf_size, "Content-Type: %s\r\n\r\n", g_mime);
    write(connfd, buf, strlen(buf));

    free(buf);
    return 0;
}

/* Creates a "human-readable" string in bytes In other words, it converts bytes
 * to the highest order possible */
void human_readable(unsigned int bytes, char* out, const size_t str_size) {
    static char units[] = "BKMGTPE";
    unsigned int remainder = bytes;
    int unit = 0;

    while (remainder > 1024) {
        remainder = remainder / 1024;
        unit++;
    }

    snprintf(out, str_size, "%d%c", remainder, units[unit]);
}

/* Outputs a file described by the file pointer fp thru a socket connfd */
int send_file (int connfd, FILE* fp) {
    char* buf;
    size_t n;
    size_t total, cur_total;
    time_t last, cur;

    /* Get us a nice clean buffer */
    buf = malloc(g_buf_size);
    memset((void*)buf, 0, g_buf_size);


    /* Set up the statistic variables if we are being verbose */
    if (g_verbose) {
        last = time(NULL);
        cur_total = 0;
        total = 0;
        puts("");
    } else {
        /* Damn gcc complaining... */
        last = 0;
    }


    while ((n = fread(buf, 1, g_buf_size, fp)) > 0) {
        /* All the program revolves around this write */
        write(connfd, buf, n);
    
        /* Print statistics */
        if (g_verbose) {
            double elapsed;
            cur_total += n;

            cur = time(NULL);
            elapsed = difftime(cur, last); 
            if (elapsed > 1.0) {
                static char human_total[6];
                static char human_transfer[6];
                total += cur_total;

                /* Get the total transfered bytes and the current transfer
                 * speed in a human readable format */
                human_readable(total, human_total, 6);
                human_readable(cur_total / elapsed, human_transfer, 6);

                printverb("\033[F\033[JSent %s (%s per second)\n",
                        human_total,
                        human_transfer);

                /* Reset stuff */
                cur_total = 0;
                last = cur;
            }
        }
    }

    free(buf);
    return 0;
}

int main(int argc, char** argv) {
    int listenfd;
    int connfd;
    FILE* fp;

    set_options(argc, argv);

    /* Open the file early, so we may fail early.
     * To note that if no file is specified, stdin will be read instead */
    if (strcmp(g_filename, "")) {
        if ((fp = fopen(g_filename, "r")) == NULL) {
            printerr("File not found");
            return 1;
        }
    } else {
        fp = stdin;
    }

    /* Start listening */
    listenfd = startup_server(g_port);
    if (listenfd < 0) return abs(listenfd);

    /* Start waiting for connections */
    connfd = waitconn(listenfd);
    if (connfd < 0) return abs(connfd);

    /* Send stuff */
    send_headers(connfd);
    send_file(connfd, fp);

    /* Cleanup */
    fclose(fp);
    close(connfd);
    close(listenfd);

    return 0;
}
