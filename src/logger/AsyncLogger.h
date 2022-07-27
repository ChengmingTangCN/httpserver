#ifndef HTTPSERVER_ASYNC_LOGGER_H
#define HTTPSERVER_ASYNC_LOGGER_H

#include <logger/BlockQueue.h>

#include <string>
#include <thread>

enum class LogLevel
{
    DEBUG,
    INFO,
    WARN,
    ERROR,
    FATAL
};

class AsyncLogger
{
public:
    AsyncLogger(const AsyncLogger &) = delete;

    AsyncLogger(AsyncLogger &&) = delete;

    AsyncLogger &operator=(const AsyncLogger &) = delete;

    AsyncLogger &operator=(AsyncLogger &&) = delete;

    ~AsyncLogger()
    {
        block_queue_.stop();
        if (worker_.joinable())
        {
            worker_.join();
        }
        if (fp_)
        {
            fclose(fp_);
        }
    }

public:
    // 获取 AsyncLogger 的唯一单例
    static AsyncLogger &getInstance()
    {
        static AsyncLogger instance;
        return instance;
    }

    // 初始化logger
    void init(const std::string &log_path, const std::string &log_filename,
              int file_max_line, int block_queue_size);

    // 输出日志
    void log(LogLevel level, const char *fmt, ...);

private:
    AsyncLogger()
      : log_path_(),
        log_suffix_(),
        log_file_max_lineno_(0),
        block_queue_(),
        fp_(nullptr),
        day_lineno_(1),
        worker_(),
        lock_()
    {}

    // 异步线程执行函数
    void threadFunc();

    void openLogfile();

    int appendTimestamp(char *start, int len, const struct tm &log_tm);

    int appendLogLevel(char *start, int len, LogLevel level);


private:
    static constexpr int LOG_FILE_MAX_LINE_LEN = 4096;
    static constexpr int STR_TIME_LEN = 50;

    std::string log_path_;           // 日志文件存放路径
    std::string log_suffix_;         // 日志文件后缀名
    int log_file_max_lineno_;        // 单个日志文件最多存放的日志记录数目
    BlockQueue<std::vector<char>> block_queue_;      // 存放需要写入磁盘的日志文件
    struct tm now_tm_;                     // 当前时间
    FILE *fp_;                             // 日志文件指针
    int day_lineno_;                       // 当天的下一条日志序号
    std::thread worker_;                   // 从阻塞队列取数据写入磁盘
    mutable std::mutex lock_;              // 互斥锁
};

#define LOG_DEBUG(fmt, ...) AsyncLogger::getInstance().log(LogLevel::DEBUG, fmt, ## __VA_ARGS__);
#define LOG_INFO(fmt, ...) AsyncLogger::getInstance().log(LogLevel::INFO, fmt, ## __VA_ARGS__);
#define LOG_WARN(fmt, ...) AsyncLogger::getInstance().log(LogLevel::WARN, fmt,  __VA_ARGS__);
#define LOG_ERROR(fmt, ...) AsyncLogger::getInstance().log(LogLevel::ERROR, fmt, ## __VA_ARGS__);
#define LOG_FATAL(fmt, ...) AsyncLogger::getInstance().log(LogLevel::FATAL, fmt, ## __VA_ARGS__);

#endif
