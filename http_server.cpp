#include<iostream>
#include<string>
#include<stdio.h>
#include<errno.h>
#include<unistd.h>
#include<sys/socket.h>
#include<arpa/inet.h>
#include<stdlib.h>
#include<pthread.h>
#include<sys/stat.h>
#include<sys/types.h>
#include<fcntl.h>
#include <cstring>


using namespace std;
#define PORT 80
#define IP "192.168.125.128"
static int debug = 1;

int get_line(int sock,char *buf,int size);
void* do_http_request(void* pclient_sock);
void do_http_response(int client_sock,const char *path);
int headers(int client_sock,FILE *resource);
void cat(int client_sock,FILE *resource);

void not_found(int client_sock);//404

void unimplemented(int client_sock);//500

// void bad_request(int client_sock);//400

void inner_error(int client_sock);
int main(void)
{ 
    int server_sock;
    struct sockaddr_in server_addr;

    server_sock = socket(AF_INET,SOCK_STREAM,0);
    if(server_sock < 0)
    {
        perror("socket error");
        return -1;
    }
    // bzero(&server_addr,sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    // server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    server_addr.sin_addr.s_addr = inet_addr(IP);
    server_addr.sin_port = htons(PORT);
    //ip复用
    int reuse = 1;
    setsockopt(server_sock,SOL_SOCKET,SO_REUSEADDR,&reuse,sizeof(reuse));


    if(bind(server_sock,(struct sockaddr *)&server_addr,sizeof(server_addr)) < 0)
    {
        perror("bind error");
        return -1;
    }

    //监听
    if(listen(server_sock,128) < 0)
    {
        perror("listen error");
        return -1;
    }

    cout<<"http server start..."<<endl;

    while(1)
    { 
        struct sockaddr_in client_addr;
        int client_sock;
        socklen_t client_addr_len = sizeof(client_addr);
        client_sock = accept(server_sock,(struct sockaddr *)&client_addr,&client_addr_len);
        if(client_sock < 0)
        {
            perror("accept error");
            continue;
        }

        cout<<"client connected..."<<endl;
        cout<<"client ip:"<<inet_ntoa(client_addr.sin_addr)<<endl;
        cout<<"client port:"<<ntohs(client_addr.sin_port)<<endl;

        // do_http_request(client_sock);
         

        // //pthread
        pthread_t tid;
        int* pclient_sock = new int(client_sock);
        pthread_create(&tid,NULL,do_http_request,pclient_sock);
        // pthread_detach(tid);//线程分离
        cout<<"####thread id:"<<tid<<endl;  
        
        

        // close(client_sock);
 
    }
   
    close(server_sock);

    



    return 0;
}

//返回值： -1 表示读取出错， 等于0表示读到一个空行， 大于0 表示成功读取一行
int get_line(int sock,char *buf,int size)
{
    int i = 0;
    char c = '\0';
    int n;

    while((i < size - 1) && (c != '\n'))
    {
        n = read(sock,&c,1);
        if(n == 1)
        {
            if(c == '\r')//回车就跳过
            {
                continue;
            }else if(c == '\n'  ){//换行就结束
                buf[i] = '\0';
                break;
            
            }
            //处理正常的字符
            buf[i] = c;//buf里会储存读取的一行
            i++;


        }else if(n == -1){
            perror("read error");
            i = -1;
            break;
        }else if(n == 0){
            cout<<"client close"<<endl; 
            i = -1;
            break;
        }
        if(i>=0){
            buf[i] = '\0';
        }
        
    }
    return i;
}
void* do_http_request(void* pclient_sock)
{
    int len = 0;
    char buf[1024];
    char method[64];
    char path[256];
    char url[256];
    int client_sock = *(int*)pclient_sock;

   
    len = get_line(client_sock,buf,sizeof(buf));
    
    // len = get_line(client_sock,buf,sizeof(buf));
    
    if(len>0){
       int i = 0,j = 0;
        while(!isspace(buf[j]) && (i < sizeof(method)-1)){
            method[i] = buf[j];
            i++;
            j++;
        }
        method[i] = '\0';
        if(debug)cout<<"method:"<<method<<endl;

    
    
    if(strcasecmp(method,"GET") ==0){
        // if(debug)cout<<"method=GET"<<endl;
        while(isspace(buf[j++]));

        i=0;
      
        while (!isspace(buf[j]) && i < sizeof(url)-1) {
            url[i++] = buf[j++];
            }


        url[i] = '\0';
        if(debug)cout<<"url:"<<url<<endl;

        do{
            len = get_line(client_sock,buf,sizeof(buf));
            if(debug)cout<<"header:"<<buf<<endl;

          }while(len>0 );

          //处理url中的？
          {
            char* pos = strchr(url,'?');
            if(pos) *pos = '\0';
            if(debug)cout<<"real url : "<<url<<endl;
          }

          // 限制最多复制 240 字节的 url
            // 在构造 path 前，确保 url 以 / 开头
        if (url[0] != '/') {
            // 临时修复：加前导 /
            char safe_url[256];
            snprintf(safe_url, sizeof(safe_url), "/%.*s", 240, url);
            snprintf(path, sizeof(path), "./html%.*s", 240, safe_url);
            } else {
             snprintf(path, sizeof(path), "./html%.*s", 240, url);
        }
            if(debug)cout<<"path:"<<path<<endl;
          //定位文件
          
          

          // 处理响应
        do_http_response(client_sock,path);
        close(client_sock);
      
        delete (int*)pclient_sock;


    }else{
        //非GET请求，501 错误
        
        do{
            len = get_line(client_sock,buf,sizeof(buf));
            if(debug)cout<<"header:"<<buf<<endl;
        }while(len>0);
        
        //501
        if(debug)cout<<"method not support"<<endl;
        unimplemented(client_sock);
        // inner_error(client_sock);
        
    }

    

    }


    

return 0;
}


void do_http_response(int client_sock, const char *path)
{
    FILE *resource = fopen(path, "rb"); // 修复1: 使用二进制模式
    if (resource == NULL) {
        not_found(client_sock);//404
        return;
    }

    int ret = headers(client_sock, resource);
    if (ret == 0) {
        cat(client_sock, resource);
    }
    
    fclose(resource); // 安全关闭（resource != NULL）
}

void not_found(int client_sock) {
    const char* response = 
        "HTTP/1.1 404 Not Found\r\n"
        "Content-Type: text/html\r\n"
        "Connection: close\r\n"
        "\r\n"
        "<html>\n"
        "<head><title>404 Not Found</title></head>\n"
        "<body>\n"
        "<center><h1>404 Not Found</h1></center>\n"
        "<hr>\n"
        "<center>Mini HTTP Server</center>\n"
        "</body>\n"
        "</html>\n";
    
    // 直接发送预定义的响应（避免拼接开销）
    write(client_sock, response, strlen(response));
}
int headers(int client_sock,FILE *response){//返回文件头
    char buf[1024];
    
    
    struct stat st;//获取文件信息



    strcpy(buf, "HTTP/1.0 200 OK\r\n");
	strcat(buf, "Server: Mini HTTP Server\r\n");
	strcat(buf, "Content-Type: text/html\r\n");
	strcat(buf, "Connection: Close\r\n");

    int fileid = fileno(response);//获取文件描述符
    if(fstat(fileid,&st) == -1){//获取文件信息错误
        inner_error(client_sock);//500 错误
        return -1;
    }

    sprintf(buf+strlen(buf),"Content-Length: %ld\r\n",st.st_size);//获取文件大小
    strcat(buf,"\r\n");

    

    if(debug)cout<<" response headers:"<<buf<<endl;

    if(send(client_sock,buf,strlen(buf),0) < 0){
        perror("send error");
        return -1;
    }

    return 0;


}
// 发送文件内容
void cat(int client_sock, FILE *resource) {
    char buf[1024];
    size_t n;
    while ((n = fread(buf, 1, sizeof(buf), resource)) > 0) {//
        if (write(client_sock, buf, n) != (ssize_t)n) {
            if (debug) perror("write error");
            break;
        }
        if (debug) {
            buf[n] = '\0';
            cout << "write: " << buf << endl;
        }
    }
}


// 500 内部错误
void inner_error(int client_sock) {
    const char* response = 
        "HTTP/1.1 500 Internal Server Error\r\n"
        "Content-Type: text/html\r\n"
        "Connection: close\r\n"
        "\r\n"
        "<html>\n"
        "<head><title>500 Internal Server Error</title></head>\n"
        "<body>\n"
        "<center><h1>500 Internal Server Error</h1></center>\n"
        "<hr>\n"
        "<center>Mini HTTP Server</center>\n"
        "</body>\n"
        "</html>\n";
    
    write(client_sock, response, strlen(response));
}

void unimplemented(int client_sock)
{
    char buf[1024];
    sprintf(buf,"HTTP/1.1 501 Method Not Implemented\r\n");
    sprintf(buf+strlen(buf),"Content-Type: text/html\r\n");
    sprintf(buf+strlen(buf),"\r\n");
    sprintf(buf+strlen(buf),"<html><title>501 Method Not Implemented</title>");
    sprintf(buf+strlen(buf),"<body><center><h1>501 Method Not Implemented</h1></center></body>");
    write(client_sock,buf,strlen(buf));
    return;
}