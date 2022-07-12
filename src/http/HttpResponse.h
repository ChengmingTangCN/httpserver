#ifndef HTTPSERVER_HTTP_HTTP_RESPONSE_H
#define HTTPSERVER_HTTP_HTTP_RESPONSE_H

#include <buffer/Buffer.h>

#include <sys/uio.h>
#include <sys/types.h>
#include <sys/stat.h>

#include <string>
#include <unordered_map>
#include <cassert>

class HttpResponse
{

public:

    using Headers = std::unordered_map<std::string, std::string>;

public:

    HttpResponse()
    : buffer_(),
      status_code_(-1),
      status_text_(),
      http_version_("/HTTP1.1"),
      headers_(),
      file_path_(),
      iovec_arr(),
      file_addr_(nullptr)
    {}

    HttpResponse(const HttpResponse &) = default;

    HttpResponse(HttpResponse &&) = default;

    HttpResponse &operator=(const HttpResponse &) = default;

    HttpResponse &operator=(HttpResponse &&) = default;

    ~HttpResponse() = default;

public:

    void setStatusCode(int status_code)
    {
        status_code_ = status_code;
    }

    void setFilePath(const std::string &file_path)
    {
        file_path_ = file_path;
    }

    void setHttpVersion(const std::string &http_version)
    {
        http_version_ = http_version;
    }

public:

    // 生成HTTP报文将其写入缓冲区
    void init();

    // 向连接socket发送HTTP响应报文
    // 报文发送完成返回true, 否则返回false
    bool write(int conn_sock, bool is_et);

public:

    static std::unordered_map<int, std::string> code_to_text;

    static std::unordered_map<std::string, std::string> suffix_to_type;  // 文件后缀映射到响应报文content-type

private:

    void addStatusLine()
    {
        const std::string line = http_version_ + " " +
                                 std::to_string(status_code_) + " " +
                                 code_to_text[status_code_] + "\r\n";
        buffer_.append(line.data(), line.size());
    }

    void setHeader(const std::string &key, const std::string &value)
    {
        headers_[key] = value;
    }

    void addHeaders()
    {
        for (const auto &p : headers_)
        {
            const std::string header = p.first + ": " + p.second + "\r\n";
            buffer_.append(header.data(), header.size());
        }
    }

    void addCrlf()
    {
        buffer_.append("\r\n", 2);
    }

    void addContent(const std::string &content)
    {
        buffer_.append(content.data(), content.size());
    }

    std::string getHtmlString(const std::string &content)
    {
        return "<html>"
               "  <head>"
               "    <title>"
               "      Http Server"
               "    </title>"
               "  </head>"
               "  <body>"
               + content +
               "  </body>"
               "</html>";
    }

    // 统一处理异常响应(状态码不是200)
    void handleExceptStatus()
    {
        auto content = getHtmlString(code_to_text[status_code_]);
        addStatusLine();
        setHeader("Connection", "keep-alive");
        setHeader("Content-Length", std::to_string(content.size()));
        setHeader("Content-Type", "text/html");
        addHeaders();
        addCrlf();
        addContent(content);

        iovec_arr[0].iov_base = const_cast<char *>(buffer_.peek());
        iovec_arr[0].iov_len = buffer_.readableBytes();
        iovec_arr[1].iov_base = NULL;
        iovec_arr[1].iov_len = 0;
    }

    // 根据文件后缀名获取文件类型
    std::string getFileType() const
    {
        auto pos = file_path_.find_last_of('.');
        if (pos == std::string::npos)
        {
            return "";
        }
        auto suffix = file_path_.substr(pos + 1);
        if (suffix_to_type.count(suffix))
        {
            return suffix_to_type[suffix];
        }
        return "";
    }

private:

    Buffer buffer_;

    int status_code_;

    std::string status_text_;

    std::string http_version_;

    Headers headers_;

    std::string file_path_;

    struct iovec iovec_arr[2];  // iovecs[0]指向缓冲区 iovecs[1]指向需要发送的文件

    void *file_addr_; // 需要发送的文件

    struct stat file_stat_;

};

#endif
