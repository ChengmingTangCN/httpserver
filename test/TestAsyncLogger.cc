#include <logger/AsyncLogger.h>
#include <thread>
#include <unistd.h>

using namespace std;

void doLog()
{
    for (int i = 0; i < 10000; ++i)
    {
        LOG_DEBUG("%s:%d:%s, %s", __FILE__, __LINE__, __PRETTY_FUNCTION__, to_string(i).data());
        LOG_INFO("%s:%d:%s, %s", __FILE__, __LINE__, __PRETTY_FUNCTION__, to_string(i).data());
        LOG_WARN("%s:%d:%s, %s", __FILE__, __LINE__, __PRETTY_FUNCTION__, to_string(i).data());
        LOG_ERROR("%s:%d:%s, %s", __FILE__, __LINE__, __PRETTY_FUNCTION__, to_string(i).data());
        LOG_FATAL("%s:%d:%s, %s", __FILE__, __LINE__, __PRETTY_FUNCTION__, to_string(i).data());
    }
}

int main()
{
    auto &logger = AsyncLogger::getInstance();
    logger.init(LogLevel::DEBUG, "./log", ".log", 1000, 1024);
    thread t1(doLog), t2(doLog), t3(doLog);

    t1.join();
    t2.join();
    t3.join();

    this_thread::sleep_for(chrono::seconds(1));
    
    return 0;
}