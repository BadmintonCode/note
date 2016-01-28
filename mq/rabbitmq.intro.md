#### 1.基本概念

* Broker：简单来说就是消息队列服务器实体。
* Binding：绑定，它的作用就是把exchange和queue按照路由规则绑定起来。
* Routing Key：路由关键字，exchange根据这个关键字进行消息投递。
* vhost：虚拟主机，一个broker里可以开设多个vhost，用作不同用户的权限分离。
* producer：消息生产者，就是投递消息的程序。
* consumer：消息消费者，就是接受消息的程序。
* channel：消息通道，在客户端的每个连接里，可建立多个channel，每个channel代表一个会话任务。

rabbitmq 架构如下：


#### 2 实例

##### 2.1 直接操作queue

producer往队列里面生产，consumer 消费队列里面的数据


__produce__
```python
#!/usr/bin/env python
import pika

connection = pika.BlockingConnection(pika.ConnectionParameters(host='localhost'))
channel = connection.channel()
channel.queue_declare(queue='hello',durable=True) #如果队列不存在就创建
channel.basic_publish(exchange='',routing_key='hello',body='Hello World!') #发送消息
connection.close()
```
__consume__
```python
#!/usr/bin/env python
import pika

connection = pika.BlockingConnection(pika.ConnectionParameters(host='localhost'))
channel = connection.channel()
channel.queue_declare(queue='hello',durable=True) #如果队列不存在就创建

def callback(ch, method, properties, body):
     print " [x] Received %r" % (body,)

channel.basic_consume(callback,queue='hello',no_ack=True) #注册消费者
channel.start_consuming()
```



__queue_declare__   
producer、consumer都必须调用，durable=True声明消息持久化， 不支持修改操作（已存在队列durable=false,再次做true声明会出错）

##### 2.2 通过exchange广播

producer把消息推送给exchange，exchange &nbsp;广播消息到queue，consumer 消费队列里面的数据

```python
#!/usr/bin/env python
import pika
import sys

connection = pika.BlockingConnection(pika.ConnectionParameters(host='localhost'))
channel = connection.channel()

channel.exchange_declare(exchange='logs',type='fanout') #创建exchange

message = ' '.join(sys.argv[1:]) or "info: Hello World!"
channel.basic_publish(exchange='logs',routing_key='',body=message) # 生产消息
connection.close()
```


```python
#!/usr/bin/env python
import pika

connection = pika.BlockingConnection(pika.ConnectionParameters(host='localhost'))
channel = connection.channel()

channel.exchange_declare(exchange='logs',type='fanout')#创建exchange

result = channel.queue_declare(exclusive=True)
queue_name = result.method.queue

channel.queue_bind(exchange='logs',queue=queue_name) #队列绑定到exchange


def callback(ch, method, properties, body):
    print " [x] %r" % (body,)

channel.basic_consume(callback,queue=queue_name,no_ack=True) #队列个注册消费者

channel.start_consuming()
```


##### 2.3 通过exchange 和 routing key 路由

>producer把消息推送给exchange，exchange 根据routing key 路由到queue，consumer 消费队列里面的数据

```python
channel.exchange_declare(exchange='direct_logs',type='direct') #创建一个 direct 类型的exchange

channel.basic_publish(exchange='direct_logs',routing_key=severity,body=message)
```


```python
channel.exchange_declare(exchange='direct_logs',type='direct') #创建一个 direct 类型的exchange
channel.queue_bind(exchange='direct_logs',queue=queue_name,routing_key=severity) #bind queue 和  exchange
channel.basic_consume(callback,queue=queue_name,no_ack=True)
```

#### 3.exchange与queue

##### 3.1 三种类型exchange

###### 3.1.1 direct

通过routing key 确定消息路由到具体的队列，系统存在一个默认的exchange ，名字是空串，消息路由时，根据routing key。

###### 3.1.2 topic

通过\* 和 # 通配符来匹配

\* (star) can substitute for exactly one word.

\# (hash) can substitute for zero or more words.

###### 3.1.3 fanout

发送消息到所有队列

fanout exchange 忽略 routing_key:

##### 3.2 临时队列

不会被镜像

result = channel.queue_declare() &nbsp;//随机名字的队列

result = channel.queue_declare(exclusive=True) // 连接断开，队列删除

如果没有队列bind 到exchange，消息会丢失

##### 3.3 操作过程

__生产者__  

a.创建一个exchange：   
>channel.exchange_declare(exchange='logs',type='fanout') \# 名字、类型_

__消费者__

a.创建一个exchange  
>channel.exchange_declare(exchange='logs',type='fanout') # 名字、类型

b.创建队列

c.bind到exchange

>channel.queue_bind(exchange=exchange_name,queue=queue_name)   
>//queue关注exchange的消息，这里是fanout类型的exchange

>channel.queue_bind(exchange=exchange_name,queue=queue_name, routing_key='black')   
>//关注black消息，这里可以是 direct类型的exchange

d.注册消费


#### 4.集群

这里说的集群是局域网的解决方案，对于广域网的方案可以见&nbsp;[http://www.rabbitmq.com/distributed.html]

broker通信： 通过Erlang message-passing，同一个集群有同样的Erlang cookie

资源共享：

virtual hosts, exchanges, users, and permissions 被复制到所有节点

queue 可以被定位在一个node上，但是连接到任何节点的client均可以看到所有的queue

存储支持两种模式：disk 和 ram

#### 5.高可用

>http://www.rabbitmq.com/ha.html

##### 5.1 特性

*   exchange/bind:存在于所有的节点上

*   queue：可以有选择的被镜像到节点上

*   镜像queue组成：一个master、多个slave

*   生产者投递到queue的消息都会复制到slave。

*   消费者不管连接到是master还是slave，都是和master通信。（不能降低负载）

 
>By default, queues within a RabbitMQ cluster are located on a single node (the node on which they were first declared). 
This is in contrast to exchanges and bindings, which can always be considered to be on all nodes.
Queues can optionally be made mirrored across multiple nodes. Each mirrored queue consists of one master and one or more slaves, 
with the oldest slave being promoted to the new master if the old master disappears for any reason.

>Messages published to the queue are replicated to all slaves. Consumers are connected to the master regardless of
which node they connect to, with slaves dropping messages that have been acknowledged at the master. Queue mirroring 
therefore enhances availability, but does not distribute load across nodes (all participating nodes each do all the work).

>This solution requires a RabbitMQ cluster, which means that it will not cope seamlessly with network partitions 
within the cluster and, for that reason, is not recommended for use across a WAN (though of course, 
clients can still connect from as near and as far as needed).


#####  5.2 镜像 policy

ha-mode:

all:镜像到所有节点

exactly：镜像到指定节点数

nodes:镜像到指定节点

##### 5.3 slave同步：

非显式：集群中加入slave，随着生产消费进行，slave 会逐渐与master 同步。

显式： ha-sync-mode =automatic | manual。
>automatic-队列加入，即开始同步。manual-用户手工操作(默认)

##### 5.4 选举master：

* 存在数据同步的slave
>选择已经保持数据同步了的slave，先前挂掉的节点重新，丢掉自身的数据，从新的master获取数据并开始同步。

* 不存在数据同步的slave
>ha-promote-on-shutdown=always\|when-synced\[default\]

__镜像队列语义__

>As discussed, for each mirrored queue there is one master and several slaves, each on a different node. 
The slaves apply the operations that occur to the master in exactly the same order as the master and thus 
maintain the same state. All actions other than publishes go only to the master, and the master then broadcasts 
the effect of the actions to the slaves. Thus clients consuming from a mirrored queue are in fact consuming from the master.


重新选master（选择最旧的slave），新master会认为consumer重连，会把所有没有收到ACk的消息requeue（ACK可能未到已挂掉的Master或者已经到已挂掉的Master，

但在广播到到Slave之前，Master挂掉），会引起重复消费。生产者发布的消息不会丢失。

>As the chosen slave becomes the master, no messages that are published to the mirrored queue during this time will be lost:
【messages published to a mirrored queue are always published directly to the master and all slaves】. 
Thus should the master fail, the messages continue to be sent to the slaves and will be added to the queue 
once the promotion of a slave to the master completes.

Similarly, messages published by clients using publisher confirms will still be confirmed correctly 
even if the master (or any slaves) fail between the message being published and the message being able to 
be confirmed to the publisher. Thus from the point of view of the publisher, publishing to a mirrored queue
is no different from publishing to any other sort of queue.


如果消费消息的时候使用 noAck=true （即不带确认机制）会导致消息丢失。

Consumer Cancellation Notification：x-cancel-on-ha-failover 设置为true，可以在队列发生fail over 得到通知。

#### 6.可靠性

>http://www.rabbitmq.com/reliability.html

*   连接失败：支持重连

*   确认机制：生产，消费均支持。at-least-once 与 at-most-once&nbsp;

*   心跳机制：检测tcp链接

#### 7.服务器配置

>http://www.rabbitmq.com/configure.html
>http://www.rabbitmq.com/configure.html#config-items

environment variables

a configuration file



#### 参考资料


>http://www.rabbitmq.com/clustering.html
>http://stackoverflow.com/questions/24773378/what-happens-to-a-rabbitmq-cluster-if-the-only-disc-node-dies/24774412#24774412
>http://zhanghua.1199.blog.163.com/blog/static/4644980720138171451630/
>http://blog.csdn.net/zyz511919766/article/details/41896823