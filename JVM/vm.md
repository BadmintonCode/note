
###btrace

btrace监控每次退出后，原先所有的class都不会被恢复，你的所有的监控代码依然一直在运行 

```
wget  https://kenai.com/projects/btrace/downloads/download/releases/release-1.2.5.1/btrace-bin.tar.gz

export BTRACE_HOME=/home/webedit/<user>
export CLASSPATH=$BTRACE_HOME/build/btrace-agent.jar:$BTRACE_HOME/build/btrace-boot.jar:$BTRACE_HOME/build/btrace-client.jar

../bin/btrace -cp  . <pid> <TraceClass>.java 
 
```

```
参考资料
http://www.iteye.com/topic/1005918
https://kenai.com/projects/btrace/pages/UserGuide
```


### jvisualvm 远程监控jvm

```
export CATALINA_OPTS="-Dcom.sun.management.jmxremote -Djava.rmi.server.hostname=`<host>` -Dcom.sun.management.jmxremote.port=`<port>` -Dcom.sun.management.jmxremote.ssl=false -Dcom.sun.management.jmxremote.authenticate=false"
```