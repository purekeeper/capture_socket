#include "TCPServer.h"
#include "ScreenCapture.h"
#include "internal/SCCommon.h" //DONT USE THIS HEADER IN PRODUCTION CODE!!!! ITS INTERNAL FOR A REASON IT WILL CHANGE!!! ITS HERE FOR TESTS ONLY!!!
#include <algorithm>
#include <atomic>
#include <chrono>
#include <climits>
#include <iostream>
#include <locale>
#include <string.h>
#include <string>
#include <thread>
#include <vector>

// THESE LIBRARIES ARE HERE FOR CONVINIENCE!! They are SLOW and ONLY USED FOR
// HOW THE LIBRARY WORKS!
#define TJE_IMPLEMENTATION
#include "tiny_jpeg.h"
#define LODEPNG_COMPILE_PNG
#define LODEPNG_COMPILE_DISK
#include "lodepng.h"
/////////////////////////////////////////////////////////////////////////
string TCPServer::Message;

void ExtractAndConvertToRGBA(const SL::Screen_Capture::Image &img, unsigned char *dst, size_t dst_size)
{
    assert(dst_size >= static_cast<size_t>(SL::Screen_Capture::Width(img) * SL::Screen_Capture::Height(img) * sizeof(SL::Screen_Capture::ImageBGRA)));
    auto imgsrc = StartSrc(img);
    auto imgdist = dst;
    for (auto h = 0; h < Height(img); h++) {
        auto startimgsrc = imgsrc;
        for (auto w = 0; w < Width(img); w++) {
            *imgdist++ = imgsrc->R;
            *imgdist++ = imgsrc->G;
            *imgdist++ = imgsrc->B;
            *imgdist++ = 0; // alpha should be zero
            imgsrc++;
        }
        imgsrc = SL::Screen_Capture::GotoNextRow(img, startimgsrc);
    }
}

using namespace std::chrono_literals;
std::shared_ptr<SL::Screen_Capture::IScreenCaptureManager> framgrabber;
std::atomic<int> realcounter;
std::atomic<int> onNewFramecounter;

inline std::ostream &operator<<(std::ostream &os, const SL::Screen_Capture::ImageRect &p)
{
    return os << "left=" << p.left << " top=" << p.top << " right=" << p.right << " bottom=" << p.bottom;
}
inline std::ostream &operator<<(std::ostream &os, const SL::Screen_Capture::Monitor &p)
{
    return os << "Id=" << p.Id << " Index=" << p.Index << " Height=" << p.Height << " Width=" << p.Width << " OffsetX=" << p.OffsetX
              << " OffsetY=" << p.OffsetY << " Name=" << p.Name;
}

auto onNewFramestart = std::chrono::high_resolution_clock::now();
size_t getSize(string path)
{
    FILE *file;
    file = fopen(path.c_str(), "rb");
    size_t sizeOfPic;

    fseek(file, 0, SEEK_END); ///将文件指针移动文件结尾
    sizeOfPic = ftell(file);  ///求出当前文件指针距离文件开始的字节数
    fclose(file);

    return sizeOfPic;
}

void sendFile(int newsockfd){
FILE *file;
 cout << "fopen!!!" << endl;
 //file = fopen("capture.jpg", "rb");
 while(true){
   char buf[1024*1000];
                          file = fopen("capture.jpg", "rb");
                                memset(buf, 0, sizeof(buf));
                              size_t readlen = fread(buf, sizeof(char), sizeof(buf), file);
                              cout<<"length"<<readlen<<endl;
                              //读取并发送
                             //发送文件大小
                              cout<<"1111111"<<endl;

                             cout<<"2222222"<<endl;
                            char file_size[256];
                            sprintf(file_size, "%d", (int)readlen);
                            cout<<"file_size"<<file_size<<endl;
                            if(send(newsockfd, file_size, strlen(file_size), 0)==-1){
                            return;}
                             cout<<"777777"<<endl;
                             //服务端确认收
                             int sizeRecv_size;
                             char sizeBuf[1024];
                            if( (sizeRecv_size = recv(newsockfd, sizeBuf, 1024, 0))==-1){
                            return;}
                             cout<<"ack="<<sizeRecv_size<<endl;

                          cout << "begin send file!!!" << endl;
                          int i=0;
                          cout << "seding!!!"<<i++ << endl;

                              if(send(newsockfd, buf, readlen, 0)==-1){return;}
                              cout<<"22222222"<<readlen<<endl;
                          cout << "over send file!!!" << endl;
                        //  fclose(file);
                          char fileBuff[1024];
                          cout << "receive ack file!!!" << endl;

                         if( recv(newsockfd, fileBuff, 1024, 0)==-1){
                         return;}
                          cout<<fileBuff[0]<<endl;
                          cout << "received ack file!!!" << endl;
                            fclose(file);
}
                          }


void TCPServer::createframegrabber()
{
    pthread_detach(pthread_self());

//cout<<"sock_id_111"<<newsockfd<<endl;
    realcounter = 0;
    onNewFramecounter = 0;
    framgrabber = nullptr;
    framgrabber = SL::Screen_Capture::CreateCaptureConfiguration([]() {
                      auto mons = SL::Screen_Capture::GetMonitors();
                      std::cout << "Library is requesting the list of monitors to capture!" << std::endl;
                      for (auto &m : mons) {
                          std::cout << m << std::endl;
                      }
                      return mons;
                  })
                      ->onNewFrame([&](const SL::Screen_Capture::Image &img, const SL::Screen_Capture::Monitor &monitor) {
                          cout << "onNewFrame!!!!";
                         // auto r = realcounter.fetch_add(1);
                          auto s = std::string("capture.jpg");
                          auto size = Width(img) * Height(img) * sizeof(SL::Screen_Capture::ImageBGRA);
                          auto imgbuffer(std::make_unique<unsigned char[]>(size));
                          ExtractAndConvertToRGBA(img, imgbuffer.get(), size);
                          cout << "write img!!!" << endl;
                          tje_encode_to_file(s.c_str(), Width(img), Height(img), 4, (const unsigned char *)imgbuffer.get());
                      })
                      ->start_capturing();
    ;
    framgrabber->setFrameChangeInterval(std::chrono::milliseconds(1000));
}
void *TCPServer::Task(void *arg)
{

    cout << "connect success!!!" << endl;
    int n;
    int newsockfd = *((int*)arg);

    char msg[MAXPACKETSIZE];
    pthread_detach(pthread_self());
  //  createframegrabber();
  sendFile(newsockfd);
  //  std::this_thread::sleep_for(std::chrono::seconds(100000));
    return 0;
}

void TCPServer::setup(int port)
{
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    memset(&serverAddress, 0, sizeof(serverAddress));
    serverAddress.sin_family = AF_INET;
    serverAddress.sin_addr.s_addr = htonl(INADDR_ANY);
    serverAddress.sin_port = htons(port);
    bind(sockfd, (struct sockaddr *)&serverAddress, sizeof(serverAddress));
    listen(sockfd, 5);
}

string TCPServer::receive()
{
    string str;
    while (1) {
        socklen_t sosize = sizeof(clientAddress);
        int newsockfd = accept(sockfd, (struct sockaddr *)&clientAddress, &sosize);
        cout<<"sock"<<newsockfd<<endl;
       // sockfdList.push_back(newsockfd);
        str = inet_ntoa(clientAddress.sin_addr);
        pthread_create(&serverThread, NULL, &Task, (void*) &newsockfd);
    }
    return str;
}

string TCPServer::getMessage() { return Message; }

void TCPServer::Send(string msg) {
 //send(newsockfd, msg.c_str(), msg.length(), 0);
 }

void TCPServer::clean()
{
    Message = "";
    memset(msg, 0, MAXPACKETSIZE);
}

void TCPServer::detach()
{
    close(sockfd);
   // close(newsockfd);
}
