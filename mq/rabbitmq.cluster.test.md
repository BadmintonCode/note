#####说明：部署一个集群由2个节点构成，stop其中一个节点，查看服务影响情况。

```
停止节点使用 rabbitmqctl stop命令
启动节点使用 rabbitmqserver -detach 命令
配置的2个节点如下：
node1：地址address1，master
node2：地址address2，salve

集群中，先启动的节点为master，后启动的为slave，所以存在数据不同步的情况
```

####客户端代码

```
ConnectionFactory factory = new ConnectionFactory();
factory.setAutomaticRecoveryEnabled(true);
connection = factory.newConnection(executor, addressList);//初始化时，依次选择addressList中的address进行连接
```

####测试结果

######master salve 数据不同步

```
addressList=new Address[]{address1,address2}
case1：stop master 直接抛异常，start master 角色不互换
case2：stop salve 无影响，start salve 角色不互换
```


######master salve 数据同步
```
addressList=new Address[]{address1,address2}
case1：stop master 直接抛异常，同时自动切换到slave，角色互换。启动salve，新的salve数据不同步
case2：stop salve 无影响，start salve 角色不互换。启动salve，新的salve数据不同步
```

######master salve 数据同步
```
addressList=new Address[]{address1,address2}
case1：stop salve 无影响（后台发现channel连的是master），start salve 角色不互换,新的salve数据不同步
```