## HTTP Server

### 问题记录

#### 利用webbench进行压测时，发现服务器进程会异常终止。

工作线程通过`HttpConn &`使用事件处理线程保存的HTTP连接实例，如果IO线程从容器中删除某个HTTP连接实例后会导致工作线程持有空悬引用。通过`shared_ptr`让工作线程以及事件处理线程共享HTTP实例，通过引用计数防止空悬引用。

#### 利用webbench进行压测时，发现bytes不为0，其它指标均为0.

webbench只有发现服务器关闭连接后或者出错才会统计请求次数，如果服务器发送完数据后不主动关闭连接则会出现该情况。

#### 计时器堆断言出错。

`HttpConn`对象销毁时调用回调函数时会取消计时器、关闭socket、取消epoll对它的监视，这导致计时器的使用出现了竞争，因此需要通过互斥锁进行同步。

#### 部署在其他主机上时无法正常访问文件，一直返回"Bad Request"。

`HttpConn.cc`中使用的是固定资源目录(`/home/cmtang/project/httpserver/resources`)。



