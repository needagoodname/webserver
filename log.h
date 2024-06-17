#ifndef LOG_H
#define LOG_H

#include <string>
#include <thread>
#include <sys/time.h>
#include <string.h>
#include <stdarg.h>           // vastart va_end
#include <assert.h>
#include <sys/stat.h>         //mkdir
#include <memory>
#include "blockqueue.h"
// #include "buffer.h"

class Log {
public:
    // 初始化日志实例
    void init(int level, const char* path = "./log", 
                const char* suffix =".log",
                int maxQueueCapacity = 1024);

    // 获得日志实例
    static Log& Instance();

    // 写日志公有方法，调用AsyncWrite_
    static void FlushLogThread();

    // 将输出内容按照格式整理
    void WriteLog(int level, const char *format,...);
    void flush();

    int GetLevel();
    void SetLevel(int level);
    bool IsOpen() { return isOpen_; }
    
private:
    Log();
    Log(const Log &l) = delete;
    const Log &operator=(const Log &l) = delete;
    void AppendLogLevelTitle_(int level);
    virtual ~Log();

    // 异步写日志方法
    void AsyncWrite_();

private:
    static const int LOG_PATH_LEN = 256;
    static const int LOG_NAME_LEN = 256; // 最大日志文件名称长度
    static const int MAX_LINES = 50000; // 最多日志记录数目

    const char* path_; //路径名
    const char* suffix_; //文件后缀名

    // int MAX_LINES_;

    int lineCount_; // 日志记录行数
    int toDay_; // 按天区分文件

    bool isOpen_;
 
    // Buffer buff_;
    int level_;
    bool isAsync_; //是否异步

    FILE* fp_; // 打开log的文件指针
    std::unique_ptr<BlockDeque<std::string>> deque_; // 阻塞队列
    std::unique_ptr<std::thread> writeThread_;
    std::mutex mtx_; // 同步日志互斥量
};

#define LOG_BASE(level, format, ...) \
    do {\
        Log* log = Log::Instance();\
        if (log->IsOpen() && log->GetLevel() <= level) {\
            log->WriteLog(level, format, ##__VA_ARGS__); \
            log->flush();\
        }\
    } while(0);

#define LOG_DEBUG(format, ...) do {LOG_BASE(0, format, ##__VA_ARGS__)} while(0);
#define LOG_INFO(format, ...) do {LOG_BASE(1, format, ##__VA_ARGS__)} while(0);
#define LOG_WARN(format, ...) do {LOG_BASE(2, format, ##__VA_ARGS__)} while(0);
#define LOG_ERROR(format, ...) do {LOG_BASE(3, format, ##__VA_ARGS__)} while(0);

#endif //LOG_H