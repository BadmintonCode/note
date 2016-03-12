###### sizeof

```
c把字符常量当做整数，c++当做一个char
c  : sprintf(stdout, '%ul', sizeof('a'));// 4
cpp: cout<<sizeof('a');//1
```

###### c语言声明
```
变量加extern ,函数不需要。
```


###### shared_ptr /weak_ptr

```
1.weak_ptr 通过 shared_ptr 构造
2.不要创建多个shared_ptr管理同一个堆对象
3.循环引用 parent 持有 child的shared_ptr， child持有 parent的 weak_ptr。child获取parent 时候需要调用weak_ptr.lock()
```

###### enable_shared_from_this

```
1.用于shared_ptr 管理类的基类，可以在类中通过shared_from_this()获取当前的 shared_ptr。
2.通过持有一个weak_ptr来实现。
3.不能在构造函数中调用。此时weak_ptr还没初始化。
4.必须要先在栈上构造一个shared_ptr 。

class C :enable_shared_from_this<T>
{
public:
    void f()
    {
        shared_from_this();
    }
};

C c;
c.f();//error

```

###### unique_ptr

```
unique_ptr 
```
