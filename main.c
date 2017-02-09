#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
// dla inet_pton()
#include <arpa/inet.h>
// read() write()
#include <unistd.h>
// dla pthread_creat() i pthread_detach()
#include <pthread.h>
// dla optarg
#include <getopt.h>

char g_ip[16];
char g_port[6];
char g_dir[100];

int daemonize()
{
    pid_t pid = fork();

    if ( pid == 0 ) {
        setsid();
        chdir("/");

        close(0); close(1); close(2);

        return 0;
    }

    if ( pid == -1 )
        return -1;

    printf("Success: process %d went to background\n", pid);
    return pid;
}

struct http_msg {
    char type;    // G - get, ...
    char file_path[40];
};

struct http_msg parser_http(char *str, int n)
{
    struct http_msg tmp;

    if ( str[0] == 'G' && str[1] == 'E' && str[2] == 'T' )
    {
        tmp.type = 'G';

        char *s1 = strpbrk(str, " ");
        char *s2 = strpbrk(s1+1, " ?");

        memset(tmp.file_path, 0, sizeof(tmp.file_path));
        memcpy(tmp.file_path, s1+1, s2-s1-1);
        ///tmp.file_path[s2-s1] = '\0';

//        std::cout << "tmp.file_path: " << tmp.file_path << std::endl;
//        std::cout << "tmp.file_path length: " << s2-s1 << std::endl;
        return tmp;
    }
}

void *worker(void *arg)
{
    int sockfd = *(int *)arg;

    char buf[1000];

    int length = read(sockfd, buf, sizeof(buf));
    buf[length] = '\0';

    //std::cout << "buf: \n" << buf << "\n";

    struct http_msg tmp = parser_http(buf, length);

    ///std::cout << "tmp.file_path: " << tmp.file_path+1 << "\n\n";
    ///std::cout << "strlen(tmp.file_path+1): " << strlen(tmp.file_path+1) << "\n\n";

    if ( tmp.type = 'G' )
    {
        char *path_to_file = (char *) malloc(strlen(g_dir) + strlen(tmp.file_path) + 1);
        strcpy(path_to_file, g_dir);
        strcat(path_to_file, tmp.file_path);

        FILE *file = fopen(path_to_file, "rb");

        if ( file == NULL )
        {
            ///std::cout << "file == NULL\n";

            char out_msg[2000];
            memset(out_msg, 0, sizeof(out_msg));

            strcat(out_msg, "HTTP/1.0 404 Not Found\r\n");
            strcat(out_msg, "Server: A8ykov-server\r\n");
            strcat(out_msg, "Content-Length: 0\r\n");
            strcat(out_msg, "Connection: close\r\n");

            write(sockfd, out_msg, strlen(out_msg));
        }
        else
        {
            char read_msg[1000];
            memset(read_msg, 0, sizeof(read_msg));

            size_t count = fread(read_msg, sizeof(read_msg), 1, file);
            // dopyshenie count < 2000
            //ind = count;

            char out_msg[2000];
            memset(out_msg, 0, sizeof(out_msg));

            strcat(out_msg, "HTTP/1.0 200 OK\r\n");
            strcat(out_msg, "Server: A8ykov-server\r\n");
            strcat(out_msg, "Date: Wed, 11 Feb 2009 11:20:59 GMT\r\n");

            char integer[10];
            sprintf(integer, "%i", strlen(read_msg));
            strcat(out_msg, "Content-Length: ");
            strcat(out_msg, integer);
            strcat(out_msg, "\r\n\n");
            strcat(out_msg, read_msg);

            write(sockfd, out_msg, sizeof(out_msg));
        }
    }

    close(sockfd);
}

int main(int argc, char *argv[])
{
    // obrabotka parametrov
    int rez=0;
    while ( (rez = getopt(argc, argv, "h:p:d:")) != -1 )
    {
        switch (rez) {
        case 'h':
            printf("argument h: %s\n", optarg);
            strcpy(g_ip, optarg);
            break;
        case 'p':
            printf("argument p: %s\n", optarg);
            strcpy(g_port, optarg);
            break;
        case 'd':
            printf("argument d: %s\n", optarg);
            strcpy(g_dir, optarg);
            break;
        case '?':
            return 1;
            break;
        }
    }
    // ----- ----- -----

    if ( daemonize() > 0 ) {
        return 0;
    }

    int listenfd, connfd;
    socklen_t clilen;
    struct sockaddr_in cliaddr, servaddr;

    listenfd = socket(AF_INET, SOCK_STREAM, 0);

    int optval = 1;
    setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval));

    memset(&servaddr, 0, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    //servaddr.sin_addr.s_addr = htonl(/*INADDR_ANY*/);
    inet_pton(AF_INET, g_ip, &(servaddr.sin_addr));
    servaddr.sin_port = htons(/*3492*/atoi(g_port));

    bind(listenfd, (struct sockaddr*)&servaddr, sizeof(servaddr));

    listen(listenfd, 10);


    pthread_t *thread1;
    // osnovnoi cikl
    while (1)
    {
        memset(&cliaddr, 0, sizeof(cliaddr));
        connfd = accept(listenfd, (struct sockaddr*)&cliaddr, &clilen);

        thread1 = (pthread_t*) malloc( sizeof(pthread_t) );  // net free
        pthread_create(thread1, NULL, worker, &connfd);
        pthread_detach(*thread1);
    }
}
