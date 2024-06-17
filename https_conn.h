#ifndef HTTPS
#define HTTPS

#include "http_conn.h"
#include <openssl/ssl.h>
#include <openssl/err.h>
using std::string;

SSL_CTX* initSSL();

void close_ssl(SSL* ssl);

class https_conn : public http_conn{
public:
    bool read();
    bool write();
    SSL* mSSL;
};

#endif