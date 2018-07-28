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
void createframegrabber(int newsockfd)
{
    FILE *file;

    realcounter = 0;
    onNewFramecounter = 0;
    framgrabber = nullptr;
    framgrabber = SL::Screen_Capture::CreateCaptureConfiguration([]() {
                      auto mons = SL::Screen_Capture::GetMonitors();
                      std::cout << "Library is requesting the list of monitors to capture!" << std::endl;
                      for (auto &m : mons) {
                          m.Height = 200;
                          m.Width = 100;
                          std::cout << m << std::endl;
                      }
                      return mons;
                  })
                      ->onNewFrame([&](const SL::Screen_Capture::Image &img, const SL::Screen_Capture::Monitor &monitor) {
                          cout << "onNewFrame!!!!";
                          auto r = realcounter.fetch_add(1);
                          auto s = std::string("capture.jpg");
                          auto size = Width(img) * Height(img) * sizeof(SL::Screen_Capture::ImageBGRA);
                          auto imgbuffer(std::make_unique<unsigned char[]>(size));
                          ExtractAndConvertToRGBA(img, imgbuffer.get(), size);
                          auto tempBuffer = (const char *)imgbuffer.get();

                          tje_encode_to_file(s.c_str(), Width(img), Height(img), 4, (const unsigned char *)imgbuffer.get());
                          //   //读取并发送
                          //   //发送文件大小
                          //   int fileSize = getSize("capture.jpg");
                          //   char file_size[256];
                          //   sprintf(file_size, "%d", fileSize);
                          //   send(newsockfd, file_size, strlen(file_size), 0);
                          //   //服务端确认收到
                          //   int sizeRecv_size;
                          //   char sizeBuf[1024];
                          //   sizeRecv_size = recv(newsockfd, sizeBuf, 1024, 0);
                          // //   char *sizeRecv = new char[sizeRecv_size + 1];
                          //   for (int i = 0; i < sizeRecv_size; i++)
                          //       sizeRecv[i] = sizeBuf[i];
                          //   sizeRecv[sizeRecv_size] = '\0';
                          //   if (strcmp(sizeRecv, "size_recv") != 0)
                          //       break;
                          file = fopen("capture.jpg", "rb");
                          char buf[1024];
                          while (!feof(file)) {
                              memset(buf, 0, sizeof(buf));
                              size_t readlen = fread(buf, sizeof(char), sizeof(buf), file);
                              send(newsockfd, buf, readlen, 0);
                          }
                          fclose(file);
                          char sizeBuf[1024];
                          recv(newsockfd, sizeBuf, 1024, 0);
                          // if (std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::high_resolution_clock::now() -
                          // onNewFramestart).count() >=
                          //     1000) {
                          //     std::cout << "onNewFrame fps" << onNewFramecounter << std::endl;
                          //     onNewFramecounter = 0;
                          //     onNewFramestart = std::chrono::high_resolution_clock::now();
                          // }
                          // onNewFramecounter += 1;
                      })
                      ->start_capturing();
    ;
    framgrabber->setFrameChangeInterval(std::chrono::milliseconds(1000));
}
void *TCPServer::Task(void *arg)
{

    cout << "connect success!!!" << endl;
    int n;
    int newsockfd = (long)arg;
    char msg[MAXPACKETSIZE];
    pthread_detach(pthread_self());

    createframegrabber(newsockfd);

    std::this_thread::sleep_for(std::chrono::seconds(100000));

    while (1) {
        n = recv(newsockfd, msg, MAXPACKETSIZE, 0);
        if (n == 0) {
            framgrabber = NULL;
            close(newsockfd);
            break;
        }
        // msg[n] = 0;
        // // send(newsockfd,msg,n,0);
        // Message = string(msg);
    }

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
        newsockfd = accept(sockfd, (struct sockaddr *)&clientAddress, &sosize);
        str = inet_ntoa(clientAddress.sin_addr);
        pthread_create(&serverThread, NULL, &Task, (void *)newsockfd);
    }
    return str;
}

string TCPServer::getMessage() { return Message; }

void TCPServer::Send(string msg) { send(newsockfd, msg.c_str(), msg.length(), 0); }

void TCPServer::clean()
{
    Message = "";
    memset(msg, 0, MAXPACKETSIZE);
}

void TCPServer::detach()
{
    close(sockfd);
    close(newsockfd);
}