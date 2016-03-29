#Netty


## EventLoopGroup 

用于管理一组线程。 这里以NioEventLoopGroup为例。

主要类的继承关系如下：
```java
MultithreadEventExecutorGroup
+children:EventExecutor[]  ;//SingleThreadEventExecutor类型，实际上是NioEventLoop类型
|
MultithreadEventLoopGroup
-ChannelFuture register(Channel channel, ChannelPromise promise) 
|
NioEventLoopGroup
```
创建时可以指定线程数，如果不指定，按照如下方式生成。
```java
 Math.max(1, SystemPropertyUtil.getInt("io.netty.eventLoopThreads", Runtime.getRuntime().availableProcessors() * 2));
```
* 内部有个children的数组成员，用于保存创建的线程，线程创建见`MultithreadEventExecutorGroup`的构造方法。
* 提供了两种方法提交任务，在内部会选择其中一个EventLoop进行提交，如下。

```java
public ChannelFuture register(Channel channel)
public Future<?> submit(Runnable task)
```

## EventLoop

执行任务的具体线程。这里以NioEventLoop为例。

* 一个EventLoop管理多个channel，并且这种关系是固定的
* 如果使用NioEventLoopGroup，child类型为NioEventLoop。

NioEventLoop类继承关系如下：
```java
SingleThreadEventExecutor
+taskQueue:Queue<Runnable> 
+thread:Thread //线程的执行逻辑调用NioEventLoop.run(),构造函数中初始化
-public Future<?> shutdownGracefully(long quietPeriod, long timeout, TimeUnit unit) 
-public void execute(Runnable task) //启动thread
|
SingleThreadEventLoop
-ChannelFuture register(Channel channel)
|
NioEventLoop
```
* `EventLoopGroup.submit(Runnable r)`提交task，最后会调用`EventLoop.execute()`来提交。
* 提交时会使用`EventLoop.inEventLoop()`来判断，如果当前是Loop线程立即执行，否则放到taskQueue队列；   
* `register(Channel channel)`同理，用于注册Channel，如果非Loop线程，会封装成一个task提交，具体逻辑可以见：

```java
//AbstractChannel
public final void register(EventLoop eventLoop, final ChannelPromise promise) 
```

__EventLoop内部线程如何启动__：在调用 `excute()`提交task时，会触发线程`start()`。


## ChannelPipeline

用于处理channel上的事件。

* 每个Channel中都会有一个成员pipeline
* 默认类型为DefaultChannelPipeline，在channel构造函数中初始化。

```java
pipeline = new DefaultChannelPipeline(this);
```
#### pipeline双向链表结构        
pipeline中有有一个`AbstractChannelHandlerContext`(如果要在pipeline上保存私有数据见`示例`部分)类型构成的双向链表，代表对事件的处理逻辑，默认链表中有2个元素，如下。

```java
class DefaultChannelPipeline{

    final AbstractChannelHandlerContext head;//指向链表头
    final AbstractChannelHandlerContext tail;//指向链表尾
    final AbstractChannel channel;//当前的channel

}
//最开始会添加2个默认的context
head = new HeadContext(this);
tail = new TailContext(this);

```
Context目前主要使用子类`DefaultChannelHandlerContext`，具体处理逻辑由handler实现。

```java
class DefaultChannelHandlerContext{
    private final AbstractChannel channel; //当前处理的channel
    private final DefaultChannelPipeline pipeline;
    final EventExecutor executor; //当前所处的eventloop
    private final ChannelHandler handler;//具体执行的逻辑

}
```


#### pipeline 上有两种事件

* inbound(对应ChannelInboundHandler)：pipeline由head开始在链表上传递（比如tcp上读事件）
* outbound(对应ChannelOutboundHandler)：pipeline由tail开始在链表上传递（比如往tcp写数据）

```java
//inbound
    ChannelPipeline fireChannelActive();
    ChannelPipeline fireChannelInactive();
    ChannelPipeline fireExceptionCaught(Throwable cause);
    ChannelPipeline fireUserEventTriggered(Object event);
    ChannelPipeline fireChannelRead(Object msg);
    ChannelPipeline fireChannelReadComplete();
    ChannelPipeline fireChannelWritabilityChanged();

//outbound
    ChannelPipeline read()
    ChannelFuture connect(SocketAddress remoteAddress);
    ChannelFuture disconnect();
    ChannelFuture close();
    ChannelFuture deregister();
    ChannelFuture bind(SocketAddress localAddress, ChannelPromise promise);
    ChannelPipeline read();
    ChannelFuture write(Object msg);
    ChannelPipeline flush();
    ChannelFuture writeAndFlush(Object msg);
```

由pipline发起调用，最终会传递到ChannelHandler。

```java
ChannelPipeline.fireChannelUnregistered()
    ->AbstractChannelHandlerContext.fireChannelRegistered()#进行线程上下文判断
    ->AbstractChannelHandlerContext.invokeChannelRegistered()
        ->ChannelInboundHandler.channelRegistered()

```
piple上发起的调用执行逻辑会放到channel所属的线程上下文去执行，以TCP上读事件为例，如果当前不在channel对应的EventLoop中会放到taskQueue中执行。
```java
  @Override
    public ChannelHandlerContext fireChannelRead(final Object msg) {

        final AbstractChannelHandlerContext next = findContextInbound();
        //找到下一个context(inbound类型）
        EventExecutor executor = next.executor();//这里是从channel中拿到eventloop
        if (executor.inEventLoop()) {
            next.invokeChannelRead(msg);
        } else {
            executor.execute(new OneTimeTask() {
                @Override
                public void run() {
                    next.invokeChannelRead(msg);
                }
            });
        }
        return this;
    }
```

####pipeline上保存数据
因为TCP发送的数据包，不可能一次读完，需要把数据先缓冲，下次再添加解析。Netty提供的ChannelHandler，可以完成此功能。
```java
public interface ChannelHandler {
    void handlerAdded(ChannelHandlerContext ctx) throws Exception;
    void handlerRemoved(ChannelHandlerContext ctx) throws Exception;
    void exceptionCaught(ChannelHandlerContext ctx, Throwable cause) throws Exception;
}

```
*   在Hanlder定义一个私有的buffer。
*   `handlerAdded()`  创建buffer。
*   `handlerRemoved()` 释放buffer(Netty 有些buffer利用了内存池)的引用计数。在channel关闭的时候会调用此方法。


## NioServerSocketChannel

主要用来执行accept。 以这个类举例。   
如何构造pipeline成员，见`ServerBootstrap.init()`方法。   
具体逻辑为：添加了一个handler，这个handler在channel(处理accpet的channel)初始化时候，再添加一个
handler，类型为ServerBootstrapAcceptor。  
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

*  执行accept操作获取channel(处理read/write)
*  设置channel(read/write)的TCP属性
*  为channel(read/write) 添加新的handler
*  选择group并将channel(read/write) register注册到EvenLoop中。


## NioSocketChannel

主要用来处理TCP读写

* 如何构造见`NioServerSocketChannel.doReadMessages()`  
* 读Channel上数据见`NioByteUnsafe.read()`


```java
class NioSocketChannel{
    
    Unsafe unsafe;//读写channel相关的操作
    ChannelPipeline pipeline;
    EventLoop eventLoop;//所属的EventLoop
    SocketChannelConfig config;//socket配置，
}
//config 默认值为new NioSocketChannelConfig(this, socket.socket());
```

##### TCP相关参数
SocketChannelConfig 可以指定Channel相关的参数，如下：

*   Buffer相关（Netty定义的Channel层面）
    *   接收Buffer 大小策略
    *   channel相关的buffer分配器

```java
class DefaultChannelConfig implements ...{
    ByteBufAllocator allocator = ByteBufAllocator.DEFAULT;//buffer分配器
    RecvByteBufAllocator rcvBufAllocator = DEFAULT_RCVBUF_ALLOCATOR;//
}
```

从TCP读数据：读事件触发新分配一个用于读的buffer写数据，rcvBufAllocator 指定分配用于读的buffer的大小策略，有如下2种：
```
AdaptiveRecvByteBufAllocator：动态大小
FixedRecvByteBufAllocator：固定
```
往TCP写数据： 数据先写到`ChannelOutboundBuffer` buffer中，每个Channel使用固定的一个，在成员unsafe中定义并初始化。

*   Channel底层的java.net.Socket相关的参数


```java
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

如果需要修改可以调用channel.config()获取config,config提供了接口修改。

__Config如何设置channel属性:__    
见如下代码，`NioSocketChannelConfig`构造时传递了原生的socket对象，直接在对象上操作。
```
public NioSocketChannel(Channel parent, SocketChannel socket) {
        super(parent, socket);
        config = new NioSocketChannelConfig(this, socket.socket());
    }
```



## Server 关闭

一般server启动是这样的：
```java
try{
    ServerBootstrap b = new ServerBootstrap();
    ChannelFuture cf = b.bind(port).sync(); 
    cf.channel().closeFuture().sync();//会阻塞在这里，直到NioServerSocketChannel 关闭
}finally{
    workerGroup.shutdownGracefully();
    bossGroup.shutdownGracefully();
}
```
* 如果关闭NioServerSocketChannel，server就关掉了
* 可以通过NioSocketChannel.parent()获取对应的NioServerSocketChannel。

每个EventLoop里面的线程状态定义如下：

```java
int ST_NOT_STARTED = 1;
int ST_STARTED = 2;
int ST_SHUTTING_DOWN = 3;
int ST_SHUTDOWN = 4;
int ST_TERMINATED = 5;
```

* 当调用`EventLoopGroup.shutdownGracefully()`,会在每个`EventLoop`上面调用`shutdownGracefully()`。
* `EventLoop.shutdownGracefully()`会把`EventLoop`状态设置为`ST_SHUTTING_DOWN`,`EventLoop`内部的thread检查到这种状态后会关闭所有channel，并退出。
* `EventLoopGroup.shutdownGracefully()`返回一个future，用来获取关闭状态，Group在构造时，对里面的每个EventLoop设置了Listener，根据收到的回调判断是否所有Loop都已经关闭。



## 示例

* 其中ServerBootstrap中的childHanler（也就是这里的`new HttpHelloWorldServerInitializer()`），会被添加到client请求建立的连接NioSocketChannel中。
* `ChannelInitializer`是一个比较特殊的handler，这个handler在channelRegistered()方法中，会调用initChannel()
添加hanlder，同时会把自己remove掉。
* 一般给NioSocketChannel添加handler可以通过`ChannelInitializer`实现，如果要保存一些私有数据（channel级别）必须保证添加的handler都是新的实例。

```java
//HttpHelloWorldServer
public final class HttpHelloWorldServer {

    static final int PORT =//

    public static void main(String[] args) throws Exception {
        // Configure the server.
        EventLoopGroup bossGroup = new NioEventLoopGroup(1);
        EventLoopGroup workerGroup = new NioEventLoopGroup();
        try {
            ServerBootstrap b = new ServerBootstrap();
            b.option(ChannelOption.SO_BACKLOG, 1024);
            b.group(bossGroup, workerGroup)
             .channel(NioServerSocketChannel.class)
             .handler(new LoggingHandler(LogLevel.INFO))
             .childHandler(new HttpHelloWorldServerInitializer());//
            Channel ch = b.bind(PORT).sync().channel();
            ch.closeFuture().sync();
        } finally {
            bossGroup.shutdownGracefully();
            workerGroup.shutdownGracefully();
        }
    }
}

//HttpHelloWorldServerInitializer
public class HttpHelloWorldServerInitializer extends ChannelInitializer<SocketChannel> {


    @Override
    public void initChannel(SocketChannel ch) {
        ChannelPipeline p = ch.pipeline();
        p.addLast(new HttpServerCodec());
        p.addLast(new HttpHelloWorldServerHandler());
    }
}

```

## 其他 
#### AttributeMap 
Channel 都继承了AttributeMap，AttributeMap可以存储channel相关的数据。
#### Promise
Promise 构造的时候，会传递一个EventLoop进去，会在Promise异步等待执行结果时，检测线程上下文，防止在EventLoop中等待。
#### DefaultChannelGroup
提供了一组管理Channel的方法
#### 线程上线文
很多组件内部都存在一个EventLoop，很多outbound事件都会转移到pipeline上操作。  
Channel上执行write，会间接调用pipeline的write操作  
ChannelHandler 方法中，调用Context的write操作，同样会间接调用pipeline的write操作  










