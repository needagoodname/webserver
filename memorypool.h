#ifndef MEMORY_POOL_H
#define MEMORY_POOL_H

#include <climits>
#include <cstddef>

template<typename T, size_t BlockSize = 4096>
class MemoryPool
{
public:
    typedef T               value_type;
    typedef T*              pointer;
    typedef T&              reference;
    typedef const T*        const_pointer;
    typedef const T&        const_reference;
    typedef size_t          size_type;
    typedef ptrdiff_t       difference_type;
    typedef std::false_type propagate_on_container_copy_assignment;
    typedef std::true_type  propagate_on_container_move_assignment;
    typedef std::true_type  propagate_on_container_swap;

    template<typename U> struct rebind {
        typedef MemoryPool<U> other;
    };

    MemoryPool() noexcept;
    MemoryPool(const MemoryPool& memoryPool) noexcept;
    MemoryPool(MemoryPool&& memoryPool) noexcept;
    template<typename U> MemoryPool(const MemoryPool<U>& MemoryPool) noexcept;

    ~MemoryPool() noexcept;

    MemoryPool& operator=(const MemoryPool& memoryPool) = delete;
    MemoryPool& operator=(MemoryPool&& memoryPool) noexcept;

    // 元素取址
    pointer address(reference x) const noexcept;
    const_pointer address(const_reference x) const noexcept;

    // 分配一个元素内存空间
    pointer allocate(size_type n = 1, const_pointer hint = 0);
    // 释放一个元素空间
    void deallocate(pointer p, size_type n = 1);
    
    // 最大大小
    size_type max_size() const noexcept;

    // 基于内存池的构造析构
    template<typename U, typename... Args> void construct(U* p, Args&&... args);
    template<typename U> void destroy(U* p);

    // 自带申请/释放内存的构造析构
    template<typename... Args> pointer newElement(Args&&... args);
    void deleteElement(pointer p);

private:
    // 存放元素或指针
    union Slot_{
        value_type element;
        Slot_* next;
    };

    typedef char*   data_pointer_; // 指向元素首地址
    typedef Slot_   slot_type_;
    typedef Slot_*  slot_pointer_;

    slot_pointer_ currentBlock_: // 内存块链表头指针
    slot_pointer_ currentSlot_; // 元素链表头指针
    slot_pointer_ lastSlot_; // 可存放元素的最后指针
    slot_pointer_ freeSlots_; // 已释放的内存链表头指针

    // 内存对齐
    size_type padPointer(data_pointer_ p, size_type align) const noexcept;
};

#endif