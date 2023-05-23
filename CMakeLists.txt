# 修改后的 cmake 配置文件
cmake_minimum_required(VERSION 3.14)

# 设置项目名称和版本
project(http_server VERSION 1.0)

# 设置 C++ 标准
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED True)

# 使用 FetchContent 模块来下载 libuv
include(FetchContent)
FetchContent_Declare(
  libuv
  GIT_REPOSITORY https://github.com/libuv/libuv.git
  GIT_TAG        v1.x
)
FetchContent_MakeAvailable(libuv)

# 添加可执行文件
add_executable(http_server main.cpp)

# 链接 libuv 库
target_link_libraries(http_server PRIVATE uv_a)

# 添加 libuv 的头文件目录到编译器的搜索路径中
target_include_directories(http_server PRIVATE ${libuv_SOURCE_DIR}/include)

# 关闭编译优化，设置编译标志为 -O0
target_compile_options(http_server PRIVATE -O0 -g)
