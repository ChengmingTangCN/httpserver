#include <server/Server.h>

#include <sys/epoll.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <fcntl.h>

#include <cstring>
#include <http/HttpConn.h>

using std::make_shared;
using std::shared_ptr;
using std::perror;

Server::Server(uint16_t port, int threads_num)
  : is_running_(false),
    port_(port),
    listen_sock_(-1),
    sock_to_http_(),
    p_thread_pool_(new ThreadPool(threads_num)),
    p_epoller_(new Epoller),
    timer_manager_(),
    listen_epoll_events_(0),
    conn_epoll_events_(0)
{
    // 设置额外需要监视的事件
    initSocketEvents();

    // 创建监听socket
    if (initListenSock())
    {
        is_running_ = true;
    }
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
            if (errno == EINTR)
            {
                continue;
            }
            // 未知错误终止服务器运行
            perror("::p_epoller->wait()");
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
            else if(events & EPOLLERR || events & EPOLLRDHUP || events & EPOLLHUP)
            {
                // 关闭HTTP连接
                closeHttpConn(fd);
            }
            else if(events & EPOLLIN)
            {
                dealRead(fd);
            }
            else if (events & EPOLLOUT)
            {
                dealWrite(fd);
            }
        }
        // 检查并处理过期的定时器
        timer_manager_.checkAndHandleTimer();
    }
}
void Server::initSocketEvents()
{
    listen_epoll_events_ = 0;
    conn_epoll_events_ = EPOLLRDHUP | EPOLLHUP | EPOLLONESHOT | EPOLLET;
}

bool Server::initListenSock()
{
    // 创建socket
    listen_sock_ = ::socket(AF_INET, SOCK_STREAM, 0);
    if (listen_sock_ < 0)
    {
        return false;
    }

    // 绑定地址
    struct sockaddr_in server_addr = {0};
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = ::htonl(INADDR_ANY);
    server_addr.sin_port = ::htons(port_);

    // 开启重用地址选项, 用来快速重启服务器
    int optval = 1;
    int ret = ::setsockopt(listen_sock_, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval));
    if (ret < 0)
    {
        std::perror("::setsockopt()");
        return false;
    }

    if (::bind(listen_sock_, 
               reinterpret_cast<struct sockaddr*>(&server_addr),
               sizeof(server_addr)) < 0)
    {
        std::perror("::bind()");
        return false;
    }

    // 转换成监听socket
    if (::listen(listen_sock_, 5) < 0)
    {
        std::perror("::listen()");
        return false;
    }

    // 通过 epoll 监视监听socket是否可读
    if (!p_epoller_->addFd(listen_sock_, listen_epoll_events_ | EPOLLIN))
    {
        std::perror("Epoller::addFd()");
        return false;
    }

    // 设置监听socket为非阻塞模式
    if (!setNonBlocking(listen_sock_))
    {
        std::perror("Server::setNonBlocking()");
        return false;
    }

    return true;
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
            std::perror("::accept()");
            return;
        }
        // todo: 返回的错误信息不是http报文, 不够友好
        if (sock_to_http_.size() >= MAX_NUMBER_HTTP_CONNS)
        {
            sendError(conn_sock, "http server busy!\n");
            return;
        }
        addHttpConn(conn_sock, client_addr);
    } while (listen_epoll_events_ & EPOLLET);
}


void Server::dealRead(int sock)
{
    // 延迟定时器的过期时间
    extentTime(sock);
    // 读任务保存着一份shared_ptr<HttpConn>
    auto p_http_conn = sock_to_http_.at(sock);
    p_thread_pool_->addTask([this, p_http_conn](){
        onRead(p_http_conn);
    });
}

void Server::dealWrite(int sock)
{
    // 延迟定时器的过期时间
    extentTime(sock);
    // 写任务保存着一份shared_ptr<HttpConn>
    auto p_http_conn = sock_to_http_.at(sock);
    p_thread_pool_->addTask([this, p_http_conn](){
        onWrite(p_http_conn);
    });
}

void Server::onRead(shared_ptr<HttpConn> p_http_conn)
{
    if (p_http_conn->processRequest())
    {
        // 请求报文处理完成, 先设置响应报文, 再注册 EPOLLOUT 事件
        p_http_conn->setResponse();
        p_epoller_->modFd(p_http_conn->getSock(), conn_epoll_events_ | EPOLLOUT);
    }
    else
    {
        // 请求报文没有处理完, 重新注册 EPOLLIN 事件, 防止ET模式下不会再次触发
        p_epoller_->modFd(p_http_conn->getSock(), conn_epoll_events_ | EPOLLIN);
    }
}

void Server::onWrite(shared_ptr<HttpConn> p_http_conn)
{
    if (p_http_conn->processResponse())
    {
        if (p_http_conn->keepAlive())
        {
            // 持久连接继续监视可读事件
            p_epoller_->modFd(p_http_conn->getSock(), conn_epoll_events_ | EPOLLIN);
        }
        else
        {
            // 释放服务器维护的shared_ptr<HttpConn>, 但可能不会立即调用::close关闭连接
            // 因为可读/可写事件中保存着一份shared_ptr<HttpConn>
            closeHttpConn(p_http_conn->getSock());
        }
    }
    else
    {
        // 数据没发完, 重新注册 EPOLLOUT 事件, 防止ET模式下不会再次触发
        p_epoller_->modFd(p_http_conn->getSock(), conn_epoll_events_ | EPOLLOUT);
    }
}

void Server::addHttpConn(int conn_sock, const struct sockaddr_in &client_addr)
{
    timer_manager_.add(conn_sock, ::time(nullptr) + CHECK_CONN_TIME_SLOT_SECONDS,
                       [this, conn_sock](){
                           closeHttpConn(conn_sock);
                       });
    sock_to_http_.emplace(conn_sock, make_shared<HttpConn>(conn_sock, client_addr, conn_epoll_events_ & EPOLLET));
    p_epoller_->addFd(conn_sock, EPOLLIN | conn_epoll_events_);
    sock_to_http_.at(conn_sock)->registerCloseCallBack([this, conn_sock](){
        p_epoller_->delFd(conn_sock);
        timer_manager_.cancel(conn_sock);
        ::close(conn_sock);
    });
    setNonBlocking(conn_sock);
}

void Server::closeHttpConn(int sock)
{
    sock_to_http_.erase(sock);
}

void Server::sendError(int sock, const std::string &msg)
{
    ::send(sock, msg.data(), msg.size(), MSG_DONTWAIT);
    ::close(sock);
}

void Server::extentTime(int sock)
{
    assert(sock_to_http_.count(sock) > 0);
    timer_manager_.adjustTime(sock, ::time(nullptr) + CHECK_CONN_TIME_SLOT_SECONDS);
}

bool Server::setNonBlocking(int fd)
{
    int old_fd_flags = ::fcntl(fd, F_GETFL);
    int new_fd_flags = old_fd_flags | O_NONBLOCK;
    return ::fcntl(fd, F_SETFL, new_fd_flags) == 0;
}
