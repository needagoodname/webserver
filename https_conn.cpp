#include "https_conn.h"
#include <cstdio>

const string cacert = "./cert/cert.pem";
const string key = "./cert/privkey.pem";
const string passwd = "123456";

bool https_conn::read() {
    printf("ssl read\n");
    if( m_read_idx >= READ_BUFFER_SIZE ) {
        return false;
    }
    int bytes_read = 0;
    while(true) {
        bytes_read = SSL_read(mSSL, m_read_buf + m_read_idx, READ_BUFFER_SIZE - m_read_idx);
        if (bytes_read <= 0) {
            unsigned long errnum = ERR_get_error();
            const char* errstr = ERR_reason_error_string(errnum);
            printf("errnum:%ld, %s\n", errnum, errstr);
            if( errno == EAGAIN || errno == EWOULDBLOCK ) {
                // 没有数据
                break;
            }
            return false;   
        } else if (bytes_read == 0) {   // 对方关闭连接
            return false;
        }
        m_read_idx += bytes_read;
    }
    return true;
}

bool https_conn::write() {
    int temp = 0;
    
    if ( bytes_to_send == 0 ) {
        // 将要发送的字节为0，这一次响应结束。
        modfd( m_epollfd, m_sockfd, EPOLLIN );
        init();
        return true;
    }

    printf("ssl write\n");
    while(1) {
        // 分散写
        for(int i=0;i<m_iv_count;++i) {
            temp = SSL_write(mSSL, m_iv[i].iov_base, m_iv[i].iov_len);

            if ( temp <= -1 ) {
                // 如果TCP写缓冲没有空间，则等待下一轮EPOLLOUT事件，虽然在此期间，
                // 服务器无法立即接收到同一客户的下一个请求，但可以保证连接的完整性。
                if( errno == EAGAIN ) {
                    modfd( m_epollfd, m_sockfd, EPOLLOUT );
                    return true;
                }
                unmap();
                return false;
            }
        }

        unmap();
        modfd(m_epollfd, m_sockfd, EPOLLIN);

        if (m_linger)
        {
            init();
            return true;
        } else {
            return false;
        }
    }
}

SSL_CTX* initSSL() {
    // SSL库初始化
    SSL_library_init();
    // 载入所有SSL算法
    OpenSSL_add_all_algorithms();
    // 载入SSL错误信息
    SSL_load_error_strings();
    SSL_CTX* ctx = SSL_CTX_new(SSLv23_server_method());
    if( !ctx ) {
        ERR_print_errors_fp(stdout);
        exit(1);
    }
    // 校验对方证书
    SSL_CTX_set_verify(ctx, SSL_VERIFY_FAIL_IF_NO_PEER_CERT, NULL);

    // 加载CA证书
    assert(SSL_CTX_load_verify_locations(ctx, cacert.c_str(), NULL));
    // 加载自己的证书
    assert(SSL_CTX_use_certificate_chain_file(ctx, cacert.c_str()) > 0);
    // 加载自己的私钥
    SSL_CTX_set_default_passwd_cb_userdata(ctx, (void*)passwd.c_str());
    assert(SSL_CTX_use_PrivateKey_file(ctx, key.c_str(), SSL_FILETYPE_PEM) > 0);
    // 判断私钥是否正确
    assert(SSL_CTX_check_private_key(ctx));
    return ctx;
}

void close_ssl(SSL* ssl) {
    if(ssl) {
        // 关闭SSL连接
        SSL_shutdown(ssl);
        // 释放SSL
        SSL_free(ssl);
    }
}