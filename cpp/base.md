

#### bind 重载

```cpp
class T
{
  void f(int i);
  void f(string s);
}
bind((void(T::*)(int))&T::f, &t, 1);
```
#### namespace前置声明
```cpp
namespace a
{
namespace c
{
class T;
}
namespace b
{
c::T* p;
}
}
namespace a
{
namespace c
{
class T{}
}
}
```

#### cast

```
static_cast
1.用于类层次结构中基类和子类之间指针或引用的转换。
　　进行上行转换（把子类的指针或引用转换成基类表示）是安全的；
　　进行下行转换（把基类指针或引用转换成子类表示）时，由于没有动态类型检查，所以是不安全的。
2.用于基本数据类型之间的转换，如把int转换成char，把int转换成enum。这种转换的安全性也要开发人员来保证。
3.把空指针转换成目标类型的空指针。
4.把任何类型的表达式转换成void类型。

dynamic_cast
用于类层次间的上行转换和下行转换，还可以用于类之间的交叉转换。
1.在类层次间进行上行转换时，dynamic_cast和static_cast的效果是一样的；
2.在进行下行转换时，dynamic_cast具有类型检查的功能，比static_cast更安全。

reinterpret_cast
可以将一个指针转换成一个整数，也可以将一个整数转换成一个指针

如果转换的类型之间有继承关系，static_cast会修正，reinterpret_cast不会。

http://stackoverflow.com/questions/17471717/c-static-cast-from-int-to-void-to-char-can-you-help-me-to-understand-this

```

#### 初始化顺序以声明为准
```c++
struct A1
{
    A1(int i)
    {
        cout<<"A()" <<endl;
    }
};
struct A2
{   
    const A1 &a1_;  //1
    A2(A1 a1):a1_(a1)
    {
        cout<<"A()" <<endl;
    }
};
struct A3
{   

    A1 a1;
    A2 a2;// a2 依赖于 a1 构造，必须声明在a1后面， 按声明顺序构造 ，否则导致1处没有初始化

    A3():a1(1),a2(a1)
    {
        cout<<"A()" <<endl;
    }
};

```
