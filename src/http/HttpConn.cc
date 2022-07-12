#include "http/HttpRequest.h"
#include <http/HttpConn.h>

#include <cassert>

// 读取数据到缓冲区, 并根据缓冲区中的数据解析报文
bool HttpConn::processRequest()
{
    request_.read(conn_sock_, is_et_);
    request_.parse();
    // 请求报文解析完成
    if (request_.finished())
    {
       return true;
    }
    // 请求报文没有解析完成
    else
    {
        return false;
    }
}

// 根据请求报文生成对应的HTTP响应报文
void HttpConn::setResponse()
{
    assert(request_.finished());

    response_.setHttpVersion("HTTP/1.1");
    // 错误的请求报文
    if (request_.badRequest())
    {
        response_.setStatusCode(400);
    }
    // 服务器处理出错
    else if (request_.unknownError())
    {
        response_.setStatusCode(500);
    }
    else
    {
        response_.setStatusCode(200);
        // 请求报文没有问题, 则由Response生成响应
        // ! 如何保证 request_.filepath() 不会通过 .. 访问上级目录
        response_.setFilePath("/home/cmtang/project/httpserver/resources" + request_.filePath());
    }
    // 准备好写入
    response_.init();

}

bool HttpConn::processResponse()
{
    if (response_.write(conn_sock_, is_et_))
    {
        request_.reset();
        return true;
    }
    return false;
}

// 关闭连接
void HttpConn::close()
{
    ::close(conn_sock_);
}
