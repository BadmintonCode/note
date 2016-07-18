
## Zookeeper Client 异常处理


#### 轮询逻辑
对于watcher。

* 如果在ping要求的时间内没收到心跳包，触发Disconnected事件，继续重试。  
* 如果连上server以后，server告知超时，触发 Expired 事件，不重试退出。  

对于直接调用（不含callback）

* 如果在ping要求的时间内没收到心跳包，抛出ConnectionLossException。
* 如果连上server以后，server告知超时，抛出SessionExpiredException。

##### SendThread.start()
```java

clientCnxnSocket.introduce(this,sessionId);
clientCnxnSocket.updateNow();
clientCnxnSocket.updateLastSendAndHeard();
int to;
long lastPingRwServer = System.currentTimeMillis();
final int MAX_SEND_PING_INTERVAL = 10000; //10 seconds
while (state.isAlive()) {//不为CLOSED状态
  try {
    if (!clientCnxnSocket.isConnected()) {
        if (closing || !state.isAlive()) {
            break;
        }
        startConnect();
        clientCnxnSocket.updateLastSendAndHeard();
    }
    if (state.isConnected()) {        
        to = readTimeout - clientCnxnSocket.getIdleRecv();
    } else {
        to = connectTimeout - clientCnxnSocket.getIdleRecv();
    }
    if (to <= 0) {//指定时间没有收到请求，抛出 SessionTimeoutException
        ...
        throw new SessionTimeoutException(warnInfo);
    }
    if (state.isConnected()) {
      //1000(1 second) is to prevent race condition missing to send the second ping
      //also make sure not to send too many pings when readTimeout is small 
        int timeToNextPing = readTimeout / 2 - clientCnxnSocket.getIdleSend() - 
            ((clientCnxnSocket.getIdleSend() > 1000) ? 1000 : 0);
        //send a ping request either time is due or no packet sent out within MAX_SEND_PING_INTERVAL
        if (timeToNextPing <= 0 || clientCnxnSocket.getIdleSend() > MAX_SEND_PING_INTERVAL) {
            sendPing();//构造一个ping 类型的 packet 放到outgoingQueue中
            clientCnxnSocket.updateLastSend();
        } else {
            if (timeToNextPing < to) {
                to = timeToNextPing;
            }
        }
    }
    ...
    clientCnxnSocket.doTransport(to, pendingQueue, outgoingQueue, ClientCnxn.this);
  } catch (Throwable e) {
    if (closing) {//client主动关闭直接退出重试
        ...
        break;
    } else {
        if (e instanceof SessionExpiredException) {
            LOG.info(e.getMessage() + ", closing socket connection");
        } else if (e instanceof SessionTimeoutException) {
            LOG.info(e.getMessage() + RETRY_CONN_MSG);
        } else if (e instanceof EndOfStreamException) {
            LOG.info(e.getMessage() + RETRY_CONN_MSG);
        } else if (e instanceof RWServerFoundException) {
            LOG.info(e.getMessage());
        } else {
            LOG.warn(
                    "Session 0x"
                            + Long.toHexString(getSessionId())
                            + " for server "
                            + clientCnxnSocket.getRemoteSocketAddress()
                            + ", unexpected error"
                            + RETRY_CONN_MSG, e);
        }
        cleanup();
        if (state.isAlive()) {//不为CLOSED 状态
            eventThread.queueEvent(new WatchedEvent(Event.EventType.None, Event.KeeperState.Disconnected, null));
        }
        clientCnxnSocket.updateNow();
        clientCnxnSocket.updateLastSendAndHeard();
    }
  }
}
cleanup();
clientCnxnSocket.close();
if (state.isAlive()) {
    // 通知Disconnected 事件
    eventThread.queueEvent(new WatchedEvent(Event.EventType.None,Event.KeeperState.Disconnected, null));
}
```

#### doTransport()

如果server通知client超时，会设置state = States.CLOSED，同时抛出SessionExpiredException异常。

```java
doTransport(int waitTimeOut, List<Packet> pendingQueue, LinkedList<Packet> outgoingQueue, ClientCnxn cnxn)
->如果连接建立
  ->SendThread.primeConnection();
    ->ConnectRequest conReq = new ConnectRequest(0, lastZxid,sessionTimeout, sessId, sessionPasswd);
    ->把watch请求放到 outgoingQueue中
    ->outgoingQueue.addFirst(new Packet(null, null, conReq,null, null, readOnly));//初始化session的请求
->如果连接可读写
  ->void doIO(List<Packet> pendingQueue, LinkedList<Packet> outgoingQueue, ClientCnxn cnxn)throws InterruptedException, IOException
    ->如果可读
      ->if (!initialized)
        ->ClientCnxnSocket.readConnectResult()
          ->this.sessionId = conRsp.getSessionId();
          ->SendThread.onConnected(conRsp.getTimeOut(), this.sessionId,conRsp.getPasswd(), isRO)
            ->negotiatedSessionTimeout = _negotiatedSessionTimeout;
             ->if negotiatedSessionTimeout <= 0 //过期了
              ->state = States.CLOSED;
              ->eventThread.queueEvent(new WatchedEvent(Watcher.Event.EventType.None,Watcher.Event.KeeperState.Expired, null))//通知过期
              ->throw new SessionExpiredException(warnInfo);//
             ->else
              ->state = (isRO) ? States.CONNECTEDREADONLY : States.CONNECTED;
              ->eventThread.queueEvent(new WatchedEvent(Watcher.Event.EventType.None,eventState, null))
```

##### cleanup()

```java
对pendingQueue 和 outgoingQueue的packet调用 conLossPacket(Packet p)
conLossPacket(Packet p)操作如下:
switch (state) {
  case AUTH_FAILED:
      p.replyHeader.setErr(KeeperException.Code.AUTHFAILED.intValue());
      break;
  case CLOSED:
      p.replyHeader.setErr(KeeperException.Code.SESSIONEXPIRED.intValue());
      break;
  default:
      p.replyHeader.setErr(KeeperException.Code.CONNECTIONLOSS.intValue());
}
finishPacket(p);
```

##### finishPacket(Packet p)
会对package进行notify
```java
if (p.watchRegistration != null) {
    p.watchRegistration.register(p.replyHeader.getErr());
}
if (p.cb == null) {
      synchronized (p) {
        p.finished = true;
              p.notifyAll();
          }
      } else {
          p.finished = true;
          eventThread.queuePacket(p);
      }
  }
```

##### Zookeeper.exists()
这里拿一个不含callback的操作来举例，其他类似。

```java
//public Stat exists(final String path, Watcher watcher)throws KeeperException, InterruptedException
->ReplyHeader r = cnxn.submitRequest(h, request, response, wcb);
  ->ReplyHeader r = new ReplyHeader();
    Packet packet = queuePacket(h, r, request, response, null, null, null,
                null, watchRegistration);
    synchronized (packet) {
        while (!packet.finished) {
            packet.wait();
        }
    }
    return r;
->if (r.getErr() != 0) 
  throw KeeperException.create(KeeperException.Code.get(r.getErr()), clientPath);
```




