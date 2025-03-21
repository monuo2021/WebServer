#include "config.h"

int main(int argc, char *argv[])
{
    try {
        // 从环境变量中读取数据库信息，避免硬编码敏感信息
        const char* user_env = std::getenv("WEBSERVER_DB_USER");
        const char* passwd_env = std::getenv("WEBSERVER_DB_PASSWD");
        const char* databasename_env = std::getenv("WEBSERVER_DB_NAME");

        if (!user_env || !passwd_env || !databasename_env) {
            throw std::runtime_error("数据库信息未正确配置，请设置环境变量 WEBSERVER_DB_USER、WEBSERVER_DB_PASSWD 和 WEBSERVER_DB_NAME");
        }

        std::string user = user_env;
        std::string passwd = passwd_env;
        std::string databasename = databasename_env;

        //命令行解析
        Config config;
        config.parse_arg(argc, argv);

        WebServer server;

        //初始化
        server.init(config.getPort(), user, passwd, databasename, config.getLOGWrite(), 
                    config.getOPTLINGER(), config.getTRIGMode(),  config.getSqlNum(),  config.getThreadNum(), 
                    config.getCloseLog(), config.getActorModel());
        

        // 日志
        server.log_write();

        // 数据库
        server.sql_pool();

        // 线程池
        server.thread_pool();

        // 触发模式
        server.trig_mode();

        // 监听
        server.eventListen();

        // 运行
        server.eventLoop();

    } catch(const std::exception& e) {
        // 捕获异常并输出错误信息
        std::cerr << "程序运行时发生错误: " << e.what() << std::endl;
        return EXIT_FAILURE;
    }
    
    return EXIT_SUCCESS;
}