#include <buffer/Buffer.h>

#include <sys/uio.h>

#include <cerrno>
#include <algorithm>

ssize_t Buffer::readFd(int fd, int *save_errno)
{
    constexpr int EXTRA_BUFFER_SIZE = 64 * 1024; // 64KiB的临时栈内存
    char extra_buffer[EXTRA_BUFFER_SIZE];  
    struct iovec iovecs[2];
    iovecs[0].iov_base = beginWrite();
    iovecs[0].iov_len = writableBytes();
    iovecs[1].iov_base = extra_buffer;
    iovecs[1].iov_len = EXTRA_BUFFER_SIZE;

    ssize_t read_len = ::readv(fd, iovecs, 2);
    if (read_len < 0)
    {
        *save_errno = errno;
    }
    // 数据全部读入buffer_
    else if (read_len <= iovecs[0].iov_len)
    {
        write_pos_ += read_len;
    }
    // 部分数据读入extra_buffer
    else
    {
        write_pos_ = buffer_.size();
        append(extra_buffer, read_len - iovecs[0].iov_len);
    }
    return read_len;
}

ssize_t Buffer::writeFd(int fd, int *save_errno)
{
    ssize_t write_len = write(fd, beginRead(), readableBytes());
    if (write_len < 0)
    {
        *save_errno = errno;
    }

    read_pos_ += write_len;
    return write_len;
}

const char *Buffer::peek() const
{
    return beginRead();
}

void Buffer::retrieve(int len)
{
    read_pos_ += len;
}

void Buffer::append(const char *ptr, int len)
{
    if (writableBytes() < len && writableBytes() + prependableBytes() >= len)
    {
        int prependable_bytes = prependableBytes();
        // 消除前面的空闲内存
        std::copy(beginRead(), beginWrite(), begin());
        read_pos_ = 0;
        write_pos_ -= prependable_bytes;
    }
    else if (writableBytes() + prependableBytes() < len)
    {
        buffer_.resize(write_pos_ + len);
    }

    std::copy(ptr, ptr + len, beginWrite());
    write_pos_ += len;
}
