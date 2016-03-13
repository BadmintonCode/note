
#### cluster


```
redis-cli -c -h -p 
CLUSTER MEET [ip] [port]  #添加节点到集群
CLUSTER ADDSLOTS [slot..] #分配槽
CLUSTER KEYSLOT   "str"    #计算key的槽
CLUSTER NODES #查看集群信息

MOVED 错误  #访问的key不在对应的节点，会输出该错误，包含负责存储该slot的node ip和port。
```

迁移步骤
```
利用工具redis-trib操作

1.CLUSTER SETSLOT <slot> IMPORTING <source-node-id>  #在目标节点操作
2.CLUSTER SETSLOT <slot> MIGRATING <destination-node-id>  #在源节点操作
3.CLUSTER GETKEYSINSLOT <slot> <count> #最多获得key 列表
4.MIGRATE <host> <port> <key> 0 <timeout> #0-cluster只能使用0号数据库，原子操作。
5.CLUSTER SETSLOT <slot> NODE <destination-node-id>

3、4 一直操作直到迁移完。
MIGRATING节点：如果访问的key存在，输出结果给client；
              如果可以不存在，输出ASK redirection给client，提示到目标节点访问。
IMPORTING节点：访问属于slot(迁移中)的key，返回MOVED redirection提示到源节点访问。
              如果先发送ASKING(一次性)，再发送要执行的命令，可以执行。
这样保证：新的key在目标节点创建；已经迁移过的节点能正确处理。（在迁移中映射关系还是按照旧的slot-node）


```

```
cluster-node-timeout <milliseconds>: 超时时间
cluster-slave-validity-factor <factor>: 0-一直尝试failover，非0-失连超过对应次数，不再failover。

```

http://blog.csdn.net/nsrainbow/article/details/49032337