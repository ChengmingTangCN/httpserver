#include <server/Server.h>

#include <sys/epoll.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <fcntl.h>

#include <cstring>

Server::Server(uint16_t port, int threads_num)
: is_running_(false),
  port_(port),
  listen_sock_(-1),
  p_thread_pool_(new ThreadPool(threads_num)),
  p_epoller_(new Epoller),
  extra_listen_events_(0),
  extra_conn_events_(0),
  http_conns_(),
  timer_manager_()
{
    // 设置额外需要监视的事件
    initExtraEvents();

    // 创建监听socket
    if (initListenSock())
    {
        is_running_ = true;
    }
}

// 设置监听socket以及连接socket额外监视的事件集合
void Server::initExtraEvents()
{
    extra_conn_events_ = EPOLLRDHUP | EPOLLHUP | EPOLLONESHOT | EPOLLET;
    extra_listen_events_ = 0;
}

// 初始化监听socket
bool Server::initListenSock()
{
    // 创建socket
    listen_sock_ = ::socket(AF_INET, SOCK_STREAM, 0);
    if (listen_sock_ < 0)
    {
        return false;
    }

    // 绑定地址
    struct sockaddr_in server_addr;
    std::memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = ::htonl(INADDR_ANY);
    server_addr.sin_port = ::htons(port_);

    // 重用地址
    int optval = 1;
    int ret = ::setsockopt(listen_sock_, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval));
    if (ret < 0)
    {
        return false;
    }

    if (::bind(listen_sock_, 
               reinterpret_cast<struct sockaddr*>(&server_addr),
               sizeof(server_addr)) < 0)
    {
        return false;
    }

    // 转换成监听socket
    if (::listen(listen_sock_, 5) < 0)
    {
        return false;
    }

    // 通过 p_epoller_ 监视监听socket
    if (!p_epoller_->addFd(listen_sock_, extra_listen_events_ | EPOLLIN))
    {
        return false;
    }

    if (!setNonBlocking(listen_sock_))
    {
        return false;
    }

    return true;
}

bool Server::setNonBlocking(int fd)
{
    int old_fd_flags = ::fcntl(fd, F_GETFL);
    int new_fd_flags = old_fd_flags | O_NONBLOCK;
    return ::fcntl(fd, F_SETFL, new_fd_flags) == 0;
}

Server::~Server()
{
    ::close(listen_sock_);
    is_running_ = false;
}

void Server::run()
{
    while (is_running_)
    {
        // 获取即将过期定时器的剩余时间
        int timeout_ms = timer_manager_.millisecondsToNextExpired();
        int ret = p_epoller_->wait(timeout_ms);
        if (ret < 0)
        {
            // 被信号打断后继续执行
            if (errno == EINTR)
            {
                continue;
            }
            is_running_ = false;
        }
        for (int i = 0; i < ret; ++i)
        {
            auto fd = p_epoller_->getFdOf(i);
            auto events = p_epoller_->getEventsOf(i);

            // 监听socket可读
            if (fd == listen_sock_)
            {
                dealListen();
            }
            // 连接socket出错
            // ??? EPOLLRDHUP EPOLLERR EPOLLHUP 区别
            else if(events & EPOLLERR || events & EPOLLRDHUP || events & EPOLLHUP)
            {
                // 移除定时器
                timer_manager_.cancel(fd);
                // 关闭HTTP连接
                closeHttpConn(http_conns_.at(fd));
            }
            else if(events & EPOLLIN)
            {
                dealRead(http_conns_.at(fd));
            }
            else if (events & EPOLLOUT)
            {
                dealWrite(http_conns_.at(fd));
            }
        }
        // 检查并处理过期的定时器
        timer_manager_.checkAndHandleTimer();
    }
}

// 处理监听socket的可读事件
void Server::dealListen()
{
    do
    {
        struct sockaddr_in client_addr;
        socklen_t addrlen = sizeof(client_addr);
        int conn_sock = ::accept(listen_sock_, (struct sockaddr *)&client_addr, &addrlen);

        if (conn_sock < 0)
        {
            // todo: 添加错误处理
            // std::fprintf(stderr, "%s\n", std::strerror(errno));
            return;
        }
        // todo: 服务器并发数过大时返回的不是http报文, 不够友好
        if (http_conns_.size() >= MAX_NUMBER_HTTP_CONNS)
        {
            sendError(conn_sock, "http server busy!\n");
            return;
        }
        addHttpConn(conn_sock, client_addr);
        // 为新建立的连接添加定时器
        timer_manager_.add(conn_sock, ::time(nullptr) + CHECK_CONN_TIME_SLOT_SECONDS,
                           [this, conn_sock](){
                            closeHttpConn(http_conns_.at(conn_sock));
                           });
    } while (extra_listen_events_ & EPOLLET);
}

// 处理连接socket的可读事件
void Server::dealRead(HttpConn &http_conn)
{
    // 延迟定时器的过期时间
    extentTime(http_conn);
    // 向线程池的任务队列放入读任务
    p_thread_pool_->addTask([&](){
        onRead(http_conn);
    });
}

// 处理连接socket的可写事件
void Server::dealWrite(HttpConn &http_conn)
{
    // 延迟定时器的过期时间
    extentTime(http_conn);
    // 向线程池的任务队列中放入写任务
    p_thread_pool_->addTask([&](){
        onWrite(http_conn);
    });
}

// 添加一个Http连接实例
void Server::addHttpConn(int conn_sock, const struct sockaddr_in &client_addr)
{
    http_conns_.emplace(conn_sock, HttpConn(conn_sock, client_addr, extra_conn_events_ & EPOLLET));
    p_epoller_->addFd(conn_sock, EPOLLIN | extra_conn_events_);
    setNonBlocking(conn_sock);
}

// 关闭一个Http连接实例
void Server::closeHttpConn(HttpConn &http_conn)
{
    // fprintf(stdout, "[DEBUG]close http connection, connect socket: %d\n", http_conn.getSock());

    http_conn.close();
    p_epoller_->delFd(http_conn.getSock());
    http_conns_.erase(http_conn.getSock());
}

// 读任务
void Server::onRead(HttpConn &httpConn)
{
    if (httpConn.processRequest())
    {
        // 请求报文处理完成, 先设置响应报文, 再注册 EPOLLOUT 事件
        httpConn.setResponse();
        p_epoller_->modFd(httpConn.getSock(), extra_conn_events_ | EPOLLOUT);
    }
    else
    {
        // 请求报文没有处理完, 重新注册 EPOLLIN 事件, 防止ET模式下不会再次触发
        p_epoller_->modFd(httpConn.getSock(), extra_conn_events_ | EPOLLIN);
    }
}

// 写任务
void Server::onWrite(HttpConn &httpConn)
{
    if (httpConn.processResponse())
    {
        // 数据发完了, 重新注册 EPOLLIN 事件
        p_epoller_->modFd(httpConn.getSock(), extra_conn_events_ | EPOLLIN);
    }
    else
    {
        // 数据没发完, 重新注册 EPOLLOUT 事件, 防止ET模式下不会再次触发
        p_epoller_->modFd(httpConn.getSock(), extra_conn_events_ | EPOLLOUT);
    }
}

void Server::sendError(int sock, const std::string &msg)
{
    // * log
    // fprintf(stdout, "[INFO]server busy: %lu clients!\n", http_conns_.size());
    ::send(sock, msg.data(), msg.size(), MSG_DONTWAIT);
    ::close(sock);
}


void Server::extentTime(const HttpConn &http_conn)
{
    timer_manager_.adjustTime(http_conn.getSock(), ::time(nullptr) + CHECK_CONN_TIME_SLOT_SECONDS);
}
