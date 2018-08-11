#include "TCPServer.h"
#include <iostream>
#include <fstream>
TCPServer tcp;

int main()
{
    //从配置文件读取监听端口号
    ifstream in("config");
    string line;
    if(in){
    //port
    getline(in,line);
     int port = atoi(line.c_str());
    cout << "监听port" << port << endl;
    //启动截图
    tcp.createframegrabber();
    //绑定
    tcp.setup(port);
    //监听
    tcp.receive();
    }
    return 0;
}
