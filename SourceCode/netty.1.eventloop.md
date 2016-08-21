netty.4.x nio 部分代码

## EventLoopGroup 

用于管理一组线程（即EventLoop）。 这里以NioEventLoopGroup为例。

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
* 提供了两种方法提交任务，Group内部会选择其中一个EventLoop进行提交，如下。

```java
public ChannelFuture register(Channel channel)
public Future<?> submit(Runnable task)
```

## EventLoop

执行任务的具体线程，主要是一些IO任务（accept/read/write/close/shutdown）。这里以NioEventLoop为例。

* 一个EventLoop管理多个Channel，并且这种关系建立好了以后是固定的。
* 如果使用NioEventLoopGroup，child类型为NioEventLoop。
* 一个Channel相关的操作，都由对应的EventLoop处理，避免并发。

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
 `submit(Runnable r)`提交task，最后会调用`EventLoop.execute()`来提交。
 提交时会使用__EventLoop.inEventLoop()__来判断，如果当前是Loop线程立即执行，否则放到taskQueue队列；   
 `register(Channel channel)`同理，用于注册Channel，如果非Loop线程，会封装成一个task提交，具体逻辑可以见：

```java
//AbstractChannel.java
public final void register(EventLoop eventLoop, final ChannelPromise promise) 
```

__EventLoop内部线程如何启动__：在调用 `excute()`提交task时，会触发线程`start()`。
