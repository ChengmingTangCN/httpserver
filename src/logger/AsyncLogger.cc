#include "logger/AsyncLogger.h"

#include <sys/stat.h>
#include <sys/types.h>

#include <cstdio>
#include <ctime>
#include <cerrno>
#include <cstring>
#include <thread>
#include <string>
#include <mutex>
#include <vector>
#include <cstdarg>

using std::thread;
using std::string;
using std::lock_guard;
using std::mutex;
using std::vector;
using std::min;
using std::to_string;

void AsyncLogger::init(LogLevel filter_level, const std::string &log_path,
                       const std::string &log_suffix, int file_max_line, int block_queue_size)
{
    opened_ = true;
    filter_level_ = filter_level;

    log_path_ = log_path;
    log_suffix_ = log_suffix;
    log_file_max_lineno_ = file_max_line;
    block_queue_.setCapactity(block_queue_size);

    // 获取当前时间
    time_t now = time(nullptr);
    auto ret = localtime_r(&now, &now_tm_);
    if (ret == nullptr)
    {
        fprintf(stderr, "localtime_r(): %s\n", strerror(errno));
    }

    // 打开日志文件
    openLogfile();

    // 创建异步写入日志数据的线程
    worker_ = thread(&AsyncLogger::threadFunc, this);
}

// 输出日志
void AsyncLogger::log(LogLevel level, const char *fmt, ...)
{
    if(!opened_ || level < filter_level_)
    {
        return;
    }
    time_t now = time(nullptr);
    struct tm log_tm;
    auto ret = localtime_r(&now, &log_tm);
    if (ret == nullptr)
    {
        fprintf(stderr, "localtime_r(): %s\n", strerror(errno));
    }

    vector<char> line(LOG_FILE_MAX_LINE_LEN + 2);
    int start = 0;
    start += appendTimestamp(line.data() + start, LOG_FILE_MAX_LINE_LEN - start, log_tm);
    start += appendLogLevel(line.data() + start, LOG_FILE_MAX_LINE_LEN - start, level);
    va_list args;
    va_start(args, fmt);
    int len = vsnprintf(line.data() + start, LOG_FILE_MAX_LINE_LEN- start, fmt, args);
    if (len >= LOG_FILE_MAX_LINE_LEN - start)
    {
        fprintf(stderr, "vsnprintf(): output was truncated\n");
        start = LOG_FILE_MAX_LINE_LEN;
    }
    else
    {
        start += len;
    }
    strncat(line.data() + start, "\n\0", 2);
    start += 2;
    va_end(args);
    line.resize(start);
    line.shrink_to_fit();

    block_queue_.put(std::move(line));
}
   
int AsyncLogger::appendTimestamp(char *start, int len, const struct tm &log_tm)
{
    char str_time[STR_TIME_LEN];
    int write_len = strftime(str_time, STR_TIME_LEN, "%Y-%m-%d %H:%M:%S ", &log_tm);
    strncpy(start, str_time, len);
    return min(write_len, len);
}

int AsyncLogger::appendLogLevel(char *start, int len, LogLevel level)
{
    int write_len = 0;
    const char *level_str = nullptr;
    switch(level)    
    {
        case LogLevel::DEBUG:
        {
            level_str = "[DEBUG] : ";
            break;
        }
        case LogLevel::INFO:
        {
            level_str = "[INFO] : ";
            break;
        }
        case LogLevel::WARN:
        {
            level_str = "[WARN] : ";
            break;
        }
        case LogLevel::ERROR:
        {
            level_str = "[ERROR] : ";
            break;
        }
        case LogLevel::FATAL:
        {
            level_str = "[FATAL] : ";
            break;
        }
        default:
            break;
    }
    assert(level_str);
    strncpy(start, level_str, len);
    return strlen(level_str);
}

void AsyncLogger::threadFunc()
{
    vector<char> line;
    while (true)
    {
        block_queue_.take(line);
        if (line.size() > 0)
        {
            time_t now = time(nullptr);
            struct tm log_tm;
            auto ret = localtime_r(&now, &log_tm);
            if (ret == nullptr)
            {
                fprintf(stderr, "localtime_r(): %s\n", strerror(errno));
            }
            if (log_tm.tm_year > now_tm_.tm_year || log_tm.tm_yday > now_tm_.tm_yday)
            {
                // 新的一天需要分割日志
                day_lineno_ = 1;
                now_tm_ = log_tm;
                openLogfile();
            }
            if (day_lineno_ > 1 && day_lineno_ % log_file_max_lineno_ == 1)
            {
                // 单个文件放不下
                now_tm_ = log_tm;
                openLogfile();
            }

            fputs(line.data(), fp_);
            fflush(fp_);
            line.clear();

            ++day_lineno_;
        }
        else
        {
            break;
        }
    }
}

void AsyncLogger::openLogfile()
{
    // 打开日志文件
    char str_time[STR_TIME_LEN] = {};
    int len = strftime(str_time, STR_TIME_LEN, "%Y_%m_%d-%H_%M_%S", &now_tm_);
    if (len == 0)
    {
        fprintf(stderr, "strftime(): error\n");
    }

    string log_filename = log_path_ + "/" + string(str_time) + "-" +
                          to_string(day_lineno_ / log_file_max_lineno_) + log_suffix_;
    if (fp_)
    {
        fclose(fp_);
    }
    fp_ = fopen(log_filename.data(), "w");
    if (fp_ == nullptr)
    {
        mkdir(log_path_.data(), 0755);
        fp_ = fopen(log_filename.data(), "w");
        if (!fp_)
        {
            fprintf(stderr, "fopen(): %s(%s)\n", strerror(errno), log_filename.data());
        }
    }
}