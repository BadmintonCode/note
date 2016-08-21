

## Server 关闭

Netty上给出的server启动是这样的：
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
* 一般给NioServerSocketChannel/NioSocketChannel添加handler可以通过`ChannelInitializer`实现，如果要保存一些私有数据（channel级别）必须保证添加的handler都是新的实例。

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
##### AttributeMap 
Channel 都继承了AttributeMap，AttributeMap可以存储channel相关的数据。

##### Promise
Promise 构造的时候，会传递一个EventLoop进去，会在Promise（非EventLoop线程）等待执行结果时，进行通知。另外会检测线程上下文，防止Promise的等待逻辑发生在EventLoop中，造成永久等待。

##### DefaultChannelGroup
提供了一组管理Channel的方法

##### pipeline执行链
实现ChannelHandler时，注意是否要调用super.channelxxx(ctx)让调用在pipeline上传递下去。

##### 线程上线文
很多组件内部都存在一个EventLoop，很多outbound事件都会转移到pipeline上操作。  
Channel上执行write，会间接调用pipeline的write操作  
ChannelHandler 方法中，调用Context的write操作，同样会间接调用pipeline的write操作  

##### 设置Channel的属性
ServerBootstrap中分别添加 handler/childHandler(ChannelInitializer类型)，server会在构造channel时调用initChannel方法，可以在此方法中实现必要的逻辑。

##### ThreadLocal优化
除了内存池用到了ThreadLocal，还有很多地方用到ThreadLocal缓存对象。

##### 使用Nio类型的Channel在哪里设置非阻塞
见 AbstractNioChannel()构造时候设置

##### SelectKey cancel
netty 如果注销一个Channel，会调用key.Cancel()，但是并不是马上注销，可以累积到一定数量，再select()。



