#### Basic-Paxos

##### 第一阶段 Prepare
* P1a：Proposer 发送 Prepare
Proposer 生成全局唯一且递增的提案 ID（Proposalid，以高位时间戳 + 低位机器 IP 可以保证唯一性和递增性），向 Paxos 集群的所有机器发送 PrepareRequest，这里无需携带提案内容，只携带 Proposalid 即可。

* P1b：Acceptor 应答 Prepare
Acceptor 收到 PrepareRequest 后，做出“两个承诺，一个应答”。

两个承诺：   
第一，不再应答 Proposalid 小于等于（注意：这里是 <= ）当前请求的 PrepareRequest；    
第二，不再应答 Proposalid 小于（注意：这里是 < ）当前请求的 AcceptRequest

一个应答：   
返回自己已经 Accept 过的提案中 ProposalID 最大的那个提案的内容，如果没有则返回空值;

注意：这“两个承诺”中，蕴含两个要点：   
就是应答当前请求前，也要按照“两个承诺”检查是否会违背之前处理 PrepareRequest 时做出的承诺；   
应答前要在本地持久化当前 Propsalid。

##### 第二阶段 Accept

* P2a：Proposer 发送 Accept
“提案生成规则”：Proposer 收集到多数派应答的 PrepareResponse 后，从中选择proposalid最大的提案内容，作为要发起 Accept 的提案，如果这个提案为空值，则可以自己随意决定提案内容。然后携带上当前 Proposalid，向 Paxos 集群的所有机器发送 AccpetRequest。

* P2b：Acceptor 应答 Accept
Accpetor 收到 AccpetRequest 后，检查不违背自己之前作出的“两个承诺”情况下，持久化当前 Proposalid 和提案内容。最后 Proposer 收集到多数派应答的 AcceptResponse 后，形成决议。


##### 分情况讨论
最简单的情况：    
发起prepare请求，response都是空值；决定提案内容v，发起accept请求，获得多数派应答，提案成功。

少数派已经accept的情况：    
发起prepare请求，提案号为p1，response都是空值；决定提案内容v1，发起accept请求，只获得少数派应答，提案失败。  
下次再发起一轮paxos，提案号为p2 (p2>p1)。有如下两种情况：  
如果prepare的多数派中包含了之前应答accept的少数派，则本轮提案为(p2,v1)。  
如果prepare的多数派中不包含了之前应答accept的少数派，则本轮提案内容自定义为v2。这时候，多数派为(p2,v2),少数派为(p1,v1)。因为在读取过程也需要执行一轮paxos，prepare 需要获取多数派，所以读到的值为v2。


总结：   
如果多数派应答accept，表示提案已经被多数派接收，这个提案可以确定是成功的。（因此不会存在多数派accept了低提案号，而少数派accept了高提案号）   
如果少数派应答accept，最终这个提案有可能被多数派接收，也可能不被多数派接收。

对应到实际工程环境，某个节点没有应答，要么不满足承诺，要么是网络故障导致发起的应答没有被收到。所以一个Client-Server的场景下，Client发起的请求在没有收到确认的情况下，无法确定请求是否在Server上成功执行，也许已经执行成功，也许执行失败。

### Multi-Paxos

经过一轮的Basic-Paxos，成功得到多数派accept的proposer即成为leader（这个过程称为leader Elect），leader任期中，由于没有了并发冲突，直接执行accept阶段即可。   

为了提高数据恢复的性能，leader得到多数派的accept后会写一个confirm日志，同时也会批量同步给其他备机。   

新leader对有confirm的提案直接回放，对没有confirm的提案，需要执行paxos协议。   

为了预防幽灵复现（只有leader上存在的提案，因为leader crash 恢复被重新选择为leader）的情况，需要在日志中记录leader的EpochID(proposalid)，遇到epochID小的数据直接丢弃（根本原因是没有形成多数派？）。

 
##### 参考资料
>http://oceanbase.org.cn/?p=90  
>http://oceanbase.org.cn/?p=111



