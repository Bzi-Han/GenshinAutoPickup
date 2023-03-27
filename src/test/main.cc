#include "DXGIRecorder.h"

#include <opencv2/opencv.hpp>

#include <Windows.h>

#include <iostream>
#include <memory>

std::shared_ptr<DXGIRecorder> g_recorder;

size_t g_desktopImageSize = 0;
cv::Mat g_desktopImage, g_desktopShowImage;

int main()
{
    try
    {
        g_recorder = std::make_shared<DXGIRecorder>();
        int screenWidth = ::GetSystemMetrics(SM_CXSCREEN), screenHeight = ::GetSystemMetrics(SM_CYSCREEN);
        g_desktopImageSize = screenWidth * screenHeight * 4;
        g_desktopImage = cv::Mat(screenHeight, screenWidth, CV_8UC4);

        for (;;)
        {
            if (0 == g_recorder->GetNextFrameData(g_desktopImage.data, g_desktopImageSize))
            {
                std::this_thread::sleep_for(std::chrono::milliseconds(1));
                continue;
            }

            cv::resize(g_desktopImage, g_desktopShowImage, {}, 0.5, 0.5);
            cv::imshow("Recorder", g_desktopShowImage);
            cv::waitKey(1);
        }
    }
    catch (const std::exception &e)
    {
        std::string message = std::string{"[-] "} + e.what();
        ::MessageBoxA(nullptr, message.data(), "Error:", MB_OK | MB_ICONERROR);
    }

    return 0;
}