#include "http_conn.h"

// 定义HTTP响应的一些状态信息
const char *ok_200_title = "OK";
const char *error_400_title = "Bad Request";
const char *error_400_form = "Your request has bad syntax or is inherently impossible to satisfy.\n";
const char *error_403_title = "Forbidden";
const char *error_403_form = "You do not have permission to get file from this server.\n";
const char *error_404_title = "Not Found";
const char *error_404_form = "The requested file was not found on this server.\n";
const char *error_500_title = "Internal Error";
const char *error_500_form = "There was an unusual problem serving the requested file.\n";

// 网站的根目录
const char *doc_root = "/home/try/demo/resources";

int setnonblocking(int fd)
{
    int old_option = fcntl(fd, F_GETFL);
    int new_option = old_option | O_NONBLOCK;
    fcntl(fd, F_SETFL, new_option);
    return old_option;
}

// 向epoll中添加需要监听的文件描述符
void addfd(int epollfd, int fd, bool one_shot)
{
    epoll_event event;
    event.data.fd = fd;
    event.events = EPOLLIN | EPOLLRDHUP;
    if (one_shot)
    {
        // 防止同一个通信被不同的线程处理
        event.events |= EPOLLONESHOT;
    }
    epoll_ctl(epollfd, EPOLL_CTL_ADD, fd, &event);
    // 设置文件描述符非阻塞
    setnonblocking(fd);
}

// 从epoll中移除监听的文件描述符
void removefd(int epollfd, int fd)
{
    epoll_ctl(epollfd, EPOLL_CTL_DEL, fd, 0);
    close(fd);
}

// 修改文件描述符，重置socket上的EPOLLONESHOT事件，以确保下一次可读时，EPOLLIN事件能被触发
void modfd(int epollfd, int fd, int ev)
{
    epoll_event event;
    event.data.fd = fd;
    event.events = ev | EPOLLET | EPOLLONESHOT | EPOLLRDHUP;
    epoll_ctl(epollfd, EPOLL_CTL_MOD, fd, &event);
}

// 所有的客户数
int http_conn::m_user_count = 0;
// 所有socket上的事件都被注册到同一个epoll内核事件中，所以设置成静态的
int http_conn::m_epollfd = -1;

// 关闭连接
void http_conn::close_conn()
{
    if (m_sockfd != -1)
    {
        removefd(m_epollfd, m_sockfd);
        m_sockfd = -1;
        --m_user_count; // 关闭一个连接，将客户总数量-1
    }
}

// 初始化连接,外部调用初始化套接字地址
void http_conn::init(int sockfd, const sockaddr_in &addr, time_t t)
{
    m_sockfd = sockfd;
    m_address = addr;
    last_active = t;

    // 端口复用
    int reuse = 1;
    setsockopt(m_sockfd, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse));
    addfd(m_epollfd, sockfd, true);
    ++m_user_count;
    init();
}

void http_conn::init()
{

    m_check_state = CHECK_STATE_REQUESTLINE; // 初始状态为检查请求行
    m_linger = false;                        // 默认不保持链接  Connection : keep-alive保持连接

    m_method = GET; // 默认请求方式为GET
    m_content_length = 0;
    m_start_line = 0;
    m_checked_idx = 0;
    bzero(m_real_file, FILENAME_LEN);
}

// 循环读取客户数据，直到无数据可读或者对方关闭连接
bool http_conn::readFd()
{
    int bytes_read = 0, Errno = 0;
    while (true)
    {
        bytes_read = m_read_buf.ReadFd(m_sockfd, &Errno);
        // printf("bytes_read:%d\n", bytes_read);
        if (bytes_read == -1)
        {
            if (Errno == EAGAIN || Errno == EWOULDBLOCK)
            {
                break;
            }
            return false;
        }
        else if (bytes_read == 0)
        {
            return false;
        }
    }
    return true;
}

// 解析一行，判断依据\r\n
http_conn::LINE_STATUS http_conn::parse_line()
{
    char temp;
    for (m_checked_idx = 0; m_checked_idx <= m_read_buf.ReadableBytes(); ++m_checked_idx)
    {
        temp = m_read_buf.GetPsn(m_checked_idx);
        if (temp == '\r')
        {
            if (m_checked_idx + 1 == m_read_buf.ReadableBytes())
            {
                return LINE_OPEN;
            }
            else if (m_read_buf.GetPsn(m_checked_idx + 1) == '\n')
            {
                m_read_buf.SetPsn('\0', m_checked_idx);
                m_read_buf.SetPsn('\0', 1 + m_checked_idx);
                m_checked_idx += 1;
                return LINE_OK;
            }
            return LINE_BAD;
        }
        else if (temp == '\n')
        {
            if ((m_checked_idx < m_read_buf.ReadableBytes()) && (m_read_buf.GetPsn(-1) == '\r'))
            {
                m_read_buf.SetPsn('\0', m_checked_idx - 1);
                m_read_buf.SetPsn('\0', m_checked_idx);
                // m_checked_idx += 1;
                return LINE_OK;
            }
            return LINE_BAD;
        }
    }
    return LINE_OPEN;
}

// 解析HTTP请求行，获得请求方法，目标URL,以及HTTP版本号
http_conn::HTTP_CODE http_conn::parse_request_line(char *text)
{
    // GET /index.html HTTP/1.1
    char *_url = strpbrk(text, " \t"); // 判断第二个参数中的字符哪个在text中最先出现
    if (!_url)
    {
        return BAD_REQUEST;
    }
    // GET\0/index.html HTTP/1.1
    *_url++ = '\0'; // 置位空字符，字符串结束符
    char *method = text;
    if (strcasecmp(method, "GET") == 0)
    { // 忽略大小写比较
        m_method = GET;
    }
    else
    {
        return BAD_REQUEST;
    }
    // /index.html HTTP/1.1
    // 检索字符串 str1 中第一个不在字符串 str2 中出现的字符下标。
    char *_version = strpbrk(_url, " \t");
    if (!_version)
    {
        return BAD_REQUEST;
    }
    *_version++ = '\0';
    if (strcasecmp(_version, "HTTP/1.1") != 0)
    {
        return BAD_REQUEST;
    }

    if (strncasecmp(_url, "http://", 7) == 0)
    {
        _url += 7;
        // 在参数 str 所指向的字符串中搜索第一次出现字符 c（一个无符号字符）的位置。
        _url = strchr(_url, '/');
    }
    if (!_url || _url[0] != '/')
    {
        return BAD_REQUEST;
    }
    m_url = string(_url);
    m_version = string(_version);
    m_check_state = CHECK_STATE_HEADER; // 检查状态变成检查头
    return NO_REQUEST;
}

// 解析HTTP请求的一个头部信息
http_conn::HTTP_CODE http_conn::parse_headers(char *text)
{
    // 遇到空行，表示头部字段解析完毕
    if (text[0] == '\0')
    {
        // 如果HTTP请求有消息体，则还需要读取m_content_length字节的消息体，
        // 状态机转移到CHECK_STATE_CONTENT状态
        if (m_read_buf.ReadableBytes() != 0)
        {
            m_check_state = CHECK_STATE_CONTENT;
            return NO_REQUEST;
        }
        // 否则说明我们已经得到了一个完整的HTTP请求
        return GET_REQUEST;
    }
    else if (strncasecmp(text, "Connection:", 11) == 0)
    {
        // 处理Connection 头部字段  Connection: keep-alive
        text += 11;
        text += strspn(text, " \t");
        if (strcasecmp(text, "keep-alive") == 0)
        {
            m_linger = true;
        }
    }
    else if (strncasecmp(text, "Content-Length:", 15) == 0)
    {
        // 处理Content-Length头部字段
        text += 15;
        text += strspn(text, " \t");
        m_content_length = atol(text);
    }
    else if (strncasecmp(text, "Host:", 5) == 0)
    {
        // 处理Host头部字段
        text += 5;
        text += strspn(text, " \t");
        m_host = string(text);
    }
    else
    {
        // printf( "oop! unknow header %s\n", text );
    }
    return NO_REQUEST;
}

// 我们没有真正解析HTTP请求的消息体，只是判断它是否被完整的读入了
http_conn::HTTP_CODE http_conn::parse_content(char *text)
{
    if (m_read_buf.ReadableBytes() == 0)
    {
        // m_read_buf.SetPsn('\0', 1);
        return GET_REQUEST;
    }
    return NO_REQUEST;
}

// 主状态机，解析请求
http_conn::HTTP_CODE http_conn::process_read()
{
    LINE_STATUS line_status = LINE_OK;
    HTTP_CODE ret = NO_REQUEST;
    while (((m_check_state == CHECK_STATE_CONTENT) && (line_status == LINE_OK)) || ((line_status = parse_line()) == LINE_OK))
    {
        // 获取一行数据
        std::string line(get_line());
        char *t = const_cast<char *>(line.c_str());

        switch (m_check_state)
        {
        case CHECK_STATE_REQUESTLINE:
        {
            ret = parse_request_line(t);
            if (ret == BAD_REQUEST)
            {
                return BAD_REQUEST;
            }
            break;
        }
        case CHECK_STATE_HEADER:
        {
            ret = parse_headers(t);
            if (ret == BAD_REQUEST)
            {
                return BAD_REQUEST;
            }
            else if (ret == GET_REQUEST)
            {
                return do_request();
            }
            break;
        }
        case CHECK_STATE_CONTENT:
        {
            ret = parse_content(t);
            if (ret == GET_REQUEST)
            {
                return do_request();
            }
            line_status = LINE_OPEN;
            break;
        }
        default:
        {
            return INTERNAL_ERROR;
        }
        }
    }
    return NO_REQUEST;
}

// 当得到一个完整、正确的HTTP请求时，我们就分析目标文件的属性，
// 如果目标文件存在、对所有用户可读，且不是目录，则使用mmap将其
// 映射到内存地址m_file_address处，并告诉调用者获取文件成功
http_conn::HTTP_CODE http_conn::do_request()
{
    // "/home/try/demo/resources"
    strcpy(m_real_file, doc_root);
    int len = strlen(doc_root);
    strncpy(m_real_file + len, m_url.c_str(), FILENAME_LEN - len - 1);
    // 获取m_real_file文件的相关的状态信息，-1失败，0成功
    if (stat(m_real_file, &m_file_stat) < 0)
    {
        return NO_RESOURCE;
    }

    // 判断访问权限
    if (!(m_file_stat.st_mode & S_IROTH))
    {
        return FORBIDDEN_REQUEST;
    }

    // 判断是否是目录
    if (S_ISDIR(m_file_stat.st_mode))
    {
        return BAD_REQUEST;
    }

    // 以只读方式打开文件
    int fd = open(m_real_file, O_RDONLY);
    // 创建内存映射
    m_file_address = (char *)mmap(0, m_file_stat.st_size, PROT_READ, MAP_PRIVATE, fd, 0);
    close(fd);
    return FILE_REQUEST;
}

// 对内存映射区执行munmap操作
void http_conn::unmap()
{
    if (m_file_address)
    {
        munmap(m_file_address, m_file_stat.st_size);
        m_file_address = nullptr;
    }
}

// 写HTTP响应
bool http_conn::writeFd()
{
    int temp = 0;

    while (1)
    {
        // 分散写
        temp = write(m_sockfd, m_write_buf.Peek(), m_write_buf.ReadableBytes());

        if (temp == -1)
        {
            // 如果TCP写缓冲没有空间，则等待下一轮EPOLLOUT事件，虽然在此期间，
            // 服务器无法立即接收到同一客户的下一个请求，但可以保证连接的完整性。
            if (errno == EAGAIN)
            {
                cout << "out" << endl;
                modfd(m_epollfd, m_sockfd, EPOLLOUT);
                return true;
            }
            unmap();
            return false;
        }

        if (temp == 0)
        {
            // 没有数据要发送了
            // int flags = fcntl(m_sockfd, F_GETFL, 0);
            // fcntl(m_sockfd, F_SETFL, flags&~O_NONBLOCK);
            if (m_file_address)
                write(m_sockfd, m_file_address, strlen(m_file_address));
            unmap();
            modfd(m_epollfd, m_sockfd, EPOLLIN);

            std::cout<<m_url<<" write end"<<std::endl;
            if (m_linger)
            {
                init();
                return true;
            }
            else
            {
                return false;
            }
        }

        m_write_buf.Retrieve(temp);
    }
    return true;
}

bool http_conn::add_status_line(int status, const char *title)
{
    return add_response("HTTP/1.1 ", std::to_string(status).c_str(), " ", title, "\r\n");
}

bool http_conn::add_headers(int content_len)
{
    add_content_length(content_len);
    add_content_type();
    add_linger();
    add_blank_line();
    return true;
}

bool http_conn::add_content_length(int content_len)
{
    return add_response("Content-Length: ", std::to_string(content_len).c_str(), "\r\n");
}

bool http_conn::add_linger()
{
    return add_response("Connection: ", (m_linger == true) ? "keep-alive" : "close", "\r\n");
}

bool http_conn::add_blank_line()
{
    return add_response("\r\n");
}

bool http_conn::add_content(const char *content)
{
    return add_response(content);
}

bool http_conn::add_content_type()
{
    if(m_url[2]=='n') return add_response("Content-Type:text/html\r\n");
    else return add_response("Content-Type:image/jpg\r\n");
}

// 根据服务器处理HTTP请求的结果，决定返回给客户端的内容
bool http_conn::process_write(HTTP_CODE ret)
{
    switch (ret)
    {
    case INTERNAL_ERROR:
        add_status_line(500, error_500_title);
        add_headers(strlen(error_500_form));
        if (!add_content(error_500_form))
        {
            return false;
        }
        break;
    case BAD_REQUEST:
        add_status_line(400, error_400_title);
        add_headers(strlen(error_400_form));
        if (!add_content(error_400_form))
        {
            return false;
        }
        break;
    case NO_RESOURCE:
        add_status_line(404, error_404_title);
        add_headers(strlen(error_404_form));
        if (!add_content(error_404_form))
        {
            return false;
        }
        break;
    case FORBIDDEN_REQUEST:
        add_status_line(403, error_403_title);
        add_headers(strlen(error_403_form));
        if (!add_content(error_403_form))
        {
            return false;
        }
        break;
    case FILE_REQUEST:
        add_status_line(200, ok_200_title);
        add_headers(m_file_stat.st_size);
        return true;
    default:
        return false;
    }
}

// 由线程池中的工作线程调用，这是处理HTTP请求的入口函数
void http_conn::process()
{
    // 解析HTTP请求
    HTTP_CODE read_ret = process_read();
    if (read_ret == NO_REQUEST)
    {
        modfd(m_epollfd, m_sockfd, EPOLLIN);
        return;
    }

    // 生成响应，需确定关连接与发送响应顺序
    bool write_ret = process_write(read_ret);
    modfd(m_epollfd, m_sockfd, EPOLLOUT);
    if (!write_ret)
    {
        cout<<m_url<<endl;
        close_conn();
    }
}

int http_conn::getfd() const
{
    return m_sockfd;
}