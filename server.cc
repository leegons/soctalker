#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <readline/readline.h>
#include <readline/history.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include <stdarg.h>
#include <pthread.h>
#include <mutex>
#include <vector>

std::vector<int> g_clifds;

void error(const char* msg) {
    perror(msg);
    exit(1);
}

void send_msg(int fd, const char* fmt, ...) {
    char buffer[1024];
    va_list args;
    va_start(args, fmt);
    vsnprintf(buffer, 1024, fmt, args);
    send(fd, buffer, strlen(buffer), 0);
    va_end(args);
}

void send_others(int myfd, const char* fmt, ...) {
    char buffer[1024];
    va_list args;
    va_start(args, fmt);
    vsnprintf(buffer, 1024, fmt, args);
    for (auto fd : g_clifds) {
        if (fd != myfd) {
            send_msg(fd, "%s", buffer);
        }
    }
    printf("%s", buffer);
    va_end(args);
}

void* admin_writer(void* argv) {
    while (true) {
        char* msg = readline("> ");
        if (strcmp(msg, "`exit`") == 0) {
            free(msg);
            break;
        }
        send_others(0, "From Admin: %s\n", msg);
        free(msg);
    }
    exit(0);
}

void* deal_with_client(void* argv) {
    int clifd = *(int*)argv;
    send_msg(clifd, "From Admin: Welcome, Codename %c.\n", 'A' + clifd);
    send_others(clifd, "From Admin: Codename %c connected.\n", 'A' + clifd);

    while (true) {
        char buffer[256];
        bzero(buffer, 256);

        int n = read(clifd, buffer, 255);
        if (n < 0) {
            error("ERROR reading from socket");
        }
        if (n == 0) {
            break;
        }
        if (strcmp(buffer, "exit") == 0 || buffer[0] == '\0') {
            break;
        }

        send_others(clifd, "From %c: %s", 'A' + clifd, buffer);
    }

    send_others(clifd, "From Admin: Codename %c disconnected.\n", 'A' + clifd);

    for (auto it = g_clifds.begin(); it != g_clifds.end(); ++it) {
        if (*it == clifd) {
            g_clifds.erase(it);
            break;
        }
    }
    close(clifd);
}


int main(int argc, char *argv[]) {
    if (argc < 2) {
        error("ERROR: no port provided\n");
    }
    int portno = atoi(argv[1]);

    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) {
        error("ERROR opening socket");
    }

    struct sockaddr_in serv_addr;
    bzero((char *)&serv_addr, sizeof(serv_addr));


    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = INADDR_ANY;
    serv_addr.sin_port = htons(portno);

    int ok = 0;
    ok = bind(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr));
    if (ok < 0) {
        error("ERROR on binding");
    }

    ok = listen(sockfd, 5);
    if (ok != 0) {
        error("ERROR on listen");
    }
    printf(" SocTalker 0.1 started.\n Listen on %d.\n", portno);

    // add admin writer
    {
        pthread_t thread;
        pthread_create(&thread, NULL, admin_writer, NULL);
    }

    // loop to accept connection
    while (true) {
        struct sockaddr_in cli_addr;
        socklen_t clilen = sizeof(cli_addr);

        int newfd = accept(sockfd, (struct sockaddr *)&cli_addr, &clilen);
        if (newfd < 0) {
            error("ERROR on accept");
        }

        printf("Codename %c connected (%s:%d)\n", 'A' + newfd,
                inet_ntoa(cli_addr.sin_addr), ntohs(cli_addr.sin_port));
        send_msg(newfd, " SocTalker 0.1\n Use (%s:%d)\n",
                inet_ntoa(cli_addr.sin_addr), ntohs(cli_addr.sin_port));

        g_clifds.push_back(newfd);

        pthread_t thread;
        int pid = pthread_create(&thread, NULL, deal_with_client, &newfd);
    }

    close(sockfd);
    return 0;
}

/* vim: set expandtab ts=4 sw=4 sts=4 tw=100: */
