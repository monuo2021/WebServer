#ifndef HTTPCONNECTION_H
#define HTTPCONNECTION_H
#include <unistd.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/epoll.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <assert.h>
#include <sys/stat.h>
#include <string.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <stdarg.h>
#include <errno.h>
#include <sys/wait.h>
#include <sys/uio.h>
#include <map>

#include "../lock/locker.h"
#include "../CGImysql/sql_connection_pool.h"
#include "../timer/lst_timer.h"
#include "../log/log.h"

class http_conn
{
public:
    static const int FILENAME_LEN = 200;            // 文件名的最大长度（200）
    static const int READ_BUFFER_SIZE = 2048;       // 读取缓冲区的大小（2048字节）
    static const int WRITE_BUFFER_SIZE = 1024;      // 写入缓冲区的大小（1024字节）
    enum METHOD
    {
        GET = 0,
        POST,
        HEAD,
        PUT,
        DELETE,
        TRACE,
        OPTIONS,
        CONNECT,
        PATH
    };
    enum CHECK_STATE
    {
        CHECK_STATE_REQUESTLINE = 0,
        CHECK_STATE_HEADER,
        CHECK_STATE_CONTENT
    };
    enum HTTP_CODE
    {
        NO_REQUEST,             // 请求不完整，需要继续读取请求报文数据；跳转主线程继续监测读事件
        GET_REQUEST,            // 请求完整，调用do_request完成请求资源映射
        BAD_REQUEST,            // HTTP请求报文有语法错误或请求资源为目录；跳转process_write完成响应报文
        NO_RESOURCE,            // 请求资源不存在；跳转process_write完成响应报文
        FORBIDDEN_REQUEST,      // 请求资源没有访问权限；跳转process_write完成响应报文
        FILE_REQUEST,           // 请求资源有效，跳转process_write完成响应报文
        INTERNAL_ERROR,         // 服务器内部错误，该结果在主状态机逻辑switch的default下，一般不会触发
        CLOSED_CONNECTION
    };
    enum LINE_STATUS
    {
        LINE_OK = 0,
        LINE_BAD,
        LINE_OPEN
    };

public:
    http_conn() {}
    ~http_conn() {}

public:
    void init(int sockfd, const sockaddr_in &addr, char *, int, int, string user, string passwd, string sqlname);
    void close_conn(bool real_close = true);
    void process();
    bool read_once();
    bool write();
    sockaddr_in *get_address()
    {
        return &m_address;
    }
    void initmysql_result(connection_pool *connPool);
    int timer_flag;     // 用于标记连接是否超时
    int improv;         // 标记连接是否需要改进（例如，是否需要执行某些额外操作，如超时处理、状态调整等）


private:
    void init();
    HTTP_CODE process_read();
    bool process_write(HTTP_CODE ret);
    HTTP_CODE parse_request_line(char *text);
    HTTP_CODE parse_headers(char *text);
    HTTP_CODE parse_content(char *text);
    HTTP_CODE do_request();
    char *get_line() { return m_read_buf + m_start_line; };
    LINE_STATUS parse_line();
    void unmap();
    bool add_response(const char *format, ...);
    bool add_content(const char *content);
    bool add_status_line(int status, const char *title);
    bool add_headers(int content_length);
    bool add_content_type();
    bool add_content_length(int content_length);
    bool add_linger();
    bool add_blank_line();

public:
    static int m_epollfd;
    static int m_user_count;
    MYSQL *mysql;
    int m_state;                                // 读为0, 写为1

private:
    int m_sockfd;                               // 客户端的 socket 文件描述符
    sockaddr_in m_address;                      // 客户端地址
    char m_read_buf[READ_BUFFER_SIZE];          // 读缓冲区
    long m_read_idx;                            // 当前已经读入缓冲区的数据的最后一个字节的下一个位置
    long m_checked_idx;                         // 当前正在分析的字符在读缓冲区中的位置
    int m_start_line;                           // 当前正在解析的行的起始位置
    char m_write_buf[WRITE_BUFFER_SIZE];        // 写缓冲区
    int m_write_idx;                            // 当前写缓冲区中已经读入的字符个数
    CHECK_STATE m_check_state;                  // 主状态机当前所处的状态
    METHOD m_method;                            // 请求方法
    char m_real_file[FILENAME_LEN];             // 请求文件的完整路径， doc_root + m_url
    char *m_url;                                // 请求的 URL
    char *m_version;                            // HTTP 版本
    char *m_host;                               // Host 头
    long m_content_length;                      // 请求内容的长度
    bool m_linger;                              // 是否保持连接
    char *m_file_address;                       // 映射到内存中的文件地址
    struct stat m_file_stat;                    // 文件状态
    struct iovec m_iv[2];                       // 数据的结构体，用于写操作
    int m_iv_count;                             // iovec 数组的大小
    int cgi;                                    // 是否是 POST 请求
    char *m_string;                             // 存储请求头数据
    int bytes_to_send;                          // 需要发送的数据总长度
    int bytes_have_send;                        // 已经发送的字节数
    char *doc_root;                             // 网站根目录

    map<string, string> m_users;                // 保存所有用户名和密码
    int m_TRIGMode;                             // 触发模式
    int m_close_log;                            // 是否关闭日志

    char sql_user[100];                         // 数据库用户名
    char sql_passwd[100];                       // 数据库密码
    char sql_name[100];                         // 数据库名
};

#endif
