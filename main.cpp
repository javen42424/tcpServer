// 引入 libuv 和 llhttp 的头文件
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <llhttp.h>
#include <string>
#include <uv.h>

// 定义一个最大长度为 2048 的缓冲区
#define MAX_LEN 2048

// 定义一个结构体，用于存储每个连接的状态和信息
typedef struct {
  uv_tcp_t tcp;          // libuv 的 TCP 句柄
  uv_write_t write_req;  // 写请求
  llhttp_t parser;       // llhttp 的解析器
  char request[MAX_LEN]; // 存储请求报文
  int request_len;       // 存储请求报文的长度
} client_t;

// 定义一个全局变量，用于存储 libuv 的事件循环
uv_loop_t *loop;

// 定义一个回调函数，用于处理写请求的完成事件
void on_write_end(uv_write_t *req, int status) {
  if (status == -1) {
    std::cerr << "Write error: " << uv_strerror(status) << std::endl;
  }
  std::cout << "on_write_end  1 " << std::endl;
  // 释放客户端连接的内存
  client_t *client = (client_t *)req->handle->data;

  std::cout << "on_write_end  2 " << std::endl;

  uv_close((uv_handle_t *)&client->tcp, NULL);

  std::cout << "on_write_end 3 " << std::endl;
  delete client;
}

// 定义一个回调函数，用于在分配读取缓冲区时使用
void alloc_buffer(uv_handle_t *handle, size_t suggested_size, uv_buf_t *buf) {
  // 分配一个固定大小的缓冲区，并将其指针和长度存储到 buf 结构体中
  buf->base = new char[MAX_LEN];
  buf->len = MAX_LEN;
}

// 定义一个回调函数，用于在读取数据时使用
void on_read(uv_stream_t *stream, ssize_t nread, const uv_buf_t *buf) {
  // 获取 client_t 结构体的指针
  client_t *client = (client_t *)stream;

  std::cout << "on_read 1 " << std::endl;

  // 检查是否有错误发生
  if (nread < 0) {
    std::cout << "on_read 2 " << std::endl;
    // 如果是因为连接断开，则关闭连接并释放资源
    if (nread == UV_EOF) {
      std::cout << "on_read 3 " << std::endl;
      uv_close((uv_handle_t *)stream, NULL);
      std::cout << "on_read 4 " << std::endl;
      free(client);
    }
    // 否则打印错误信息
    else {
      std::cout << "on_read 5 " << std::endl;
      fprintf(stderr, "Read error: %s\n", uv_strerror(nread));
    }
    std::cout << "on_read 6 " << std::endl;
    // 释放缓冲区内存
    free(buf->base);
    std::cout << "on_read 7 " << std::endl;
    return;
  }

  // 检查是否有数据可读
  if (nread > 0) {
    std::cout << "on_read 8 " << std::endl;
    // 调用 llhttp_execute 函数，将读取到的数据传递给 llhttp 解析器
    enum llhttp_errno err = llhttp_execute(&client->parser, buf->base, nread);

    // 检查是否有解析错误发生，如果有，则打印错误信息和原因，并关闭连接
    if (err != HPE_OK) {
      std::cout << "on_read 9 " << std::endl;
      fprintf(stderr, "Parse error: %s %s\n", llhttp_errno_name(err),
              client->parser.reason);
      std::cout << "on_read 10 " << std::endl;
      uv_close((uv_handle_t *)stream, NULL);
      std::cout << "on_read 11 " << std::endl;
      free(client);
      free(buf->base);
      std::cout << "on_read 12 " << std::endl;
      return;
    }

    // 将读取到的数据追加到请求报文缓冲区中，并更新长度
    memcpy(client->request + client->request_len, buf->base, nread);
    client->request_len += nread;

    std::cout << "on_read 13 " << std::endl;

    // 如果解析完成，则发送响应报文给客户端，并关闭连接
    if (llhttp_finish(&client->parser) == HPE_OK) {
      std::cout << "on_read 14 " << std::endl;
      uv_buf_t res;
      res.base = client->request;
      res.len = client->request_len;
      uv_write_t req;
      uv_write(&req, stream, &res, 1, NULL);
      std::cout << "on_read 15 " << " type : " << (uv_handle_t *)stream->type << std::endl;
      uv_close((uv_handle_t *)stream, NULL);
      free(client);
      std::cout << "on_read 16 " << std::endl;
    }
  }
  std::cout << "on_read 17 " << std::endl;
  // 释放缓冲区内存
  free(buf->base);
}

// 定义一个回调函数，用于在接受客户端连接时使用
void on_connection(uv_stream_t *server, int status) {
  std::cout << "on_connection 1 " << std::endl;
  // 检查是否有错误发生
  if (status < 0) {
    std::cout << "on_connection 2 " << std::endl;
    fprintf(stderr, "Connection error: %s\n", uv_strerror(status));
    return;
  }
  std::cout << "on_connection 3 " << std::endl;
  // 分配一个 client_t 结构体的内存，并初始化其内容
  client_t *client = (client_t *)malloc(sizeof(client_t));
  memset(client, 0, sizeof(client_t));

  std::cout << "on_connection 4 " << std::endl;

  // 初始化 libuv 的 TCP 句柄，并将其与事件循环关联
  uv_tcp_init(loop, &client->tcp);

  std::cout << "on_connection 5 " << std::endl;

  // 初始化 llhttp 的解析器，并设置其类型为 HTTP_REQUEST
  llhttp_init(&client->parser, HTTP_REQUEST, NULL);

  std::cout << "on_connection 6 " << std::endl;

  // 接受客户端连接，并将其与 TCP 句柄关联
  if (uv_accept(server, (uv_stream_t *)&client->tcp) == 0) {
    std::cout << "on_connection 7 " << std::endl;
    // 开始读取客户端发送的数据，并设置读取回调函数为 on_read
    uv_read_start((uv_stream_t *)&client->tcp, alloc_buffer, on_read);
  } else {
    std::cout << "on_connection 8 " << std::endl;
    // 如果接受失败，则关闭连接并释放资源
    uv_close((uv_handle_t *)&client->tcp, NULL);
    free(client);
  }
  std::cout << "on_connection 9 " << std::endl;
}

// 主函数
int main() {

  // 创建 libuv 的事件循环，并获取其指针
  loop = uv_default_loop();

  // 创建一个 libuv 的 TCP 句柄，并初始化其内容
  uv_tcp_t server;
  uv_tcp_init(loop, &server);

  // 创建一个 IPv4 地址结构体，并设置其 IP 地址和端口号为本地的 8000 端口
  struct sockaddr_in addr;
  uv_ip4_addr("192.168.1.116", 8080, &addr);

  // 将 TCP 句柄与该地址绑定，并开始监听该地址上的连接请求，设置连接回调函数为
  // on_connection
  uv_tcp_bind(&server, (const struct sockaddr *)&addr, 0);
  int r = uv_listen((uv_stream_t *)&server, SOMAXCONN, on_connection);

  // 检查是否有错误发生，如果有，则打印错误信息并退出程序
  if (r) {
    fprintf(stderr, "Listen error: %s\n", uv_strerror(r));
    return -1;
  }

  // 运行事件循环，直到没有活动句柄或请求
  return uv_run(loop, UV_RUN_DEFAULT);
}
