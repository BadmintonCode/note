##muduo

### 写数据
___
写数据分2种。  
一种是当前直接写入socket，并不关注写事件;   
二种是异步写，把数据写入buffer，关注写事件，触发回调从buffer往socket写入数据，成功后取消写事件。

#### 主动写数据

```
TcpConnection::send()
    TcpConnection::sendInLoop()
```

* 1.如果不在EventLoop线程中，把调用TcpConnection::sendInLoop放到EventLoop的队列中异步处理，退出。
  如果在EventLoop线程中，直接调用TcpConnection::sendInLoop进入2。
* 2.如果当前Channel没有关注写事件`!Channel::isWriting()`，并且OutputBuffer数据为空。
    * 2.1直接调用write()写数据，
    * 2.2如果写完，放入一个WriteCompleteCallback调用到EventLoop的队列。
* 3.如果当前要写的数据写完，退出；如果没有写完，进入4。
* 4.如果剩下没有写入的数据长度+OutputBuffer中待写数据长度 > highWaterMark_
  放入一个HighWaterMarkCallback调用到EventLoop的队列；
* 5.把没有写入的数据加到OutputBuffer；
* 6.调用`Channel::enableWriting()` 关注写事件；


#### 事件触发写数据

`TcpConnection::handleWrite()`   

* 1.如果没关注写事件，退出，如果关注了写事件，进入2。
* 2.调用write(),将OutputBuffer 中的数据写入socket.如果写完,
    * 2.1取消写关注事件`Channel::disableWriting()`。
    * 2.2放入一个WriteCompleteCallback调用到EventLoop的队列。


### 连接状态
___

主要有4种状态：`kDisconnected`, `kConnecting`, `kConnected`, `kDisconnecting`。 


* kConnecting:  `new TcpConnection()`里设置。
* kConnected:`TcpConnection::connectEstablished()`里面设置。
* kDisconnecting:`TcpConnection::shutdown()`里设置。



### 关闭连接
___

需要设置连接状态、取消关注事件、回调callback移除连接、关闭fd。    
除“关闭fd”外，其他操作都可以在`Connection.cc`中看到。
#### 关闭fd

这里使用在Connection回收时候（closeCallback中），因为没有引用，进而导致Socket被回收，Socket析构函数中关闭fd。如果提前调用close()关闭fd，可能导致新的请求利用刚回收的fd，导致数据错乱。

#### 主要函数
* shutdown()：在`kConnected`状态才能执行，设置为`kDisconnecting`，EventLoop队列中加入`shutdownInLoop()`。 此时，不能再直接调用send往EventLoop发送数据。
* shutdownInLoop():如果当前没有关注写事件，使用`shutdown(sockfd, SHUT_WR)`关闭socket写入;
* forceClose():在`kConnected`、`kDisconnecting`状态下，设置状态为`kDisconnecting`,异步调用forceCloseInLoop();
* forceCloseInLoop()：在`kConnected`、`kDisconnecting`状态下，调用`handleClose()`。
* handleClose() ：`kConnected`,`kDisconnecting` 2种状态下调用，并且`setState(kDisconnected)`  ；取消关注的任何事件；回调closeCallback   
* handleError()：无关闭操作。
* handleRead()：`read()`如果发现客户端关闭连接，调用`handleClose()`,如果发生异常，调用` 
handleError()`，空操作。
*handleWrite()：如果buffer里面的数据写完，并且当前状态是`kDisconnecting`，调用`shutdownInLoop()`;


#### 关闭流程

* 主动关闭写，两种情况   
1.shudown()->队列中写任务全部执行完,没有关注写事件->shutdownInLoop()->关闭写入   
2.shudown()->队列中写任务没有执行完，outputbuffer有数据，此时必定关注了写事件->shutdownInLoop()不关闭写入->handleWrite()发现关注了写事件，并且buffer为空，并且当前状态为"正在关闭中"，关闭写入

* 读到Client关闭写
handleRead()读取到client关闭写，直接调用handleClose()，移除connection

