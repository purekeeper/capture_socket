#include "TCPServer.h"
#include <iostream>

TCPServer tcp;

int main()
{
    //从配置文件读取监听端口号
    FILE *file;
    char buf[100];
    file = fopen("config", "rb");
    memset(buf, 0, sizeof(buf));
    fread(buf, sizeof(char), sizeof(buf), file);
    int port = atoi(buf);
    cout << "监听port" << port << endl;
    //启动截图
    tcp.createframegrabber();
    //绑定
    tcp.setup(port);
    //监听
    tcp.receive();
    return 0;
}
