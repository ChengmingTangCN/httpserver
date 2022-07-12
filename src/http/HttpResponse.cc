#include <http/HttpResponse.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/fcntl.h>
#include <sys/mman.h>
#include <unistd.h>

#include <cstdio>

std::unordered_map<int, std::string> HttpResponse::code_to_text = {
    {200, "OK"},
    {400, "Bad Request"},
    {403, "Forbidden"},
    {404, "Not Found"},
    {500, "Internal Server Error"},
};

std::unordered_map<std::string, std::string> HttpResponse::suffix_to_type = {
    {"html", "text/html"},
    {"jpeg", "image/jpeg"},
    {"jpg", "image/jpg"},
    {"png", "image/png"}
};

// 生成HTTP报文将其写入缓冲区
void HttpResponse::init()
{
    // 请求报文解析成功
    if (status_code_ == 200)
    {
        int fd = ::open(file_path_.data(), O_RDONLY);
        if (fd < 0)
        {
            // ::perror("::open()");
            // printf("%s\n", file_path_.data());
            status_code_ = 400;
        }
        else
        {
            fstat(fd, &file_stat_);

            if(!S_ISREG(file_stat_.st_mode))
            {
                status_code_ = 400;
            }
            else
            {
                file_addr_ = ::mmap(nullptr, file_stat_.st_size,
                                    PROT_READ, MAP_PRIVATE, fd, 0);
                if (file_addr_ == MAP_FAILED)
                {
                    ::perror("::mmap()");
                    status_code_ = 500;
                }

                addStatusLine();
                setHeader("Content-Length", std::to_string(file_stat_.st_size));
                auto file_type = getFileType();
                setHeader("Content-Type", file_type);
                setHeader("Connection", "keep-alive");
                addHeaders();
                addCrlf();

                iovec_arr[0].iov_base = const_cast<char *>(buffer_.peek());
                iovec_arr[0].iov_len = buffer_.readableBytes();
                iovec_arr[1].iov_base = file_addr_;
                iovec_arr[1].iov_len = file_stat_.st_size;
                ::close(fd);
                return;
            }
        }
        ::close(fd);
    }
    // * 请求没有成功, 生成格式一致的响应报文
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
            // todo: 处理其它错误
            ::perror("::writev()");
            break;
        }
        // 只发送了buffer_的数据
        if (write_len < iovec_arr[0].iov_len)
        {
            buffer_.retrieve(write_len);
            iovec_arr[0].iov_base = const_cast<char *>(buffer_.peek());
            iovec_arr[0].iov_len -= write_len;
        }
        // 发送了文件数据
        else
        {
            iovec_arr[1].iov_base = static_cast<char *>(file_addr_) + write_len - iovec_arr[0].iov_len;
            iovec_arr[1].iov_len -= write_len - iovec_arr[0].iov_len;
            buffer_.retrieve(iovec_arr[0].iov_len);
            iovec_arr[0].iov_base = nullptr;
            iovec_arr[0].iov_len = 0;
            if (iovec_arr[1].iov_len == 0)
            {
                ::munmap(iovec_arr[1].iov_base, file_stat_.st_size);
                return true;
            }
        }

    } while (is_et);
    return false;
}
