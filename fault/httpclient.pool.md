#httpclient ssl 连接引起连接池泄露

###背景


版本：4.3  
故障现象：日志出现异常 Timeout waiting for connection from pool  
原因：因为请求https站点，参数设置不当引起连接池泄露  




###问题分析

#####故障现象

连接池日志如下，其中total kept alive 表示连接池当前可用连接，如果为0就表示已经没有可用的连接了，进而报timeout waiting ... 。
```
o.a.h.i.c.PoolingHttpClientConnectionManager-Connection request: [route: {s}->https://api.mch.weixin.qq.com:443]
[total kept alive: 0; route allocated: 20 of 20; total allocated: 20 of 200]
```

#####故障原因

httpclient建立连接，对http站点的连接和https站点的连接处理不同。
建立过程见HttpClientConnectionOperator，setSoTimeout 主要用来设置建立tcp连接时，读取tcp连接的超时时间。

```java
sock.setSoTimeout(socketConfig.getSoTimeout());
sock = sf.connectSocket(connectTimeout, sock, host, remoteAddress, localAddress, context);
```

成功建立连接后会用用户设置的超时时间覆盖。重新设置读取tcp超时 见MainClientExec

```java
final int timeout = config.getSocketTimeout();  
if (timeout >= 0) {  
    managedConn.setSocketTimeout(timeout);
}
```

对http站点：建立连接时，只建立连接，不读取数据，所以 HttpClientConnectionOperator 里面设置的soTimeout没影响。  

对https站点：建立连接时，先建立连接，再读取数据，所以 HttpClientConnectionOperator里面设置的soTimeout会影响初次建立连接。  
而这个默认值为0（阻塞读），如果服务器无响应会导致线程阻塞，连接不能还回连接池。

#####如何认定https请求时，发生了这种故障

*  在请求https站点时，发现某个线程只有取连接log，无还连接log，后续也没有这个线程的log出现，线程栈日志（这个thread状态还是RUNNABLE）如下。

*  通过线程栈发现，当前正在建立ssl连接，并且有read 操作。

*  另外tomcat重启时，会出现这个线程还连接的log，猜测因为进程关闭回收资源引起。

```
"http-apr-18089-exec-81" daemon prio=10 tid=0x00007fd5040a1800 nid=0x52d0 runnable [0x00007fd540751000]
   java.lang.Thread.State: RUNNABLE
    at java.net.SocketInputStream.socketRead0(Native Method)
    at java.net.SocketInputStream.read(SocketInputStream.java:152)
    at java.net.SocketInputStream.read(SocketInputStream.java:122)
    at sun.security.ssl.InputRecord.readFully(InputRecord.java:442)
    at sun.security.ssl.InputRecord.readV3Record(InputRecord.java:554)
    at sun.security.ssl.InputRecord.read(InputRecord.java:509)
    at sun.security.ssl.SSLSocketImpl.readRecord(SSLSocketImpl.java:927)
    - locked <0x00000006f80e55d0> (a java.lang.Object)
    at sun.security.ssl.SSLSocketImpl.performInitialHandshake(SSLSocketImpl.java:1312)
    - locked <0x00000006f80e5680> (a java.lang.Object)
    at sun.security.ssl.SSLSocketImpl.startHandshake(SSLSocketImpl.java:1339)
    at sun.security.ssl.SSLSocketImpl.startHandshake(SSLSocketImpl.java:1323)
    at org.apache.http.conn.ssl.SSLConnectionSocketFactory.connectSocket(SSLConnectionSocketFactory.java:262)
    at org.apache.http.impl.conn.HttpClientConnectionOperator.connect(HttpClientConnectionOperator.java:118)
    at org.apache.http.impl.conn.PoolingHttpClientConnectionManager.connect(PoolingHttpClientConnectionManager.java:314)
    at org.apache.http.impl.execchain.MainClientExec.establishRoute(MainClientExec.java:357)
    at org.apache.http.impl.execchain.MainClientExec.execute(MainClientExec.java:218)
    at org.apache.http.impl.execchain.ProtocolExec.execute(ProtocolExec.java:194)
    at org.apache.http.impl.client.InternalHttpClient.doExecute(InternalHttpClient.java:186)
    at org.apache.http.impl.client.CloseableHttpClient.execute(CloseableHttpClient.java:82)
    at org.apache.http.impl.client.CloseableHttpClient.execute(CloseableHttpClient.java:106)
```

#####解决办法

可以设置这个soTimeOut参数来解决，这个soTimeOut 参数和我们平时设置的soTimeOut 并不是同一个地方。
修改后发现连接数比较稳定。


```java
PoolingHttpClientConnectionManager cm=xxx;
SocketConfig defaultSocketConfig = SocketConfig.custom()
                .setSoTimeout(5000).setSoLinger(-1).setTcpNoDelay(true).build();
cm.setDefaultSocketConfig(defaultSocketConfig);
```

并且 系统开始正常抛异常，主要是ssl连接建立的异常。

```
Caused by: org.apache.http.conn.ConnectTimeoutException: Connect to api.mch.weixin.qq.com:443 [api.mch.weixin.qq.com/101.226.90.149, api.mch.weixin.qq.com/101.226.129.200] failed: connect timed out
        at org.apache.http.impl.conn.HttpClientConnectionOperator.connect(HttpClientConnectionOperator.java:130) ~[httpclient-4.3.jar:4.3]
        at org.apache.http.impl.conn.PoolingHttpClientConnectionManager.connect(PoolingHttpClientConnectionManager.java:314) ~[httpclient-4.3.jar:4.3]
        at org.apache.http.impl.execchain.MainClientExec.establishRoute(MainClientExec.java:357) ~[httpclient-4.3.jar:4.3]
        at org.apache.http.impl.execchain.MainClientExec.execute(MainClientExec.java:218) ~[httpclient-4.3.jar:4.3]
        at org.apache.http.impl.execchain.ProtocolExec.execute(ProtocolExec.java:194) ~[httpclient-4.3.jar:4.3]
        at org.apache.http.impl.client.InternalHttpClient.doExecute(InternalHttpClient.java:186) ~[httpclient-4.3.jar:4.3]
        at org.apache.http.impl.client.CloseableHttpClient.execute(CloseableHttpClient.java:82) ~[httpclient-4.3.jar:4.3]
        at org.apache.http.impl.client.CloseableHttpClient.execute(CloseableHttpClient.java:106) ~[httpclient-4.3.jar:4.3]
        ... 50 common frames omitted
Caused by: java.net.SocketTimeoutException: connect timed out
        at java.net.PlainSocketImpl.socketConnect(Native Method) ~[na:1.7.0_45]
        at java.net.AbstractPlainSocketImpl.doConnect(AbstractPlainSocketImpl.java:339) ~[na:1.7.0_45]
        at java.net.AbstractPlainSocketImpl.connectToAddress(AbstractPlainSocketImpl.java:200) ~[na:1.7.0_45]
        at java.net.AbstractPlainSocketImpl.connect(AbstractPlainSocketImpl.java:182) ~[na:1.7.0_45]
        at java.net.SocksSocketImpl.connect(SocksSocketImpl.java:392) ~[na:1.7.0_45]
        at java.net.Socket.connect(Socket.java:579) ~[na:1.7.0_45]
        at sun.security.ssl.SSLSocketImpl.connect(SSLSocketImpl.java:618) ~[na:1.7.0_45]
        at org.apache.http.conn.ssl.SSLConnectionSocketFactory.connectSocket(SSLConnectionSocketFactory.java:251) ~[httpclient-4.3.jar:4.3]
        at org.apache.http.impl.conn.HttpClientConnectionOperator.connect(HttpClientConnectionOperator.java:118) ~[httpclient-4.3.jar:4.3]
```