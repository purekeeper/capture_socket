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
                              int  port=atoi(buf);
                              cout<<"port"<<port<<endl;
    tcp.createframegrabber();
    tcp.setup(port);
    tcp.receive();
    // if( pthread_create(&msg, NULL, loop, (void *)0) == 0)
    // {
    // 	tcp.receive();
    // }
    return 0;
}
