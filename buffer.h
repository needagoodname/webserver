#ifndef BUFFER_H
#define BUFFER_H

#include <string>   
#include <string.h>
#include <vector> 
#include <assert.h>
#include <unistd.h>  
#include <sys/uio.h>

// 输入输出缓冲池
class Buffer final
{
public:
    Buffer(size_t initialSize = 2048);
    ~Buffer() = default;

    //可读bytes
    inline size_t ReadableBytes() const 
    {
        return writerIndex - readerIndex;
    }

    //可写bytes
    size_t WritableBytes() const
    {
        return buffer.size() - writerIndex;
    }

    //预留bytes
    inline size_t PrependableBytes() const
    {
        return readerIndex;
    }

    // 可写区入口迭代器
    char* BeginWrite()
    {
        return begin() + writerIndex;
    }
    const char* BeginWrite() const
    {
        return begin() + writerIndex;
    }
    // 内容可读区入口迭代器
    const char* Peek() const
    {
        return begin() + readerIndex;
    }

    // 可读区入口数据
    char GetPsn(const int& psn) const {
        return buffer[readerIndex + psn];
    }

    char GetBegin() const {
        return GetPsn(0);
    }

    // 修改距离读指针psn位置
    void SetPsn(const char &c, const int& psn) {
        buffer[readerIndex + psn] = c;
    }

    void EnsureWriteableBytes(size_t len); // 确保buffer大小合适
    // 向buffer写入len数据
    inline void HasWritten(size_t len) {
        writerIndex += len;
    }; 
    void Unwrite(size_t len); // 撤销向buffer写入len数据
    
    void Retrieve(size_t len); // 从buffer读取len数据
    void RetrieveUntil(const char* end); // 读取end为止的数据
    void RetrieveAll(); // 清空buffer缓冲区

    std::string RetrieveAllAsString(); // 读取全部数据副本
    std::string RetrieveAsString(size_t len); // 读取指定字节数副本
 
    void Append(const std::string& str); // 添加数据
    void Append(const char* str, size_t len);
    void Append(const void* data, size_t len);
 
    ssize_t ReadFd(int fd, int* Errno); // 读取指定fd缓冲区数据

private:
    // 提供迭代器
    char* begin() {
        return &*buffer.begin();
    }
 
    //常量默认调用此重载函数
    const char* begin() const {
        return &*buffer.begin();
    }

    // 扩容
    void MakeSpace(size_t len);
 
    std::vector<char> buffer;
    size_t readerIndex;
    size_t writerIndex;
};
 
#endif //BUFFER_H