#include <http/HttpRequest.h>

#include <regex>

using std::string;
using std::search;
using std::begin;
using std::end;

// 读取数据
void HttpRequest::read(int conn_sock, bool is_et)
{
    do
    {
        int save_errno = 0;
        ssize_t read_len = buffer_.readFd(conn_sock, &save_errno);
        if (read_len < 0)
        {
            // 没有数据可读, 跳出循环
            if (save_errno == EAGAIN || save_errno == EWOULDBLOCK)
            {
                break;
            }
            // todo: 如何处理其它错误
        }
        // * 对方关闭连接或者写方向的连接
        if (read_len == 0)
        {
            is_closed_ = true;
            break;
        }
    } while (is_et);
}


// todo: 解析报文主体部分存在问题, 不能用从状态机驱动
void HttpRequest::parse()
{
    // 如果请求报文解析完成或者出错退出循环
    while(!finished())
    {
        const string crlf("\r\n");
        auto buffer_end = buffer_.peek() + buffer_.readableBytes();
        auto crlf_iter = search(buffer_.peek(),
                                buffer_end,
                                begin(crlf),
                                end(crlf));

        auto findCrlf = [&]()
        {
            return crlf_iter != buffer_end;
        };

        // todo: 没有处理post方法的body
        // 1. 解析请求首行, 首部字段时遇到CRLF开始解析
        // 2. 如果连接关闭或者对端关闭写进行解析
        if ((parse_state_ != ParseState::BODY && findCrlf()) ||
            is_closed_)
        {
            // 从缓冲区中取出一行数据进行解析
            int line_len = crlf_iter - buffer_.peek() + 2;
            std::string line(buffer_.peek(), line_len);
            buffer_.retrieve(line_len);
            if (parse_state_ != ParseState::BODY && findCrlf())
            {
                // 移除末尾的 \r\n
                line.pop_back();
                line.pop_back();
            }

            // 检测到一行数据
            switch(parse_state_)
            {
                // 解析请求首行
                case ParseState::REQUESTLINE:
                {
                    parseRequestLine(line);
                    break;
                }
                // 解析请求头
                case ParseState::HEADER:
                {
                    parseHeader(line);
                    break;
                }
                // 解析请求体
                case ParseState::BODY:
                {
                    parseBody(line);
                    break;
                }
                // 请求解析完成
                case ParseState::OK:
                {
                    break;
                }
                // 错误的请求报文
                case ParseState::BAD_REQUEST:
                {
                    break;
                }
                // 未知情况
                default:
                {
                    parse_state_ = ParseState::UNKNOWN_ERROR;
                    break;
                }
            }
        }

    }
}

void HttpRequest::parseRequestLine(const std::string &line)
{
    // * 通过正则表达式解析请求首行
    std::regex pattern("^([^ ]*) ([^ ]*) HTTP/([^ ]*)$");
    std::smatch submatch;
    if (std::regex_match(line, submatch, pattern))
    {
        method_ = submatch[1];
        path_ = submatch[2];
        version_ = submatch[3];
        parse_state_ = ParseState::HEADER;
    }
    else
    {
        // 匹配失败, 说明请求报文格式有问题
        parse_state_ = ParseState::BAD_REQUEST;
    }
}

void HttpRequest::parseHeader(const std::string &line)
{
    if (line.empty())
    {
        // GET请求报文不需要解析请求体
        if (method_ == "GET")
        {
            parse_state_ = ParseState::OK;
        }
        // POST请求报文需要解析请求体
        else if (method_ == "POST")
        {
            parse_state_ = ParseState::BODY;
        }
        // 不处理其它类型的请求报文
        else {
            parse_state_ = ParseState::BAD_REQUEST;
        }
        return;
    }
    // * 通过正则表达式解析请求头
    // 没有遇到空行, 一直处于 PARSE_HEADER 状态
    std::regex patten("^([^:]*): ?(.*)$");
    std::smatch subMatch;
    if(std::regex_match(line, subMatch, patten)) {
        header_[subMatch[1]] = subMatch[2];
    }
    else
    {
        parse_state_ = ParseState::BAD_REQUEST;
    }

}

// todo: 需要修改
void HttpRequest::parseBody(const std::string &line)
{
    body_ = line;
    parse_state_ = ParseState::OK;
}
