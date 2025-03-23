#include "sql_connection_pool.h"
#include <stdexcept>
#include <iostream>

connection_pool::connection_pool() : m_MaxConn(0), m_CurConn(0), m_FreeConn(0) {}

//构造初始化
void connection_pool::init(std::string url, std::string User, std::string PassWord, std::string DBName, int Port, int MaxConn, int close_log)
{
	m_url = std::move(url);
	m_Port = std::to_string(Port);
	m_User = std::move(User);
	m_PassWord = std::move(PassWord);
	m_DatabaseName = std::move(DBName);
	m_close_log = (close_log);

	if (MaxConn <= 0) {
        throw std::invalid_argument("MaxConn must be positive");
    }

	for (int i = 0; i < MaxConn; i++) {
		MYSQL* con = mysql_init(nullptr);

		if (!con)
		{
			LOG_ERROR("Failed to initialize MySQL connection");
            throw std::runtime_error("MySQL initialization failed");
		}
		con = mysql_real_connect(con, m_url.c_str(), m_User.c_str(), m_PassWord.c_str(), m_DatabaseName.c_str(), Port, NULL, 0);

		if (!con) {
            LOG_ERROR("MySQL connection failed: %s", mysql_error(con));
            mysql_close(con);
            throw std::runtime_error("MySQL connection failed");
        }
		connList.push_back(con);
		++m_FreeConn;
	}

	m_MaxConn = m_FreeConn;
	reserve = sem(m_FreeConn);
}


//当有请求时，从数据库连接池中返回一个可用连接，更新使用和空闲连接数
MYSQL *connection_pool::GetConnection()
{
	if (connList.empty()) {
        LOG_WARN("No available connections in pool");
        return nullptr;
    }

	reserve.wait();
	std::lock_guard<std::mutex> lock(mtx_);

	MYSQL *con = connList.front();
	connList.pop_front();

	--m_FreeConn;
	++m_CurConn;

	return con;
}

//释放当前使用的连接
bool connection_pool::ReleaseConnection(MYSQL *con)
{
	if (!con)
		return false;

	{
		std::lock_guard<std::mutex> lock(mtx_);

		connList.push_back(con);
		++m_FreeConn;
		--m_CurConn;
	}

	reserve.post();
	return true;
}

//销毁数据库连接池
void connection_pool::DestroyPool()
{
	std::lock_guard<std::mutex> lock(mtx_);
	if (connList.size() > 0)
	{
		std::list<MYSQL *>::iterator it;
		for (it = connList.begin(); it != connList.end(); ++it)
		{
			MYSQL *con = *it;
			mysql_close(con);
		}
		m_CurConn = 0;
		m_FreeConn = 0;
		connList.clear();
	}
}

//当前空闲的连接数
int connection_pool::GetFreeConn() const
{
	std::lock_guard<std::mutex> lock(mtx_);
	return this->m_FreeConn;
}

connection_pool::~connection_pool()
{
	DestroyPool();
}

connectionRAII::connectionRAII(MYSQL **SQL, connection_pool *connPool){
	*SQL = connPool->GetConnection();
	
	conRAII = *SQL;
	poolRAII = connPool;
}

connectionRAII::~connectionRAII(){
	if(conRAII) {
		poolRAII->ReleaseConnection(conRAII);
	}
}