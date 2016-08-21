
#### shared_ptr /weak_ptr

```
1.weak_ptr 通过 shared_ptr 构造
2.不要创建多个shared_ptr管理同一个堆对象
3.循环引用:parent 持有 child的shared_ptr， child持有 parent的 weak_ptr。child获取parent 时候需要调用weak_ptr.lock()
```

#### enable_shared_from_this

```
1.用于shared_ptr 管理类的基类，可以在类中通过shared_from_this()获取当前的 shared_ptr。
2.不能在构造函数中调用。此时weak_ptr还没初始化。
3.必须使用 shared_ptr构造 。
4.实现原理：
enable_shared_from_this 内部有个weak_ptr，假如类C是enable_shared_from_this的派生类，
在构造shared_ptr<C>时候，会调用enable_shared_from_this上的一个函数构造内部的weak_ptr,
其中有个参数为栈上的shared_ptr。
[http://www.cnblogs.com/lzjsky/archive/2011/05/05/2037363.html]

class C :enable_shared_from_this<C>
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


#### thread safe
```
1.同一个shared_ptr可以被多线程同时读取。
2.不同的shared_ptr,管理同一个对象，可以被多线程修改。
3.同一个shared_ptr,同时写入，读取不是线程安全的。
[http://blog.csdn.net/jiangfuqiang/article/details/8292906]
```

#### std::shared_ptr 类型擦除

```cpp
//demo 
#include <functional>

template <typename T>
void delete_deleter( void * p ) {
  delete static_cast<T*>(p);
}

template <typename T>
struct my_unique_ptr {
  std::function< void (void*) > deleter;
  T * p;
  
  template <typename U>
  my_unique_ptr( U * p, std::function< void(void*) > deleter = &delete_deleter<U> ) 
     : p(p), deleter(deleter) 
  {

  }

  ~my_unique_ptr() {
     deleter( p );   
  }

};

int main() {
   my_unique_ptr<void> p( new double ); // deleter == &delete_deleter<double>
}
[http://stackoverflow.com/questions/5913396/why-do-stdshared-ptrvoid-work]
```



