
## NioServerSocketChannel

主要用来执行accept。 以这个类举例。   

* 设置Channel的pipeline成员，见`ServerBootstrap.init()`方法。   
* 具体逻辑为：添加了一个handler，这个handler在Channel(处理accpet的Channel，也就是自己)初始化时候会被调用，被用来再添加一个handler（类型为ServerBootstrapAcceptor）。
```java
    p.addLast(new ChannelInitializer<Channel>() {
            @Override
            public void initChannel(Channel ch) throws Exception {
                ChannelPipeline pipeline = ch.pipeline();
                ChannelHandler handler = handler();
                if (handler != null) {
                    pipeline.addLast(handler);
                }
                pipeline.addLast(new ServerBootstrapAcceptor(
                        currentChildGroup, currentChildHandler, currentChildOptions, currentChildAttrs));
            }
        });
``` 

ServerBootstrapAcceptor主要用来   

*  执行accept操作获取Channel(代表一个Client连接，用来处理read/write)
*  设置Channel(read/write)的TCP属性
*  为Channel(read/write) 添加新的handler
*  选择Group并将Channel(read/write) register注册到EvenLoop中。


## NioSocketChannel

主要用来处理TCP读写

* 如何构造见`NioServerSocketChannel.doReadMessages()`  
* 读Channel上数据见`NioByteUnsafe.read()`


```java
class NioSocketChannel{
    
    Unsafe unsafe;//读写JDK Channel相关的操作
    ChannelPipeline pipeline;
    EventLoop eventLoop;//所属的EventLoop
    SocketChannelConfig config;//socket配置，
}
//config 默认值为new NioSocketChannelConfig(this, socket.socket());
```

### Channel相关参数
SocketChannelConfig 可以指定Channel相关的参数，如下：

#### Buffer

这里指Netty定义的Channel用到的buffer。主要涉及到2块。

*   接收Buffer 大小策略 （分配大小 见 **RecvByteBufAllocator**）
*   channel相关的buffer分配器 (buffer底层实现 见 ** ByteBufAllocator **)

```java
class DefaultChannelConfig implements ...{
    ByteBufAllocator allocator = ByteBufAllocator.DEFAULT;//buffer分配器
    RecvByteBufAllocator rcvBufAllocator = DEFAULT_RCVBUF_ALLOCATOR;//
}
```

**从TCP读数据**


读事件触发新分配一个buffer用于保存读到的数据（每次读都会新分配一个），rcvBufAllocator 指定分配用于读的buffer的大小策略，有两种策略。之所以存在大小分配策略，是因为   在读数据的时候，有可能当前不能读完，需要新申请buffer，如果指定动态分配，每次会记下当次buffer使用情况，
用于对下一次buffer大小策略（见NioByteUnsafe.read()）。   这里的大小指的是buffer的initialCapacity参数。   
见 `RecvByteBufAllocator`的2个子类

```java
AdaptiveRecvByteBufAllocator：动态大小
FixedRecvByteBufAllocator：固定
```


** 往TCP写数据 **

 数据先写到`ChannelOutboundBuffer` buffer中（每个Channel使用固定的一个，并且生命周期只使用一个），在成员unsafe中定义并初始化。

一般调用write写buffer时候，**写到ChannelOutboundBuffer之前会判断是否是DirectBuffer，如果不是会进行转化**，
见`AbstractNioByteChannel.filterOutboundMessage`。

** buffer底层实现 **

有2个维度，一个是 heap/direct ，一个是 pooled/unpooled。

`ByteBufAllocator` 类下面有2个子类:
```java
UnpooledByteBufAllocator
PooledByteBufAllocator
```
基于内存池的buffer使用threadlocal来实现，所以跨线程传递 buffer的时候容易出现内存泄露（`http://www.infoq.com/cn/articles/netty-version-upgrade-history-thread-part`）。

#####  socket相关的参数


```java
//见DefaultSocketChannelConfig.java
 public <T> boolean setOption(ChannelOption<T> option, T value) {
        validate(option, value);
        if (option == SO_RCVBUF) {
            setReceiveBufferSize((Integer) value);
        } else if (option == SO_SNDBUF) {
            setSendBufferSize((Integer) value);
        } else if (option == TCP_NODELAY) {
            setTcpNoDelay((Boolean) value);
        } else if (option == SO_KEEPALIVE) {
            setKeepAlive((Boolean) value);
        } else if (option == SO_REUSEADDR) {
            setReuseAddress((Boolean) value);
        } else if (option == SO_LINGER) {
            setSoLinger((Integer) value);
        } else if (option == IP_TOS) {
            setTrafficClass((Integer) value);
        } else if (option == ALLOW_HALF_CLOSURE) {
            setAllowHalfClosure((Boolean) value);
        } else {
            return super.setOption(option, value);
        }
        return true;
    }
```
#####  应用相关的参数

其中有部分和Buffer相关。

```java
//见DefaultChannelConfig.java
    public <T> boolean setOption(ChannelOption<T> option, T value) {
        validate(option, value);

        if (option == CONNECT_TIMEOUT_MILLIS) {
            setConnectTimeoutMillis((Integer) value);
        } else if (option == MAX_MESSAGES_PER_READ) {
            setMaxMessagesPerRead((Integer) value);
        } else if (option == WRITE_SPIN_COUNT) {
            setWriteSpinCount((Integer) value);
        } else if (option == ALLOCATOR) {
            setAllocator((ByteBufAllocator) value);
        } else if (option == RCVBUF_ALLOCATOR) {
            setRecvByteBufAllocator((RecvByteBufAllocator) value);
        } else if (option == AUTO_READ) {
            setAutoRead((Boolean) value);
        } else if (option == AUTO_CLOSE) {
            setAutoClose((Boolean) value);
        } else if (option == WRITE_BUFFER_HIGH_WATER_MARK) {
            setWriteBufferHighWaterMark((Integer) value);
        } else if (option == WRITE_BUFFER_LOW_WATER_MARK) {
            setWriteBufferLowWaterMark((Integer) value);
        } else if (option == MESSAGE_SIZE_ESTIMATOR) {
            setMessageSizeEstimator((MessageSizeEstimator) value);
        } else {
            return false;
        }

        return true;
    }
```

WRITE_BUFFER_HIGH_WATER_MARK：如果当前buffer中的数据超了，可以通过AbstractChannel.isWritable()进行判断是否继续写数据。

#### 如何设置Channel参数

如果需要修改可以调用channel.config()获取config,config提供了接口修改。
Config如何设置channel属性，见如下代码，`NioSocketChannelConfig`构造时传递了原生的socket对象，直接在对象上操作。
```java
public NioSocketChannel(Channel parent, SocketChannel socket) {
        super(parent, socket);
        config = new NioSocketChannelConfig(this, socket.socket());
    }
```

另外一种方式是在ServerBootstrap中设置。
```java
ServerBootstrap b = new ServerBootstrap();
b.childOption(ChannelOption.SO_KEEPALIVE, true);//设置ServerSocketChannel相关参数
b.childOption(ChannelOption.ALLOCATOR,PooledByteBufAllocator.DEFAULT);
```
