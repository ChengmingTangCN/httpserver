#include <http/HttpResponse.h>
#include <logger/AsyncLogger.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/fcntl.h>
#include <sys/mman.h>
#include <unistd.h>
#include <cerrno>
#include <cstring>

#include <cstdio>

using std::string;
using std::unordered_map;

unordered_map<int, string> HttpResponse::code_to_text = {
    {200, "OK"},
    {400, "Bad Request"},
    {403, "Forbidden"},
    {404, "Not Found"},
    {500, "Internal Server Error"},
};

unordered_map<string, string> HttpResponse::suffix_to_type = {
    {"html", "text/html"},
    {"jpeg", "image/jpeg"},
    {"jpg", "image/jpg"},
    {"png", "image/png"}
};

// 生成HTTP报文将其写入缓冲区
void HttpResponse::init()
{
    while (true)
    {
        if (status_code_ != 200)
        {
            break;
        }
        int fd = ::open(file_path_.data(), O_RDONLY);
        if (fd < 0)
        {
            status_code_ = 400;
            break;
        }
        fstat(fd, &file_stat_);

        if(!S_ISREG(file_stat_.st_mode))
        {
            status_code_ = 400;
            break;
        }
        file_addr_ = ::mmap(nullptr, file_stat_.st_size,
                            PROT_READ, MAP_PRIVATE, fd, 0);
        if (file_addr_ == MAP_FAILED)
        {
            ::perror("::mmap()");
            status_code_ = 500;
            break;
        }
        ::close(fd);

        addStatusLine();
        setHeader("Content-Length", std::to_string(file_stat_.st_size));
        setHeader("Content-Type", getFileType());
        setHeader("Connection", keep_alive_ ? "keep-alive" : "close");
        addHeaders();
        addCrlfLine();

        iovec_arr[0].iov_base = const_cast<char *>(buffer_.peek());
        iovec_arr[0].iov_len = buffer_.readableBytes();
        iovec_arr[1].iov_base = file_addr_;
        iovec_arr[1].iov_len = file_stat_.st_size;
        return;
    }
    // * 请求没有成功, 生成异常响应报文
    handleExceptStatus();
}

// 向连接socket发送HTTP响应报文
bool HttpResponse::write(int conn_sock, bool is_et)
{
    do
    {
        ssize_t write_len = ::writev(conn_sock, iovec_arr, 2);
        if (write_len < 0)
        {
            if (errno == EWOULDBLOCK || errno == EAGAIN)
            {
                break;
            }
            LOG_ERROR("client:%d writev(): %s", conn_sock, strerror(errno));
            return true;
        }
        // 更新 iovec_arr[0]
        auto buffer_len = iovec_arr[0].iov_len;
        if (iovec_arr[0].iov_len > 0)
        {
            if (write_len >= iovec_arr[0].iov_len)
            {
                buffer_.retrieve(buffer_len);
                iovec_arr[0].iov_len = 0;
                iovec_arr[0].iov_base = nullptr;
            }
            else
            {
                buffer_.retrieve(write_len);
                iovec_arr[0].iov_len -= write_len;
                iovec_arr[0].iov_base = const_cast<char *>(buffer_.peek());
            }
        }
        // 更新 iovec_arr[1]
        if (buffer_len < write_len)
        {
            iovec_arr[1].iov_len  -= write_len - buffer_len;
            iovec_arr[1].iov_base = static_cast<char *>(iovec_arr[1].iov_base) +
                                    write_len - buffer_len;
            if (iovec_arr[1].iov_len == 0)
            {
                munmap(file_addr_, file_stat_.st_size);
                file_addr_ = nullptr;
            }
        }
        if (iovec_arr[0].iov_len == 0 && iovec_arr[1].iov_len == 0)
        {
            // 数据全部发送完
            return true;
        }
    } while (is_et);
    return false;
}

void HttpResponse::handleExceptStatus()
{
    auto content = getHtmlString(code_to_text[status_code_]);
    addStatusLine();
    setHeader("Connection", keep_alive_ ? "keep-alive" : "close");
    setHeader("Content-Length", std::to_string(content.size()));
    setHeader("Content-Type", "text/html");
    addHeaders();
    addCrlfLine();
    addContent(content);

    iovec_arr[0].iov_base = const_cast<char *>(buffer_.peek());
    iovec_arr[0].iov_len = buffer_.readableBytes();
    iovec_arr[1].iov_base = nullptr;
    iovec_arr[1].iov_len = 0;
}

// 根据文件后缀名获取文件类型
string HttpResponse::getFileType() const
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

// 生成简单的HTML页面
std::string HttpResponse::getHtmlString(const std::string &content)
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
