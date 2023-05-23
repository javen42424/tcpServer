// 优化后的 main.cpp
#include <cstdint>
#include <cstring>
#include <uv.h>
#include <iostream>
#include <string>
#include <signal.h>

// 定义一些常量
const int PORT = 8080; // 监听端口
// const char* RESPONSE = "HTTP/1.1 200 OK\r\nContent-Type: text/plain\r\nContent-Length: 12\r\n\r\nHello World!"; // 响应内容
const std::string RESPONSE = "HTTP/1.1 200 OK\r\nContent-Type: text/plain\r\n"; // 响应内容
const std::string CONTENT_LENGTH = "Content-Length: "; // 响应内容
const std::string CONTENT = "Hello World!"; // 响应内容

static uint32_t refreshNum = 0;

// 定义一个结构体，用于存储客户端连接的信息
struct client_t {
    uv_tcp_t handle; // TCP 句柄
    uv_write_t write_req; // 写请求
    std::string data; // 接收到的数据
};

// 定义一个全局变量，用于存储服务器的句柄
uv_tcp_t server;

// 定义一个回调函数，用于处理写请求的完成事件
void on_write_end(uv_write_t* req, int status) {
    if (status == -1) {
        std::cerr << "Write error: " << uv_strerror(status) << std::endl;
    }
    std::cout << "on_write_end  "  << std::endl;
    // 释放客户端连接的内存
    client_t* client = (client_t*) req->handle->data;
    uv_close((uv_handle_t*) &client->handle, NULL);
    delete client;
}

// 定义一个回调函数，用于处理读取数据的事件
void on_read(uv_stream_t* handle, ssize_t nread, const uv_buf_t* buf) {
    std::cout << "on_read 1  "  << std::endl;
    if (nread > 0) {
        std::cout << "on_read 2  "  << std::endl;
        // 将读取到的数据追加到客户端连接的 data 字段中
        client_t* client = (client_t*) handle->data;
        client->data.append(buf->base, nread);
        // 检查是否读取到了完整的 HTTP 请求
        if (client->data.find("\r\n\r\n") != std::string::npos) {
            std::cout << " client data : " << client->data << std::endl;
            // 如果是，就准备发送响应内容
            uv_buf_t resbuf;
            std::string str = CONTENT + std::to_string(refreshNum++);
            uint32_t contentSize = str.size();
            std::cout << " str : " << str << " ,  size : " << str.size() << std::endl;
            str = RESPONSE + CONTENT_LENGTH + std::to_string(contentSize)  + "\r\n\r\n" + str;
            const char* resp = str.c_str();
            resbuf.base = (char*) resp;
            resbuf.len = strlen(resp);
            // 发送写请求，并传入写请求完成的回调函数
            uv_write(&client->write_req, handle, &resbuf, 1, on_write_end);
        }
    } else if (nread < 0) {
        std::cout << "on_read 3  "  << std::endl;
        if (nread != UV_EOF) {
            std::cerr << "Read error: " << uv_err_name(nread) << std::endl;
        }
        // 释放客户端连接的内存
        client_t* client = (client_t*) handle->data;
        uv_close((uv_handle_t*) &client->handle, NULL);
        delete client;
    }
}

// 定义一个回调函数，用于分配缓冲区给读取数据的操作
void alloc_buffer(uv_handle_t* handle, size_t suggested_size, uv_buf_t* buf) {
    buf->base = new char[suggested_size];
    buf->len = suggested_size;
    std::cout << "alloc_buffer  "  << std::endl;
}

// 定义一个回调函数，用于处理新连接的事件
void on_new_connection(uv_stream_t* server, int status) {
    if (status == -1) {
        std::cerr << "New connection error: " << uv_strerror(status) << std::endl;
        return;
    }
    // 创建一个客户端连接的结构体，并初始化 TCP 句柄
    client_t* client = new client_t();
    uv_tcp_init(uv_default_loop(), &client->handle);
    client->handle.data = client;

    std::cout  << " server 1 " << server << " , client : " << &client->handle << std::endl;

    // 接受新连接，并开始读取数据
    if (uv_accept(server, (uv_stream_t*) &client->handle) == 0) {
        uv_read_start((uv_stream_t*) &client->handle, alloc_buffer, on_read);
        std::cout  << " server 2 " << server << " , client : " << &client->handle << std::endl;
    } else {
        // 如果接受失败，就关闭连接并释放内存
        uv_close((uv_handle_t*) &client->handle, NULL);
        std::cout  << " server 3 " << server << " , client : " << &client->handle << std::endl;
        delete client;
    }
}

// 定义一个回调函数，用于处理信号事件
void on_signal(uv_signal_t* handle, int signum) {
    std::cout << "Received signal: " << signum << std::endl;
    // 关闭服务器句柄，并停止事件循环
    uv_close((uv_handle_t*) &server, NULL);
    uv_stop(uv_default_loop());
    std::cout << "on_signal  "  << std::endl;
}

int main() {
    // 创建一个 TCP 服务器，并绑定到指定端口上
    uv_tcp_init(uv_default_loop(), &server);

    struct sockaddr_in addr;
    uv_ip4_addr("0.0.0.0", PORT, &addr);

    uv_tcp_bind(&server, (const struct sockaddr*)&addr, 0);
    
    // 监听新连接，并传入新连接事件的回调函数
    int r = uv_listen((uv_stream_t*) &server, 128, on_new_connection);
    if (r) {
        std::cerr << "Listen error: " << uv_strerror(r) << std::endl;
        return 1;
    }

    // 创建一个信号处理器，并注册 SIGINT 和 SIGTERM 信号
    uv_signal_t sigint_handler;
    uv_signal_init(uv_default_loop(), &sigint_handler);
    uv_signal_start(&sigint_handler, on_signal, SIGINT);

    uv_signal_t sigterm_handler;
    uv_signal_init(uv_default_loop(), &sigterm_handler);
    uv_signal_start(&sigterm_handler, on_signal, SIGTERM);

#ifdef _WIN32
	// 在 Windows 上隐藏控制台窗口，使程序后台运行
	ShowWindow(GetConsoleWindow(), SW_HIDE);
#endif

	// 启动事件循环
	return uv_run(uv_default_loop(), UV_RUN_DEFAULT);
}
