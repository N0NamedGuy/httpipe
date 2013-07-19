#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>

#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>

#define DEF_BUF_SIZE 1024
#define DEF_PORT 5000

int g_buf_size;
int g_port;

void set_options(int argc, char** argv) {
    g_buf_size = DEF_BUF_SIZE;
    g_port = DEF_PORT;
}

int startup_server(int port) {
    int listenfd;
    struct sockaddr_in serv_addr;
    char optval;

    listenfd = socket(AF_INET, SOCK_STREAM, 0);
    if (!listenfd) {
        perror("Couldn't create a socket");
        return -1;
    } 

    memset(&serv_addr, 0, sizeof(serv_addr)); 
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    serv_addr.sin_port = htons(port);

    optval = 1;
    setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof optval);

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

bool starts_with(const char *pre, const char *str)
{
    size_t lenpre = strlen(pre),
           lenstr = strlen(str);
    return lenstr < lenpre ? false : strncmp(pre, str, lenpre) == 0;
}

bool read_request(int connfd) {
    char* buf;
    bool ret;

    buf = malloc(g_buf_size);

    read(connfd, buf, g_buf_size);

    ret = starts_with(buf, "GET");
    
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

int main(int argc, char** argv) {
    char* buf;
    size_t n;
    int listenfd;
    int connfd;

    set_options(argc, argv);
    listenfd = startup_server(g_port);
    if (listenfd < 0) return abs(listenfd);

    connfd = waitconn(listenfd);
    if (connfd < 0) return abs(connfd);

    buf = malloc(g_buf_size);

    while ((n = fread(buf, 1, g_buf_size, stdin)) > 0) {
        write(connfd, buf, n);
    }

    close(connfd);
    shutdown(listenfd, SHUT_RDWR);
    printf("The end\n");

    free(buf);

    return 0;
}
