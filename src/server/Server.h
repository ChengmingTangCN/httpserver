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

    // 设置监听socket以及连接socket额外监视的事件集合
    void initExtraEvents();

    // 初始化监听socket
    bool initListenSock();

    // 将文件描述符修改为非阻塞模式
    bool setNonBlocking(int fd);

private:

    // 处理监听socket的可读事件
    void dealListen();

    // 处理连接socket的可读事件
    void dealRead(HttpConn &http_conn);

    // 处理连接socket的可写事件
    void dealWrite(HttpConn &http_conn);

private:

    // 添加一个Http连接实例
    void addHttpConn(int conn_sock, const struct sockaddr_in &client_addr);

    // 关闭一个Http连接实例
    void closeHttpConn(HttpConn &httpConn);

    // 读任务
    void onRead(HttpConn &httpConn);

    // 写任务
    void onWrite(HttpConn &httpConn);

    // 服务器直接给客户端发送错误信息并关闭socket
    // !!! 应该在accept(2)后直接执行, 不要操作已经绑定到HttpConn对象的socket
    void sendError(int sock, const std::string &msg);

    void extentTime(const HttpConn &);

private:

    static constexpr int MAX_NUMBER_HTTP_CONNS = 60000;  // 服务器支持的最大并发数
    static constexpr int CHECK_CONN_TIME_SLOT_SECONDS = 5;  // 服务器每隔5s定时检查不活跃的连接

    bool                              is_running_;     // 服务器是否正在运行

    uint16_t                          port_;  // 服务器端口号
    int                               listen_sock_;  // 监听socket

    std::unique_ptr<ThreadPool>       p_thread_pool_; // 线程池+任务队列
    std::unique_ptr<Epoller>          p_epoller_;     // epoll实例

    uint32_t extra_listen_events_;
    uint32_t extra_conn_events_;

    std::unordered_map<int, HttpConn> http_conns_;  // 连接socket到http连接的映射
    TimerManager timer_manager_;  // 定时器组件
    
};

#endif
