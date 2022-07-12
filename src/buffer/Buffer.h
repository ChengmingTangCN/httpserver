// 应用层缓冲区
//
// fd -> buffer
// -> ssize_t read_len = buffer.readfd(fd, &save_errno)
//
// buffer -> user
// -> int len = buffer.readableBytes();
// -> const char *data = buffer.peek();
// -> read read_len bytes from buffer
// -> buffer.retireve(read_len);
//
// buffer -> fd
// -> ssize_t write_len = buffer.writefd(fd, &save_errno);
//
// user -> buffer
// -> buffer.append(data, len);

#ifndef HTTPSERVER_BUFFER_H
#define HTTPSERVER_BUFFER_H

#include <unistd.h>

#include <vector>

class Buffer
{
    friend class HttpRequest;
public:

    Buffer(int size = 1024)
    : buffer_(size),
      read_pos_(0),
      write_pos_(0)
      {}

    Buffer(const Buffer &) = default;

    Buffer(Buffer &&) = default;

    Buffer &operator=(const Buffer &) = default;

    Buffer &operator=(Buffer &&) = default;

    ~Buffer() = default;

    ssize_t readFd(int fd, int *save_errno);

    ssize_t writeFd(int fd, int *save_errno);

    const char *peek() const;

    void retrieve(int len);

    void append(const char *ptr, int len);

    // 缓冲区可写字节数
    int writableBytes() const
    {
        return buffer_.size() - write_pos_;
    }

    // 缓冲区可读字节数
    int readableBytes() const
    {
        return write_pos_ - read_pos_;
    }

private:

    char *begin()
    {
        return buffer_.data();
    }

    const char *begin() const
    {
        return buffer_.data();
    }

    // 写起始地址
    char *beginWrite()
    {
        return buffer_.data() + write_pos_;
    }

    // 写起始地址
    const char *beginWrite() const
    {
        return buffer_.data() + write_pos_;
    }

    // 读起始地址
    const char *beginRead() const
    {
        return buffer_.data() + read_pos_;
    }

    // 读起始地址
    char *beginRead()
    {
        return buffer_.data() + read_pos_;
    }


    // 读指针之前可用的字节数
    int prependableBytes() const
    {
        return read_pos_;
    }

private:

    std::vector<char> buffer_;  // 容器

    int read_pos_;  // 读指针

    int write_pos_; // 写指针

};

#endif
