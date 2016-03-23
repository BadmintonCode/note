### Java Client 使用手册

#### 1 java client 使用规范


##### 1.1 创建tcp连接

```java
ConnectionFactory factory = new ConnectionFactory();
connectionFactory.setAutomaticRecoveryEnabled(true);// 故障自动重连
factory.setUsername(userName);
factory.setPassword(password);
factory.setVirtualHost(virtualHost);
factory.setHost(hostName);
factory.setPort(portNumber);
ThreadPoolExecutor executor=xxx;//用于执行consumer的线程池
Connection conn = factory.newConnection(executor);
```

##### 1.2 生产者

* 创建channel

```java
Channel channel = conn.createChannel();
channel.queueDeclare(String queue, boolean durable, boolean exclusive, boolean autoDelete, Map<String, Object> arguments);
//durable-持久 ,exclusive-无连接自动删除队列 ,autoDelete-不使用自动删除队列

channel.confirmSelect() //设置确认机制
```

* 发送消息，等待确认

```java
BasicProperties props = queueConfig.isDurable() ? MessageProperties.MINIMAL_PERSISTENT_BASIC: MessageProperties.MINIMAL_BASIC
channel.basicPublish(String exchange, String routingKey,BasicProperties props, byte[] body);//发送消息
channel.waitForConfirms(long timeout)// 等待队列发送确认
```

##### 1.3 消费者

* 创建channel

```java
Channel channel = conn.createChannel();
channel.queueDeclare(String queue, boolean durable, boolean exclusive, boolean autoDelete, Map<String, Object> arguments);
channel.basicQos() //流控
```

* 注册消费

```java
channel.basicConsume(queueName, autoAck, "myConsumerTag",   //autoAck=false queue等consumer 发送确认才删除消息
     new DefaultConsumer(channel) {
         @Override
         public void handleDelivery(String consumerTag,
                                    Envelope envelope,
                                    AMQP.BasicProperties properties,
                                    byte[] body)
             throws IOException
         {
             String routingKey = envelope.getRoutingKey();
             String contentType = properties.getContentType();
             long deliveryTag = envelope.getDeliveryTag();
             // (process the message components here ...)
             channel.basicAck(deliveryTag, false);  //void basicAck(long deliveryTag, boolean multiple) throws IOException; multiple表示是否一次确认多个



         }
     });
```

##### 1.4 取消注册

```java
String basicConsume(String queue, Consumer callback) throws IOException; 返回为消费者生成的随机名字
void basicCancel(String consumerTag) throws IOException; 取消消费者
```


#### 2 要点


##### 2.1 生产者waitForConfirms /waitForConfirmsOrDie

* waitForConfirms：超时直接抛异常

* waitForConfirmsOrDie：超时不会抛异常，直接关闭channel

##### 2.2.consumer 响应 ack/nack/reject区别

```
basicNack(long deliveryTag, boolean multiple, boolean requeue)//消费者在消费消息失败情况下的响应：
```
[参考](http://stackoverflow.com/questions/24107913/how-to-requeue-messages-in-rabbitmq?answertab=votes#tab-top)

* basicNack(deliveryTag, false, true)   
   队列中的消息会被派发到别的consumer，做出此响应的consumer仍然可以收到消息，直到所有消息完成

* basicNack(deliveryTag, false, false)    
   做出此响应的consumer仍然可以收到消息，直到所有消息完成，但是失败的消息不会再出现在队列

* 无动作   
   消息处于没确定状态（对队列来讲），消息不会被派发给别的消费者（当该consumer被取消注册以后，消息被派发给别的consumer）。
 

##### 2.3.channel 与多线程



* 建议consumer producer 独占channel 使用。在某些情况下 channel 并不是线程安全的。
* channel 与 connection 是多对一的关系，一个connection表示一个tcp 连接。



 
_Channels and Concurrency Considerations (Thread Safety)
Channel instances must not be shared between threads. 
Applications should prefer using a Channel per thread instead of sharing the same  Channel across multiple threads. While some operations on channels are safe to invoke concurrently, some are not and will result in incorrect frame interleaving on the wire. Sharing channels between threads will also interfere with_
 

参考资料
-------
[channel thread](http://www.rabbitmq.com/api-guide.html#channel-threads)   
[rabbitmq-clustering-ha](http://88250.b3log.org/rabbitmq-clustering-ha)   
[Publisher Confirms](http://www.rabbitmq.com/confirms.html)
