#ifndef HTTPSERVER_HTTP_HTTP_REQUEST_H
#define HTTPSERVER_HTTP_HTTP_REQUEST_H

#include <buffer/Buffer.h>

#include <string>
#include <unordered_map>

// HTTP请求解析状态(主状态机)
enum class ParseState
{
    REQUESTLINE,   // 正在解析请求首行
    HEADER,        // 正在解析请求首部字段
    BODY,          // 正在解析请求主体
    OK,            // 请求解析正常完成
    BAD_REQUEST,   // 请求报文格式不对
    UNKNOWN_ERROR, // 意料之外的错误
};

class HttpRequest
{

public:

    HttpRequest()
      : parse_state_(ParseState::REQUESTLINE),
        buffer_(),
        method_(),
        path_(),
        version_(),
        headers_(),
        body_(),
        is_closed_(false)
    {}

    HttpRequest(const HttpRequest &) = default;

    HttpRequest(HttpRequest &&) = default;

    HttpRequest &operator=(const HttpRequest &) = default;

    HttpRequest &operator=(HttpRequest &&) = default;

    ~HttpRequest() = default;

public:
    // 读取数据
    void read(int conn_sock, bool is_et);

    // 驱动状态机执行
    void parse();

    // 持久连接: 清空上一次请求的内容
    void reset();

    // 请求文件路径
    const std::string &filePath() const { return path_; }

public:
    // 请求是否解析完成
    bool finished() const
    {
        return parse_state_ == ParseState::OK ||
               parse_state_ == ParseState::BAD_REQUEST ||
               parse_state_ == ParseState::UNKNOWN_ERROR;
    }

    // 请求报文是否有误
    bool badRequest() const { return parse_state_ == ParseState::BAD_REQUEST; }

    // 服务器出现未知错误
    bool unknownError() const { return parse_state_ == ParseState::UNKNOWN_ERROR; }

    // 请求报文是否请求持久连接
    bool keepAlive() const
    {
        return headers_.count("Connection") &&
               (headers_.at("Connection") == "keep-alive");
    }

private:
    // 解析请求首行
    void parseRequestLine(const std::string &line);

    // 解析请求头
    void parseHeader(const std::string &line);

    // 解析请求体
    void parseBody(const std::string &line);

    void getHttpLine();

private:
    ParseState parse_state_;

    Buffer buffer_;

    std::string method_;
    std::string path_;
    std::string version_;

    std::unordered_map<std::string, std::string> headers_;
    std::string body_;

    bool is_closed_;  // 对端关闭或者关闭了写方向
};

#endif
