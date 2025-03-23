# 设置 CMake 最低版本要求
cmake_minimum_required(VERSION 3.10)

# 项目名称
project(WebServer)

# 设置 C++ 标准
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 定义 DEBUG 变量，默认值为 1
option(DEBUG "Build with debug information" ON)

# 根据 DEBUG 设置编译选项
if(DEBUG)
    add_compile_options(-g)
else()
    add_compile_options(-O2)
endif()

# 添加源文件
set(SOURCES
    main.cpp
    timer/lst_timer.cpp
    http/http_conn.cpp
    log/log.cpp
    sqlConnectionPool/sqlConnectionPool.cpp
    webserver.cpp
    config.cpp
)

# 创建可执行文件
add_executable(server ${SOURCES})

# 链接库
target_link_libraries(server PRIVATE
    pthread
    mysqlclient
)

# 可选：设置输出目录
set_target_properties(server PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/.."
)

# 添加 clean 目标（CMake 自带 clean 目标，这里只是说明）
# make clean 在 CMake 中是内置的，使用 "cmake --build . --target clean"