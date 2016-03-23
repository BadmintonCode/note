#Netty


Bootstrap

EventLoopGroup


NioEventLoop

NioSocketChannel

### Bootstrap
核心启动类。
```java
AbstractBootstrap
+group:EventLoopGroup
-channelFactory
-doBind()
-initAndRegister()
|
ServerBootstrap
+childGroup:EventLoopGroup
-init(Channel channel)
```
#### bind()
调用逻辑如下：
```
bind()
    ->doBind()
        ->initAndRegister()
            ->init(channel)
```

#### initAndRegister()   
调用`channelFactory`创建一个`Channel`；   
调用`init(Channel channel)`初始化；   
并且将`channel`注册到`group`中；   

#### init(Channel channel):   
往`channel`的`pipleline`中添加一个 `ServerBootstrapAcceptor`对象。

### Acceptor
处理客服端请求上来的连接。
```java
ServerBootstrapAcceptor
+childGroup:EventLoopGroup
+childHandler:ChannelHandler
-void channelRead(ChannelHandlerContext ctx, Object channel)
```

#### channelRead()

* 客户端请求到达时候，`channelRead()`被调用，`channel`表示客户端的连接，实际类型是`Channel`   
* 将`ServerBootstrap`中的`childHandler` 添加到 `channel`的 `pipeline`
 `channel.pipeline().addLast(childHandler);`
* 将`channel`注册到`childGroup`中（`EventLoopGroup.register(Channel channel)`）


###Channel

```java
AbstractChannel
+pipeline:ChannelPipeline
+eventLoop:EventLoop
```


### EventLoopGroup 

管理一组线程，

```java
MultithreadEventExecutorGroup
+children:EventExecutor[]  ;//SingleThreadEventExecutor类型，实际上是NioEventLoop类型
|
MultithreadEventLoopGroup
-ChannelFuture register(Channel channel, ChannelPromise promise) 
|
NioEventLoopGroup
```

register():
选择一个 children数组中的元素,调用上面的register方法


### EventLoop
```java
SingleThreadEventExecutor
|
+taskQueue:Queue<Runnable> 
+thread:Thread //线程的执行逻辑调用NioEventLoop.run(),构造函数中初始化
-public void execute(Runnable task) //启动thread

SingleThreadEventLoop
-ChannelFuture register(Channel channel)
|
NioEventLoop
```

 

ChannelFuture register(final Channel channel, final ChannelPromise promise)
        channel.unsafe().register(this, promise);
设置channel中的eventLoop，注册到loop中。会进行线程状态的判断。


 


PooledByteBufAllocator:基于threadlocal进行内存分配

ioRatio:io任务执行的比例
taskQueue

ChannelConfig

DefaultChannelConfig:RecvByteBufAllocator
NioSocketChannel - NioByteUnsafe

判断handler是 inbound还是outbound
在添加时，会判断isInbound
DefaultChannelHandlerContext



