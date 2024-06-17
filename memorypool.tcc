#ifndef MEMORY_BLOCK_TCC
#define MEMORY_BLOCK_TCC

#include "memorypool.h"

template<typename T, size_t BlockSize>
inline typename MemoryPool<T, BlockSize>::size_type
MemoryPool<T, BlockSize>::padPointer(data_pointer_ p, size_type align)
const noexcept {
    uintptr_t result = reinterpret_cast<uintptr_t>(p);
    return ((align - result) % align);
}

template<typename T, size_t BlockSize>
MemoryPool<T, BlockSize>::MemoryPool()
noexcept
{
    currentBlock_ = nullptr;
    currentSlot_ = nullptr;
    lastSlot_ = nullptr;
    freeSlots_ = nullptr;
}

template<typename T, size_t BlockSize>
MemoryPool<T, BlockSize>::MemoryPool(const MemoryPool<U>& memoryPool) 
noexcept : MemoryPool() {}

template<typename T, size_t BlockSize>
MemoryPool<T, BlockSize>::MemoryPool(MemoryPool&& memorypool)
noexcept {
    currentBlock_ = memoryPool.currentBlock_;
    memoryPool.currentBlock_ = nullptr;
    currentSlot_ = memoryPool.currentSlot_;
    lastSlot_ = memoryPool.lastSlot_;
    freeSlots_ = memoryPool.freeSlots;
}

// 拷贝构造函数，通过委托构造函数实现
template<typename T, size_t BlockSize>
template<class U>
MemoryPool<T, BlockSize>::MemoryPool(const MemoryPool<U>& MemoryPool)
noexcept : MemoryPool() {}

template<typename T, size_t BlockSize>
MemoryPool<T, BlockSize>& MemoryPool<T, BlockSize>::operator=(MemoryPool&& memoryPool)
noexcept {
    if(this != &memoryPool) {
        std::swap(currentBlock_, memoryPool.currentBlock_);
        currentBlock_ = memoryPool.currentBlock_;
        lastSlot_ = memoryPool.lastSlot_;
        freeSlots_ = memoryPool.freeSlots_;
    }
    return *this;
}

template<typename T, size_t BlockSize>
MemoryPool<T, BlockSize>::~MemoryPool() noexcept
{
    slot_pointer_ curr = currentBlock_;
    while(curr != nullptr) {
        slot_pointer_ prev = curr->next;
        operator delete(reinterpret_cast<void*>(curr));
        curr = prev;
    }
}

template<typename T, size_t BlockSize>
inline typename MemoryPool<T, BlockSize>::pointer
MemoryPool<T, BlockSize>::address(reference x) const noexcept
{
    return &x;
} 

template<typename T, size_t BlockSize>
inline typename MemoryPool<T, BlockSize>::const_pointer
MemoryPool<T, BlockSize>::address(const_reference x)
const noexcept
{
    return &x;
}

// 申请一个新的block放进内存池
template<typename T, size_t BlockSize>
void MemoryPool<T, BlockSize>::allocateBlock()
{
    // 申请BlockSize字节大小内存
    data_pointer_ newBlock = reinterpret_cast<data_pointer_>(operator new(BlockSize));
    // newBlock成为新的block内存首址
    reinterpret_cast<slot_pointer_>(newBlock)->next = currentBlock_;
    // currentBlock更新为newBlock位置
    currentBlock_ = reinterpret_cast<slot_pointer_>(newBlock);
    // 保存第一个slot用于Block链表连接
    data_pointer_ body = newBlock + sizeof(slot_pointer_);
    // 求解空间对齐需要字节数
    size_type bodyPadding = padPointer(body, alignof(slot_type_));
    // 若bodyPadding不足以存放一个slot，跳过这个空间
    currentSlot_ = reinterpret_cast<slot_pointer_>(body + bodyPadding);
    lastSlot_ = reinterpret_cast<slot_pointer_>(newBlock + BlockSize - sizeof(slot_type_) + 1);
    // 始址: newBlock  块大小 BlockSize 末址 newBlock + BlockSize 末址减去
    // 一个slot槽大小得到倒数第二个slot的末址 再加一得到最后一块slot的始址
}

template <typename T, size_t BlockSize>
inline typename MemoryPool<T, BlockSize>::pointer
MemoryPool<T, BlockSize>::allocate(size_type n, const_pointer hint)
{
  if (freeSlots_ != nullptr) {
    pointer result = reinterpret_cast<pointer>(freeSlots_);
    freeSlots_ = freeSlots_->next;
    return result;
  }
  else {
    if (currentSlot_ >= lastSlot_) allocateBlock();
    return reinterpret_cast<pointer>(currentSlot_++);
  }
}

template <typename T, size_t BlockSize>
inline void
MemoryPool<T, BlockSize>::deallocate(pointer p, size_type n)
{
  if (p != nullptr) {
    reinterpret_cast<slot_pointer_>(p)->next = freeSlots_;
    freeSlots_ = reinterpret_cast<slot_pointer_>(p);
  }
}

template <typename T, size_t BlockSize>
inline typename MemoryPool<T, BlockSize>::size_type
MemoryPool<T, BlockSize>::max_size()
const noexcept
{
  size_type maxBlocks = -1 / BlockSize;
  return (BlockSize - sizeof(data_pointer_)) / sizeof(slot_type_) * maxBlocks;
}

// 在已有内存上构造对象
template <typename T, size_t BlockSize>
template <class U, class... Args>
inline void
MemoryPool<T, BlockSize>::construct(U* p, Args&&... args)
{
  new (p) U (std::forward<Args>(args)...);
}

// 销毁对象
template <typename T, size_t BlockSize>
template <class U>
inline void
MemoryPool<T, BlockSize>::destroy(U* p)
{
  p->~U();
}

// 创建新元素
template <typename T, size_t BlockSize>
template <class... Args>
inline typename MemoryPool<T, BlockSize>::pointer
MemoryPool<T, BlockSize>::newElement(Args&&... args)
{
  pointer result = allocate();
  construct<value_type>(result, std::forward<Args>(args)...);
  return result;
}

// 删除元素
template <typename T, size_t BlockSize>
inline void
MemoryPool<T, BlockSize>::deleteElement(pointer p)
{
  if (p != nullptr) {
    p->~value_type();
    deallocate(p);
  }
}

#endif