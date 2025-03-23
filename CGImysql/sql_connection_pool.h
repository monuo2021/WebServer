#ifndef _CONNECTION_POOL_
#define _CONNECTION_POOL_

#include <mysql/mysql.h>
#include <string>
#include <list>
#include <mutex>
#include <memory>
#include "../lock/locker.h"
#include "../log/log.h"

class connection_pool
{
public:
	//单例模式
	static connection_pool *GetInstance()
	{
		static connection_pool instance;
		return &instance;
	}

	MYSQL *GetConnection();				 //获取数据库连接
	bool ReleaseConnection(MYSQL *conn); //释放连接
	int GetFreeConn() const;					 //获取连接
	void DestroyPool();					 //销毁所有连接

	void init(std::string url, std::string User, std::string PassWord, std::string DataBaseName, int Port, int MaxConn, int close_log); 

private:
	connection_pool();
	~connection_pool();

	int m_MaxConn;  				// 最大连接数
	int m_CurConn;  				// 当前已使用的连接数
	int m_FreeConn; 				// 当前空闲的连接数
	mutable std::mutex mtx_;		// 互斥锁，保护共享资源
	std::list<MYSQL *> connList; 	// 数据库连接池
	sem reserve;					// 信号量，用于管理连接可用性

public:
	std::string m_url;				// 主机地址
	std::string m_Port;		 		// 数据库端口号
	std::string m_User;		 		// 登陆数据库用户名
	std::string m_PassWord;	 		// 登陆数据库密码
	std::string m_DatabaseName; 	// 使用数据库名
	int m_close_log;				// 是否关闭日志
};

class connectionRAII{

public:
	connectionRAII(MYSQL **con, connection_pool *connPool);
	~connectionRAII();
	
private:
	MYSQL *conRAII;
	connection_pool *poolRAII;
};

#endif
