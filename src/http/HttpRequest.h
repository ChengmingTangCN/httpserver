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
      header_(),
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

    // 请求是否解析完成
    bool finished() const
    {
    return parse_state_ == ParseState::OK ||
           parse_state_ == ParseState::BAD_REQUEST ||
           parse_state_ == ParseState::UNKNOWN_ERROR;
    }

    bool badRequest() const
    {
        return parse_state_ == ParseState::BAD_REQUEST;
    }

    bool unknownError() const

    {
        return parse_state_ == ParseState::UNKNOWN_ERROR;
    }

    const std::string &filePath() const
    {
        return path_;
    }

    // 持久连接: 清空上一次请求的内容
    void reset()
    {
        parse_state_ = ParseState::REQUESTLINE;
        method_.clear();
        path_.clear();
        version_.clear();
        header_.clear();
        body_.clear();
    }

private:

    void parseRequestLine(const std::string &line);

    void parseHeader(const std::string &line);

    void parseBody(const std::string &line);

    void getHttpLine();

private:

    ParseState parse_state_;

    Buffer buffer_;  // 读缓冲区

    std::string method_;
    std::string path_;
    std::string version_;

    std::unordered_map<std::string, std::string> header_;  // 请求头
    std::string body_;  // 请求体

    bool is_closed_; // 对端关闭或者关闭了写方向
};

#endif
