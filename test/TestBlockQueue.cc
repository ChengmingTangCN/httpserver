#include <logger/BlockQueue.h>

#include <string>
#include <iostream>
#include <thread>

using namespace std;

void produce(BlockQueue<string> &block_queue)
{
    for (int i = 1; i <= 30; ++i)
    {
        block_queue.put(to_string(i));
        cout << "put into queue: " << i << endl;
        cout << block_queue.size() << " " << block_queue.capacity() << endl;
    }
    this_thread::sleep_for(std::chrono::seconds(1));
    block_queue.stop();
}

void consume(BlockQueue<string> &block_queue)
{
    while (true)
    {
        string str;
        block_queue.take(str);
        if (block_queue.stopped())
        {
            break;
        }
        cout << "take from queue: " << str << endl;
    }
}

int main()
{
    BlockQueue<string> block_queue(10);

    vector<thread> producers(1);
    vector<thread> consumers(5);
    for (int i = 0; i < 3; ++i)
    {
        producers[i] = thread(produce, ref(block_queue));
    }
    for (int i = 0; i < 5; ++i)
    {
        consumers[i] = thread(consume, ref(block_queue));
    }

    for (int i = 0; i < 3; ++i)
    {
        producers[i].join();
    }
    for (int i = 0; i < 5; ++i)
    {
        consumers[i].join();
    }

    return 0;
}