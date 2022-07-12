#ifndef HTTPSERVER_HTTP_CONN_H
#define HTTPSERVER_HTTP_CONN_H

#include <http/HttpRequest.h>
#include <http/HttpResponse.h>

#include <arpa/inet.h>

class HttpConn
{

public:

    HttpConn(int conn_sock, const struct sockaddr_in &client_addr, bool is_et)
    : conn_sock_(conn_sock),
      client_addr_(client_addr),
      is_et_(is_et),
      request_()
    {}

    HttpConn(const HttpConn &) = default;

    HttpConn(HttpConn &&) = default;

    HttpConn &operator=(const HttpConn &) = default;

    HttpConn &operator=(HttpConn &&) = default;

    ~HttpConn() = default;

public:

    // * EPOLLIN触发
    // 读数据到缓冲区, 并进行解析
    // 1. 请求报文解析成功, 将响应写入缓冲区, 返回true
    // 2. 请求报文解析失败, 将响应写入缓冲区, 返回true
    // 3. 请求报文没有解析完, 返回false
    bool processRequest();

    // 根据请求报文生成对应的HTTP响应报文
    void setResponse();

    // * EPOLLOUT触发
    // 发送缓冲区中的数据
    // 1. 缓冲区中的数据发送完, 返回true
    // 2. 没有发送完, 返回false
    bool processResponse();

    // 关闭连接
    void close();

    // 从http连接获取socket
    int getSock() const
    {
      return conn_sock_;
    }

private:

    int conn_sock_;  // 连接socket
    struct sockaddr_in client_addr_; // 客户端的socket地址

    bool is_et_; // epoll是否工作在ET模式

    HttpRequest request_;

    HttpResponse response_;

};

#endif
