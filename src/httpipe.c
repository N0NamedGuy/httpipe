#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>

#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>

#define MAX_FILENAME 256

#define DEF_BUF_SIZE 1024
#define DEF_PORT 5000

int g_buf_size;
int g_port;
char g_filename[MAX_FILENAME];

void set_options(int argc, char** argv) {
    g_buf_size = DEF_BUF_SIZE;
    g_port = DEF_PORT;
   
    if (argc > 1) {
        strncpy((char*)g_filename, argv[1], MAX_FILENAME);
    } else {
        strncpy((char*)g_filename, "", MAX_FILENAME); 
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

bool starts_with(const char *pre, const char *str)
{
    size_t lenpre = strlen(pre),
           lenstr = strlen(str);
    return lenstr < lenpre ? false : strncmp(pre, str, lenpre) == 0;
}

bool read_request(int connfd) {
    char* buf;

    buf = malloc(g_buf_size);

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

int main(int argc, char** argv) {
    char* buf;
    size_t n;
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

    buf = malloc(g_buf_size);


    while ((n = fread(buf, 1, g_buf_size, fp)) > 0) {
        write(connfd, buf, n);
    }

    fclose(fp);

    close(connfd);
    shutdown(listenfd, SHUT_RDWR);

    free(buf);

    return 0;
}
