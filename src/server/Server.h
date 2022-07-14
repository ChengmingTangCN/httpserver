#ifndef HTTPSERVER_SERVER_H
#define HTTPSERVER_SERVER_H

#include <pool/ThreadPool.h>
#include <server/Epoller.h>
#include <http/HttpConn.h>
#include <timer/TimerManager.h>

#include <cstdint>
#include <unordered_map>
#include <vector>
#include <memory>

class Server
{
public:
    Server(uint16_t port, int threads_num);
    Server(const Server &) = delete;
    Server(Server &&) = delete;

    Server &operator=(const Server &) = delete;
    Server &operator=(Server &&) = delete;

    ~Server();

public:
    // 运行服务器
    void run();

private:
    // 设置监听socket以及连接socket固定监视的事件集合
    void initSocketEvents();

    // 初始化监听socket
    bool initListenSock();

private:
    // 处理监听socket的可读事件
    void dealListen();

    // 处理连接socket的可读事件
    void dealRead(int sock);

    // 处理连接socket的可写事件
    void dealWrite(int sock);

private:
    // 添加一个Http连接实例
    void addHttpConn(int conn_sock, const struct sockaddr_in &client_addr);

    // 删除Server维护的某个Http连接实例
    void closeHttpConn(int sock);

    // 删除Server维护的某个Http连接实例并取消对应定时器
    void closeHttpConnAndCancelTimer(int sock);

private:
    // 读任务
    void onRead(int sock);

    // 写任务
    void onWrite(int sock);

private:
    // 服务器直接给客户端发送错误信息并关闭socket
    // !!! 应该在accept(2)后直接执行, 不要操作已经绑定到HttpConn对象的socket
    void sendError(int sock, const std::string &msg);

    // 延长Http连接对应的定时器过期时间
    void extentTime(int sock);

    // 将文件描述符修改为非阻塞模式
    bool setNonBlocking(int fd);

private:
    static constexpr int MAX_NUMBER_HTTP_CONNS = 60000;      // 服务器支持的最大并发数
    static constexpr int CHECK_CONN_TIME_SLOT_SECONDS = 60;  // 服务器每隔60s定时检查不活跃的连接

    bool                              is_running_;           // 是否运行服务器

    uint16_t                          port_;
    int                               listen_sock_;
    std::unordered_map<int, std::unique_ptr<HttpConn>> sock_to_http_;  // 连接socket到http连接对象的映射

    std::unique_ptr<ThreadPool>       p_thread_pool_;
    std::unique_ptr<Epoller>          p_epoller_;
    TimerManager timer_manager_;

    uint32_t listen_epoll_events_;      // 监听socket需要监视的EPOLL事件
    uint32_t conn_epoll_events_;        // 连接socket需要监视的固定事件
};

#endif
