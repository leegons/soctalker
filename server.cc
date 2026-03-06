/**
 * SocTalker Server - 多客户端聊天服务器
 * 
 * 一个基于 socket 的 C/S 模式聊天工具服务器端
 * 支持多客户端并发连接，管理员可广播消息
 * 
 * @author leegons
 * @version 0.1
 */

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

/**
 * 全局客户端文件描述符列表
 * 注意：多线程访问时需要加锁保护
 */
std::vector<int> g_clifds;
std::mutex g_clifds_mutex;  // 保护全局列表的互斥锁

/**
 * 错误处理函数
 * @param msg 错误消息
 */
void error(const char* msg) {
    perror(msg);
    exit(1);
}

/**
 * 向指定文件描述符发送消息
 * @param fd 目标文件描述符
 * @param fmt 格式化字符串
 * @param ... 可变参数
 */
void send_msg(int fd, const char* fmt, ...) {
    char buffer[1024];
    va_list args;
    va_start(args, fmt);
    vsnprintf(buffer, 1024, fmt, args);
    send(fd, buffer, strlen(buffer), 0);
    va_end(args);
}

/**
 * 向除指定客户端外的所有其他客户端广播消息
 * 同时将消息打印到服务器控制台
 * 
 * @param myfd 排除的文件描述符（发送者）
 * @param fmt 格式化字符串
 * @param ... 可变参数
 */
void send_others(int myfd, const char* fmt, ...) {
    char buffer[1024];
    va_list args;
    va_start(args, fmt);
    vsnprintf(buffer, 1024, fmt, args);
    
    // 线程安全地遍历客户端列表
    std::lock_guard<std::mutex> lock(g_clifds_mutex);
    for (auto fd : g_clifds) {
        if (fd != myfd) {
            send_msg(fd, "%s", buffer);
        }
    }
    
    // 在服务器控制台打印消息
    printf("%s", buffer);
    fflush(stdout);  // 确保立即输出
    va_end(args);
}

/**
 * 管理员线程函数
 * 在独立线程中运行，读取管理员输入并广播给所有客户端
 * 输入 `exit` 退出服务器
 */
void* admin_writer(void* argv) {
    while (true) {
        char* msg = readline("> ");
        if (msg == NULL) {  // 处理 EOF
            break;
        }
        if (strcmp(msg, "`exit`") == 0) {
            free(msg);
            break;
        }
        if (strlen(msg) > 0) {
            add_history(msg);  // 添加到命令历史
            send_others(0, "From Admin: %s\n", msg);
        }
        free(msg);
    }
    exit(0);
}

/**
 * 客户端处理线程函数
 * 为每个连接的客户端创建独立线程处理消息收发
 * 
 * @param argv 客户端文件描述符指针
 */
void* deal_with_client(void* argv) {
    int clifd = *(int*)argv;
    free(argv);  // 释放动态分配的内存
    
    // 发送欢迎消息
    send_msg(clifd, "From Admin: Welcome, Codename %c.\n", 'A' + clifd);
    // 通知其他客户端有新连接
    send_others(clifd, "From Admin: Codename %c connected.\n", 'A' + clifd);

    // 主循环：持续读取客户端消息
    while (true) {
        char buffer[256];
        memset(buffer, 0, sizeof(buffer));  // 使用 memset 替代过时的 bzero

        int n = read(clifd, buffer, 255);
        if (n < 0) {
            error("ERROR reading from socket");
        }
        if (n == 0) {  // 客户端关闭连接
            break;
        }
        if (strcmp(buffer, "exit") == 0 || buffer[0] == '\0') {
            break;  // 收到退出命令或空消息
        }

        // 将消息广播给其他客户端
        send_others(clifd, "From %c: %s", 'A' + clifd, buffer);
    }

    // 通知其他客户端此用户已断开
    send_others(clifd, "From Admin: Codename %c disconnected.\n", 'A' + clifd);

    // 从全局列表中移除该客户端
    {
        std::lock_guard<std::mutex> lock(g_clifds_mutex);
        for (auto it = g_clifds.begin(); it != g_clifds.end(); ++it) {
            if (*it == clifd) {
                g_clifds.erase(it);
                break;
            }
        }
    }
    
    close(clifd);  // 关闭文件描述符
    return NULL;
}

/**
 * 服务器主函数
 * @param argc 参数个数
 * @param argv 参数数组，需要指定端口号
 * @return 退出码
 */
int main(int argc, char *argv[]) {
    // 检查端口参数
    if (argc < 2) {
        error("ERROR: no port provided\n");
    }
    int portno = atoi(argv[1]);

    // 创建 socket
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) {
        error("ERROR opening socket");
    }

    // 初始化服务器地址结构
    struct sockaddr_in serv_addr;
    memset((char *)&serv_addr, 0, sizeof(serv_addr));

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = INADDR_ANY;  // 监听所有网络接口
    serv_addr.sin_port = htons(portno);

    // 绑定 socket 到指定端口
    int ok = bind(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr));
    if (ok < 0) {
        error("ERROR on binding");
    }

    // 开始监听
    ok = listen(sockfd, 5);
    if (ok != 0) {
        error("ERROR on listen");
    }
    printf(" SocTalker 0.1 started.\n Listen on %d.\n", portno);
    fflush(stdout);

    // 创建管理员线程
    {
        pthread_t thread;
        pthread_create(&thread, NULL, admin_writer, NULL);
        pthread_detach(thread);  // 分离线程，避免资源泄漏
    }

    // 主循环：接受客户端连接
    while (true) {
        struct sockaddr_in cli_addr;
        socklen_t clilen = sizeof(cli_addr);

        int newfd = accept(sockfd, (struct sockaddr *)&cli_addr, &clilen);
        if (newfd < 0) {
            error("ERROR on accept");
        }

        // 打印连接信息
        printf("Codename %c connected (%s:%d)\n", 'A' + newfd,
                inet_ntoa(cli_addr.sin_addr), ntohs(cli_addr.sin_port));
        fflush(stdout);
        
        // 发送欢迎消息
        send_msg(newfd, " SocTalker 0.1\n Use (%s:%d)\n",
                inet_ntoa(cli_addr.sin_addr), ntohs(cli_addr.sin_port));

        // 将新客户端添加到全局列表
        {
            std::lock_guard<std::mutex> lock(g_clifds_mutex);
            g_clifds.push_back(newfd);
        }

        // 创建新线程处理该客户端
        pthread_t thread;
        int* fd_ptr = (int*)malloc(sizeof(int));
        *fd_ptr = newfd;
        int pid = pthread_create(&thread, NULL, deal_with_client, fd_ptr);
        if (pid != 0) {
            error("ERROR creating thread");
        }
        pthread_detach(thread);  // 分离线程
    }

    close(sockfd);
    return 0;
}

/* vim: set expandtab ts=4 sw=4 sts=4 tw=100: */
