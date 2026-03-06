/**
 * SocTalker Client - 聊天客户端
 * 
 * 连接到 SocTalker 服务器，发送和接收消息
 * 使用 nc (netcat) 或此客户端均可连接
 * 
 * @author leegons
 * @version 0.1
 */

#include <string>
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
#include <pthread.h>

/**
 * 错误处理函数
 * @param msg 错误消息
 */
void error(const char* msg) {
    perror(msg);
    exit(1);
}

/**
 * 全局 socket 文件描述符，供接收线程使用
 */
int g_sockfd = -1;

/**
 * 消息接收线程函数
 * 持续从服务器读取消息并打印到控制台
 * 
 * @param argv 未使用
 */
void* receive_messages(void* argv) {
    char buffer[1024];
    
    while (true) {
        memset(buffer, 0, sizeof(buffer));
        int n = read(g_sockfd, buffer, sizeof(buffer) - 1);
        
        if (n < 0) {
            error("ERROR reading from socket");
        }
        if (n == 0) {
            // 服务器关闭连接
            printf("\nServer disconnected.\n");
            exit(0);
        }
        
        // 打印接收到的消息
        printf("%s", buffer);
        fflush(stdout);
        
        // 如果收到换行，重新显示提示符
        if (strchr(buffer, '\n') != NULL) {
            printf("> ");
            fflush(stdout);
        }
    }
    
    return NULL;
}

/**
 * 客户端主函数
 * 连接到指定服务器和端口，启动接收线程，然后处理用户输入
 * 
 * @param argc 参数个数
 * @param argv 参数数组：server_ip port
 * @return 退出码
 */
int main(int argc, char *argv[]) {
    // 检查参数
    if (argc < 3) {
        fprintf(stderr, "Usage: %s <server_ip> <port>\n", argv[0]);
        fprintf(stderr, "Example: %s 127.0.0.1 9285\n", argv[0]);
        exit(1);
    }
    
    const char* server_ip = argv[1];
    int portno = atoi(argv[2]);
    
    // 创建 socket
    g_sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (g_sockfd < 0) {
        error("ERROR opening socket");
    }
    
    // 设置服务器地址
    struct sockaddr_in serv_addr;
    memset((char *)&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(portno);
    
    // 转换 IP 地址
    if (inet_pton(AF_INET, server_ip, &serv_addr.sin_addr) <= 0) {
        error("ERROR invalid IP address");
    }
    
    // 连接服务器
    if (connect(g_sockfd, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) < 0) {
        error("ERROR connecting");
    }
    
    printf("Connected to SocTalker server at %s:%d\n", server_ip, portno);
    printf("Type 'exit' to disconnect.\n\n");
    
    // 创建消息接收线程
    pthread_t recv_thread;
    if (pthread_create(&recv_thread, NULL, receive_messages, NULL) != 0) {
        error("ERROR creating receive thread");
    }
    pthread_detach(recv_thread);  // 分离线程
    
    // 主循环：读取用户输入并发送到服务器
    while (true) {
        printf("> ");
        fflush(stdout);
        
        char* msg = readline("");
        if (msg == NULL) {
            // EOF (Ctrl+D)
            break;
        }
        
        // 检查退出命令
        if (strcmp(msg, "exit") == 0) {
            free(msg);
            break;
        }
        
        // 发送消息（添加换行符）
        if (strlen(msg) > 0) {
            char buffer[1024];
            snprintf(buffer, sizeof(buffer), "%s\n", msg);
            int n = write(g_sockfd, buffer, strlen(buffer));
            if (n < 0) {
                error("ERROR writing to socket");
            }
            
            // 添加到命令历史
            add_history(msg);
        }
        
        free(msg);
    }
    
    // 关闭连接
    printf("Disconnecting...\n");
    close(g_sockfd);
    
    return 0;
}

/* vim: set expandtab ts=4 sw=4 sts=4 tw=100: */
