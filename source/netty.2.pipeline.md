## ChannelPipeline

用于处理Channel上的事件。

* 每个Channel中都会有一个成员pipeline
* 默认类型为DefaultChannelPipeline，在channel构造函数中初始化。

```java
pipeline = new DefaultChannelPipeline(this);
```
#### pipeline双向链表结构        
pipeline中有一个`AbstractChannelHandlerContext`类型构成的双向链表，代表对事件的处理逻辑，默认链表中有2个元素，如下。

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

由pipline发起调用，最终会传递到ChannelHandler，如下：

```java
ChannelPipeline.fireChannelUnregistered()
    ->AbstractChannelHandlerContext.fireChannelRegistered()#进行线程上下文判断
    ->AbstractChannelHandlerContext.invokeChannelRegistered()
        ->ChannelInboundHandler.channelRegistered()

```
piple上发起的调用执行逻辑会放到channel所属的线程上下文去执行,以TCP上读事件为例，如果当前不在channel对应的EventLoop中会放到taskQueue中执行。

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

一般用户编写代码可以持有AbstractChannelHandlerContext，该对象的方法调用逻辑同pipeline。

####pipeline上保存数据
因为TCP发送的数据包以流的形式发送，不可能一次读到完整的应用层数据包，需要把数据先缓冲，下次再添加解析。Netty提供的ChannelHandler，可以完成此功能。
```java
public interface ChannelHandler {
    void handlerAdded(ChannelHandlerContext ctx) throws Exception;
    void handlerRemoved(ChannelHandlerContext ctx) throws Exception;
    void exceptionCaught(ChannelHandlerContext ctx, Throwable cause) throws Exception;
}

```
*   创建一个实现了ChannelHandler的子类
*   在Hanlder定义一个私有的Buffer。
*   `handlerAdded()`  创建Buffer。
*   `handlerRemoved()` 释放Buffer(本身JVM GC可以回收，但是Netty有些buffer使用内存池实现，为了保证统一)的引用计数。
*   在Channel处理话的时候，添加一个此Handler实例。

原理：在每个Channel初始化，添加Handler的时候，会调用handlerAdded()；当channel关闭的时候会移除handler，进而调用handlerRemoved()方法。   
注意：必须保证对每个新建立的Channel，设置一个新的ChannelHandler实例。
