


查看依赖的动态链接库   


```
readelf -d libleveldb.so #-d Display the dynamic section
-
Dynamic section at offset 0x53cf0 contains 26 entries:
  Tag        Type                         Name/Value
 0x0000000000000001 (NEEDED)             Shared library: [libtcmalloc.so.4]
 0x0000000000000001 (NEEDED)             Shared library: [libstdc++.so.6]
 0x0000000000000001 (NEEDED)             Shared library: [libm.so.6]
 0x0000000000000001 (NEEDED)             Shared library: [libgcc_s.so.1]
 0x0000000000000001 (NEEDED)             Shared library: [libpthread.so.0]
 0x0000000000000001 (NEEDED)             Shared library: [libc.so.6]
 0x000000000000000e (SONAME)             Library soname: [libleveldb.so.1]  //so名字
```

查看链接器路径
``` 
 readelf -x .interp memcached   #-x Dump the contents of section

 Hex dump of section '.interp':
  0x00400238 2f6c6962 36342f6c 642d6c69 6e75782d /lib64/ld-linux-
  0x00400248 7838362d 36342e73 6f2e3200          x86-64.so.2.
```

```
  readelf -x .interp memcached #-S --section-headers   Display the sections' header
 Section Headers:
  [Nr] Name              Type             Address           Offset
       Size              EntSize          Flags  Link  Info  Align
  [ 0]                   NULL             0000000000000000  00000000
       0000000000000000  0000000000000000           0     0     0
  [ 1] .interp           PROGBITS         0000000000400238  00000238
```

