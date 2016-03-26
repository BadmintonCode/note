#Netty


### EventLoopGroup 

用于管理一组线程。 这里以NioEventLoopGroup为例，主要类的继承关系如下：
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

### EventLoop
执行任务的具体线程

* 一个EventLoop管理多个channel，并且这种关系是固定的
* 如果使用NioEventLoopGroup，child类型为NioEventLoop。

NioEventLoop类层次关系如下
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
AbstractChannel
public final void register(EventLoop eventLoop, final ChannelPromise promise) 
```

* 在调用 `excute()`提交task时，会触发线程`start()`。


### ChannelPipeline

用于处理channel上的事件。

* 每个Channel中都会有一个成员pipeline
* 默认类型为DefaultChannelPipeline，在channel构造函数中初始化。

```java
pipeline = new DefaultChannelPipeline(this);
```
pipeline中有有一个`AbstractChannelHandlerContext`(`每个channel对应的context链表都是在channel创建的时候new出来的，所以并不会被其他channel共享，其中的handler除外`)类型构成的双向链表，代表对事件的处理逻辑，默认链表中有2个元素，如下。

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
目前主要使用子类DefaultChannelHandlerContext，具体处理逻辑由handler实现。

```java
class DefaultChannelHandlerContext{
    private final AbstractChannel channel; //当前处理的channel
    private final DefaultChannelPipeline pipeline;
    final EventExecutor executor; //当前所处的eventloop
    private final ChannelHandler handler;//具体执行的逻辑

}
```
以TCP上读事件为例，如果当前不在channel对应的EventLoop中会放到taskQueue中执行。
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

pipeline 上有两种事件   

* inbound(见ChannelInboundHandler)：pipeline由head开始在链表上传递（比如tcp上读事件）
* outbound(见ChannelOutboundHandler)：pipeline由tail开始在链表上传递（比如往tcp写数据）


### NioServerSocketChannel

以这个类举例，主要用来执行accept。   
如何构造pipeline成员，见`ServerBootstrap.init()`方法。具体逻辑为：添加了一个handler，这个handler在channel(处理accpet的channel)初始化时候，再添加一个
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
ServerBootstrapAcceptor主要用来执行accept操作，设置channel(处理read/write)的TCP属性，
为channel 添加新的handler，选择group并将channel register注册到EvenLoop中。


### NioSocketChannel


处理TCP读写

* 如何构造见`NioServerSocketChannel.doReadMessages()`  
* 读channel上数据见`NioByteUnsafe.read()`

```java
class NioSocketChannel{
    
    Unsafe unsafe;//读写channel相关的操作
    ChannelPipeline pipeline;
    EventLoop eventLoop;
    SocketChannelConfig config;//socket配置，
}
//config 默认值为new NioSocketChannelConfig(this, socket.socket());
```


SocketChannelConfig 可以指定接收buffer 大小策略，channel相关的buffer分配器。
如果需要修改可以调用channel.config()获取config再修改。
```java
class DefaultChannelConfig implements ...{
ByteBufAllocator allocator = ByteBufAllocator.DEFAULT;//buffer分配器
RecvByteBufAllocator rcvBufAllocator = DEFAULT_RCVBUF_ALLOCATOR;//
}
```
* rcvBufAllocator 指定分配接收buffer时，大小策略，有如下2种：
    * AdaptiveRecvByteBufAllocator：动态大小
    * FixedRecvByteBufAllocator：固定


* 每次可读时，都是新分配一个用于读的buffer写数据。
* 写buffer 使用的类型为ChannelOutboundBuffer，每个Channel使用固定的一个，在成员unsafe中定义并初始化。



### server 关闭

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














