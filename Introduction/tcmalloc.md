tcmalloc   
version:2.0.0


## 内存分配
```
static const size_t kMaxSize    = 256 * 1024;

小内存分配：ThreadCache -> CentralFreeList -> PageHeap
大内存分配：PageHeap
```

## ThreadCache

每个线程有自己的 ThreadCache。
```cpp
//主要成员
FreeList list_[kNumClasses]; //链表构成的数组
```
获取内存的顺序：

* 分配内存时， 先计算需要的尺寸，按下标在数组中找到链表，然后在链接中选择。如果没有，进入下一步
* 从`CentralFreeList` 中获取内存，见
`ThreadCache::FetchFromCentralCache(size_t cl, size_t byte_size)`
这里会一次多获取几块内存，添加到对应的list中。

## Span

表示一块联连续的内存。
```cpp
struct Span {
  PageID        start;          // Starting page number
  Length        length;         // Number of pages in span
  Span*         next;           // Used when in link list
  Span*         prev;           // Used when in link list
  void*         objects;        // Linked list of free objects
...
}
```

## CentralFreeList

这里会存在一个全局数组，数组成员为`CentralFreeList`，每种尺寸的内存对应一个。

```cpp
//主要成员
struct TCEntry {
  void *head;  // Head of chain of objects.
  void *tail;  // Tail of chain of objects.
};
TCEntry tc_slots_[kMaxNumTransferEntries];
Span     empty_;          //链表结构 Dummy header for list of empty spans
Span     nonempty_;       // Dummy header for list of non-empty spans
```
从 `CentralFreeList` 中取内存时，依次 

* tc_slots_，见 `RemoveRange(void **start, void **end, int N)`
* nonempty_ ，见 `FetchFromOneSpansSafe(int N, void **start, void **end)`
* 调用`PageHeap::New(Length n)` 分配n个 page

`CentralFreeList` 是一个链表定义，一个链表数组被定义在 `Static` 的静态成员中。
```cpp
class Static{
  static CentralFreeListPadded central_cache_[kNumClasses];
}
```

##PageHeap

```cpp
//主要成员
template <> class MapSelector<32> {
 public:
  typedef TCMalloc_PageMap2<32-kPageShift> Type; //Two-level radix tree.用于根据内存地址定位到对应的Span。
  typedef PackedCache<32-kPageShift, uint16_t> CacheType;
};

struct SpanList {
  Span        normal;  //这里是链表
  Span        returned;
};
// List of free spans of length >= kMaxPages
SpanList large_;
// Array mapping from span length to a doubly linked list of free spans
SpanList free_[kMaxPages];

typedef MapSelector<kAddressBits>::Type PageMap;
PageMap pagemap_;  // TCMalloc_PageMap2

```

* 在 free_中找到合适 的SpanList，依次查找 normal 或者 returned ，见 `PageHeap::New(Length n)`
* 在large_中查找，见`PageHeap::AllocLarge(Length n)` 
* 如果仍找不到会调用 PageHeap::GrowHeap(Length n) 分配内存,见`TCMalloc_SystemAlloc()`


##TCMalloc_SystemAlloc()
用于向系统申请内存
```cpp
TCMalloc_SystemAlloc(size_t size, size_t *actual_size, size_t alignment)
```
使用了2个内存分配工具
```cpp
//InitSystemAllocators(void) 
MmapSysAllocator *mmap = new (mmap_space) MmapSysAllocator();
SbrkSysAllocator *sbrk = new (sbrk_space) SbrkSysAllocator();
```


```
参考资料
http://kernelmaker.github.io/2016/04/23/TCMalloc-1-Introduction.html
```
