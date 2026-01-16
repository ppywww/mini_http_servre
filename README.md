# Mini HTTP Server

一个轻量级、基于 C++ 的简易 HTTP 服务器，支持静态文件服务和基本的 HTTP/1.0 协议。

## 🌟 功能特性

- 支持 `GET` 请求
- 静态文件服务（HTML、CSS、JS、图片等）
- 自动识别文件类型（当前默认为 `text/html`，可扩展）
- 错误处理：
  - `404 Not Found`
  - `501 Method Not Implemented`（非 GET 请求）
  - `500 Internal Server Error`
- 多线程处理并发请求
- 支持 URL 查询参数（自动忽略 `?` 后内容）

## 🛠️ 编译与运行

### 依赖
- Linux 环境
- g++ 编译器
- pthread 库（通常已内置）

### 编译
```bash
g++ -o server http_server.cpp -lpthread
```

### 运行
```bash
./server
```

> 默认监听 `192.168.125.128:80`，请确保：
> - 该 IP 是本机有效 IP
> - 有权限绑定 80 端口（可能需要 `sudo`）
> - 防火墙允许 80 端口访问

### 目录结构
项目根目录下需创建 `html/` 文件夹存放静态资源：
```
mini_http/
├── http_server.cpp
├── server              # 编译生成的可执行文件
└── html/               # 静态文件目录
    ├── index.html
    └── ...
```

## 🌐 使用示例

1. 启动服务器：
   ```bash
   ./server
   ```

2. 浏览器访问：
   ```
   http://192.168.125.128/
   ```

3. 访问子页面：
   ```
   http://192.168.125.128/about.html
   ```

## ⚙️ 配置说明

在 [http_server.cpp](file:///home/ppy001/projects/mini_http/http_server.cpp) 中可修改以下宏定义：

```cpp
#define PORT 80                    // 监听端口
#define IP "192.168.125.128"      // 绑定 IP（建议改为 INADDR_ANY 以监听所有接口）
static int debug = 1;             // 1=开启调试日志，0=关闭
```

> **建议**：将 `IP` 改为 `INADDR_ANY` 以支持任意 IP 访问：
> ```cpp
> // server_addr.sin_addr.s_addr = inet_addr(IP);
> server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
> ```

## 📂 项目结构

```
.
├── README.md
├── http_server.cpp          # 核心源码
└── html/                    # 静态资源目录（需手动创建）
```

## 🚧 注意事项

- 当前仅支持 `GET` 方法，其他方法返回 `501`
- 文件服务路径为 `./html{URL}`，例如 `/test.html` 对应 `./html/test.html`
- 默认以二进制模式读取文件，适合传输任意类型静态资源
- 线程未分离（`pthread_detach` 被注释），长期运行可能积累僵尸线程

## 📜 协议支持

- HTTP/1.0（响应头中声明）
- 支持的请求头：自动跳过所有请求头（仅解析第一行）
- 响应包含：
  - `Content-Length`
  - `Content-Type: text/html`（可扩展）
  - `Connection: Close`

## 🤝 贡献

欢迎提交 Issue 或 Pull Request！建议改进方向：
- 支持更多 MIME 类型
- 实现 `HEAD` 方法
- 添加日志文件输出
- 支持配置文件

---

> **Mini HTTP Server** —— 简单、清晰、易懂的网络编程学习项目 💻
