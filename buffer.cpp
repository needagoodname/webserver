#include "buffer.h"

Buffer::Buffer(size_t initialSize):buffer(initialSize),readerIndex(0),writerIndex(0)
{
    assert(ReadableBytes() == 0);
    assert(WritableBytes() == initialSize);
    assert(PrependableBytes() == 0);
}

void Buffer::Unwrite(size_t len)
{
    assert(len <= ReadableBytes());
    writerIndex -= len;
}

void Buffer::Retrieve(size_t len)
{
    assert(len <= ReadableBytes());
    if(len < ReadableBytes()) {
        readerIndex += len;
    } else {
        RetrieveAll();
    }
}

void Buffer::RetrieveAll()
{
    bzero(&buffer[0], buffer.size());
    readerIndex = 0;
    writerIndex = 0;
}

void Buffer::RetrieveUntil(const char* end)
{
    assert(Peek() <= end);
    assert(end <= BeginWrite());
    Retrieve(end - Peek());
}

std::string Buffer::RetrieveAllAsString()
{
    return RetrieveAsString(ReadableBytes());
}

std::string Buffer::RetrieveAsString(size_t len)
{
    assert(len <= ReadableBytes());
    std::string res(Peek(), len);
    Retrieve(len);
    return res;
}

void Buffer::MakeSpace(size_t len)
{
    //剩余空间不足len，则直接扩大buffer大小
    if (WritableBytes() + PrependableBytes() < len)
    {
        buffer.resize(writerIndex + len + 1);
    }
    else//剩余空间大于等于len，则移动可读数据至最前面，腾出空间
    {
        size_t readable = ReadableBytes();
        std::copy(begin() + readerIndex, begin() + writerIndex, begin());
        readerIndex = 0;
        writerIndex = readerIndex + readable;
        assert(readable == ReadableBytes());
    }
}

void Buffer::EnsureWriteableBytes(size_t len)
{
    if(WritableBytes() < len)
    {
        MakeSpace(len);
    }
    assert(WritableBytes() >= len);
}

void Buffer::Append(const std::string& str)
{
    Append(str.data(), str.length());
}

void Buffer::Append(const char* str, size_t len)
{
    EnsureWriteableBytes(len);
    std::copy(str, str + len, BeginWrite());
    HasWritten(len);
}

void Buffer::Append(const void* data, size_t len)
{
    Append(static_cast<const char*>(data), len);
}

ssize_t Buffer::ReadFd(int fd, int* Errno)
{
    char extrabuf[65536];
    struct iovec vec[2];
    const size_t writable = WritableBytes();
    //分散读，buffer内的 writable字节（堆区）+ 固定的 extrabuf（栈区）
    vec[0].iov_base = begin() + writerIndex;
    vec[0].iov_len = writable;
    vec[1].iov_base = extrabuf;
    vec[1].iov_len = sizeof(extrabuf);
    //如果writable已经很大了，就无需将第二块内存分配出去
    const int iovcnt = (writable < sizeof(extrabuf)) ? 2 : 1;
    const ssize_t len = readv(fd, vec, iovcnt);
    if (len < 0)
    {
        *Errno = errno;
    }
    else if (static_cast<size_t>(len) <= writable)//长度未超过buffer的writable字节数
    {
        writerIndex += len;
    }
    else //长度超过buffer的writable字节数
    {
        writerIndex = buffer.size();
        Append(extrabuf, len - writable);
    }
    return len;
}