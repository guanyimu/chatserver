cmake_minimum_required(VERSION 3.0)
project(main)

# 配置编译选项
set(CMAKE_CXX_FLAGS ${CMAKE_CXX_FLAGE} -g)



# 设置可执行文件最终存储的路径
# set(EXECUTABLE_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/bin)



# 配置头文件搜索路径
# include_directories()

# 配置库文件搜索路径
# link_directories()

# 表示生成可执行文件server
aux_source_directory(. SRC_LIST)
add_executable(server ${SRC_LIST})

# 添加需要链接的库文件
target_link_libraries(server muduo_net muduo_base pthread)