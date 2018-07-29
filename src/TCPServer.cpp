#include "TCPServer.h"
#include "ScreenCapture.h"
#include "internal/SCCommon.h"
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
            *imgdist++ = 0;
            imgsrc++;
        }
        imgsrc = SL::Screen_Capture::GotoNextRow(img, startimgsrc);
    }
}

using namespace std::chrono_literals;
std::shared_ptr<SL::Screen_Capture::IScreenCaptureManager> framgrabber;
auto onNewFramestart = std::chrono::high_resolution_clock::now();
void sendFile(int newsockfd)
{
    FILE *file;
    while (true) {
        char buf[1024 * 1000];
        file = fopen("capture.jpg", "rb");
        memset(buf, 0, sizeof(buf));
        size_t readlen = fread(buf, sizeof(char), sizeof(buf), file);
        //发送文件大小
        char file_size[256];
        sprintf(file_size, "%d", (int)readlen);
        if (send(newsockfd, file_size, strlen(file_size), 0) == -1) {
            return;
        }
        //服务端确认收
        int sizeRecv_size;
        char sizeBuf[1024];
        if ((sizeRecv_size = recv(newsockfd, sizeBuf, 1024, 0)) == -1) {
            return;
        }
        if (send(newsockfd, buf, readlen, 0) == -1) {
            return;
        }
        //  fclose(file);
        char fileBuff[1024];

        if (recv(newsockfd, fileBuff, 1024, 0) == -1) {
            return;
        }
        fclose(file);
    }
}

void TCPServer::createframegrabber()
{
    pthread_detach(pthread_self());
    framgrabber = nullptr;
    framgrabber = SL::Screen_Capture::CreateCaptureConfiguration([]() { return SL::Screen_Capture::GetMonitors(); })
                      ->onNewFrame([&](const SL::Screen_Capture::Image &img, const SL::Screen_Capture::Monitor &monitor) {
                          auto s = std::string("capture.jpg");
                          auto size = Width(img) * Height(img) * sizeof(SL::Screen_Capture::ImageBGRA);
                          auto imgbuffer(std::make_unique<unsigned char[]>(size));
                          ExtractAndConvertToRGBA(img, imgbuffer.get(), size);
                          tje_encode_to_file(s.c_str(), Width(img), Height(img), 4, (const unsigned char *)imgbuffer.get());
                      })
                      ->start_capturing();
    ;
    framgrabber->setFrameChangeInterval(std::chrono::milliseconds(500));
}
void *TCPServer::Task(void *arg)
{
    cout << "connect success!!!" << endl;
    int newsockfd = *((int *)arg);
    char msg[MAXPACKETSIZE];
    pthread_detach(pthread_self());
    sendFile(newsockfd);
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
        str = inet_ntoa(clientAddress.sin_addr);
        pthread_create(&serverThread, NULL, &Task, (void *)&newsockfd);
    }
    return str;
}

string TCPServer::getMessage() { return Message; }

void TCPServer::Send(string msg)
{
    // send(newsockfd, msg.c_str(), msg.length(), 0);
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
