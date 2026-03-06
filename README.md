# SocTalker

一个 C/S 模式的 socket 聊天工具，支持多客户端并发连接。

## 功能特性

- 🖥️ 多客户端并发支持
- 📢 管理员广播消息
- 👤 客户端代号自动分配 (Codename A, B, C...)
- 📜 命令历史记录
- 🔒 线程安全的客户端管理

## 编译

### 依赖

- g++ (支持 C++11 或更高版本)
- libreadline

### 编译命令

```bash
# macOS
brew install readline

# Ubuntu/Debian
sudo apt-get install libreadline-dev

# 编译服务器
g++ -std=c++11 -o server server.cc -lpthread -lreadline

# 编译客户端
g++ -std=c++11 -o client client.cc -lpthread -lreadline
```

## 使用方法

### 启动服务器

```bash
./server <端口号>

# 示例
./server 9285
```

服务器启动后会：
1. 监听指定端口
2. 启动管理员线程，等待管理员输入
3. 接受客户端连接并创建独立线程处理

### 连接客户端

**方式一：使用专用客户端**
```bash
./client <服务器 IP> <端口号>

# 示例
./client 127.0.0.1 9285
```

**方式二：使用 netcat**
```bash
nc <服务器 IP> <端口号>

# 示例
nc 127.0.0.1 9285
```

### 管理员命令

在服务器控制台输入：
- 直接输入消息 → 广播给所有客户端
- 输入 `` `exit` `` → 退出服务器

### 客户端命令

- 输入任意消息 → 发送给所有其他客户端
- 输入 `exit` → 断开连接

## 使用示例

**服务器终端：**
```bash
$ ./server 9285
 SocTalker 0.1 started.
 Listen on 9285.
> hello
From Admin: hello
> Codename E connected (127.0.0.1:35764)
From Admin: Codename E connected.
From E: hello admin
Codename F connected (127.0.0.1:35766)
From Admin: Codename F connected.
From F: hello, I'm in.

> hi, all members.
From Admin: hi, all members.
> From Admin: Codename E disconnected.
```

**客户端 1：**
```bash
$ nc 127.0.0.1 9285
 SocTalker 0.1
 Use (127.0.0.1:35764)
From Admin: Welcome, Codename E.
hello admin
From Admin: Codename F connected.
From F: hello, I'm in.
From Admin: hi, all members.
^C
$
```

**客户端 2：**
```bash
$ nc 127.0.0.1 9285
 SocTalker 0.1
 Use (127.0.0.1:35766)
From Admin: Welcome, Codename F.
hello, I'm in.
From Admin: hi, all members.
From Admin: Codename E disconnected.
```

## 项目结构

```
soctalker/
├── server.cc      # 服务器端代码
├── client.cc      # 客户端代码
├── Makefile       # 编译配置
└── README.md      # 说明文档
```

## 技术细节

- 使用 POSIX socket API 进行网络通信
- 使用 pthread 实现多线程并发
- 使用互斥锁 (std::mutex) 保护共享资源
- 使用 readline 库提供命令行历史功能

## License

MIT License
