#include "DXGIRecorder.h"

#include <opencv2/opencv.hpp>

#include <Windows.h>

#include <iostream>
#include <memory>
#include <vector>
#include <string>
#include <filesystem>

std::shared_ptr<DXGIRecorder> g_recorder;

double g_similarity = 0.8;
HWND g_genshinHWND = nullptr;
size_t g_desktopImageSize = 0;
cv::Mat g_desktopImage, g_desktopGrayImage;
cv::Mat g_searchRangeCache, g_matchResult;
cv::Mat g_fkeyTemplate = cv::imread("resources/FKey.png", -1);
std::vector<cv::Mat> g_filterTypes;
cv::Rect g_fkeyDetectionRange{1090, 430, 60, 300};
cv::Rect g_typeDetectionRange{+65, -12, 50, 55};
cv::Rect g_typeTemplateRange{+73, -2, 34, 35};

INPUT F[2]{
    {.type = INPUT_KEYBOARD, .ki = {.wVk = 0x46}},
    {.type = INPUT_KEYBOARD, .ki = {.wVk = 0x46, .dwFlags = KEYEVENTF_KEYUP}},
};

void Initialize()
{
    g_genshinHWND = ::FindWindowA("UnityWndClass", "\xD4\xAD\xC9\xF1");
    if (nullptr == g_genshinHWND)
        throw std::exception("Can't found genshin window");

    RECT genshinWindowRect{};
    if (!::GetWindowRect(g_genshinHWND, &genshinWindowRect))
        throw std::exception("Failed to get genshin window rectangle");

    std::filesystem::directory_iterator diri("resources");
    for (const auto &file : diri)
    {
        if ("FKey.png" == file.path().filename())
            continue;

        g_filterTypes.push_back(cv::imread(file.path().string(), -1));
        std::cout << "[=] Load resource " << file.path() << std::endl;
    }

    int screenWidth = ::GetSystemMetrics(SM_CXSCREEN), screenHeight = ::GetSystemMetrics(SM_CYSCREEN);
    g_desktopImageSize = screenWidth * screenHeight * 4;
    g_desktopImage = cv::Mat(screenHeight, screenWidth, CV_8UC4);

    auto genshinWindowWidth = genshinWindowRect.right - genshinWindowRect.left;
    auto genshinWindowHeight = genshinWindowRect.bottom - genshinWindowRect.top;
    if (::IsIconic(g_genshinHWND))
    {
        genshinWindowWidth = screenWidth;
        genshinWindowHeight = screenHeight;
        std::cout << "[=] 当前判定原神窗口为全屏游戏。如果不是，请取消最小化原神窗口并重新打开此程序！" << std::endl;
    }
    if (1920 != genshinWindowWidth)
    {
        auto resamplingRate = genshinWindowWidth / 1920.;

        g_fkeyDetectionRange.x = static_cast<int>(g_fkeyDetectionRange.x * resamplingRate);
        g_fkeyDetectionRange.y = static_cast<int>(g_fkeyDetectionRange.y * resamplingRate);
        g_fkeyDetectionRange.width = static_cast<int>(g_fkeyDetectionRange.width * resamplingRate);
        g_fkeyDetectionRange.height = static_cast<int>(g_fkeyDetectionRange.height * resamplingRate);
        if (screenWidth != genshinWindowWidth)
            g_fkeyDetectionRange.x += genshinWindowRect.left;
        if (screenHeight != genshinWindowHeight)
            g_fkeyDetectionRange.y += genshinWindowRect.top;

        g_typeDetectionRange.x = static_cast<int>(g_typeDetectionRange.x * resamplingRate);
        g_typeDetectionRange.y = static_cast<int>(g_typeDetectionRange.y * resamplingRate);
        g_typeDetectionRange.width = static_cast<int>(g_typeDetectionRange.width * resamplingRate);
        g_typeDetectionRange.height = static_cast<int>(g_typeDetectionRange.height * resamplingRate);

        g_typeTemplateRange.x = static_cast<int>(g_typeTemplateRange.x * resamplingRate);
        g_typeTemplateRange.y = static_cast<int>(g_typeTemplateRange.y * resamplingRate);
        g_typeTemplateRange.width = static_cast<int>(g_typeTemplateRange.width * resamplingRate);
        g_typeTemplateRange.height = static_cast<int>(g_typeTemplateRange.height * resamplingRate);

        cv::resize(g_fkeyTemplate, g_fkeyTemplate, {}, resamplingRate, resamplingRate);

        for (auto &filter : g_filterTypes)
            cv::resize(filter, filter, {}, resamplingRate, resamplingRate);
    }
}

void OnCustomDataAdd(int x, int y)
{
    cv::Mat templateImage;
    cv::threshold(
        g_desktopGrayImage({
            g_fkeyDetectionRange.x + x + g_typeTemplateRange.x,
            g_fkeyDetectionRange.y + y + g_typeTemplateRange.y,
            g_typeTemplateRange.width,
            g_typeTemplateRange.height,
        }),
        templateImage,
        255, 255, cv::THRESH_BINARY_INV | cv::THRESH_OTSU);

    std::string fileName = "resources/" + std::to_string(std::chrono::system_clock::now().time_since_epoch().count()) + ".png";
    cv::imwrite(fileName, templateImage);
    std::cout << "[=] Save custom template " << fileName << std::endl;

    g_filterTypes.push_back(templateImage);
}

void OnDetected(int x, int y)
{
    ::SendInput(2, F, sizeof(INPUT));
}

int main()
{
    std::system("chcp 65001");

    try
    {
        Initialize();
        g_recorder = std::make_shared<DXGIRecorder>();

        for (;;)
        {
            if (::IsIconic(g_genshinHWND))
            {
                std::this_thread::sleep_for(std::chrono::milliseconds(1));
                continue;
            }

            if (0 == g_recorder->GetNextFrameData(g_desktopImage.data, g_desktopImageSize))
            {
                std::this_thread::sleep_for(std::chrono::milliseconds(1));
                continue;
            }

            double minValue, maxValue;
            cv::Point minLocation, maxLocation;

            cv::cvtColor(g_desktopImage, g_desktopGrayImage, cv::COLOR_BGR2GRAY);

            // Match F key
            cv::threshold(g_desktopGrayImage(g_fkeyDetectionRange), g_searchRangeCache, 255, 255, cv::THRESH_BINARY_INV | cv::THRESH_OTSU);
            cv::matchTemplate(g_searchRangeCache, g_fkeyTemplate, g_matchResult, cv::TM_CCOEFF_NORMED);
            cv::minMaxLoc(g_matchResult, &minValue, &maxValue, &minLocation, &maxLocation);
            if (g_similarity > maxValue)
            {
                std::this_thread::sleep_for(std::chrono::milliseconds(1));
                continue;
            }
            auto x = maxLocation.x, y = maxLocation.y;

            // Match type
            bool isFilterItem = false;
            cv::threshold(
                g_desktopGrayImage({
                    g_fkeyDetectionRange.x + maxLocation.x + g_typeDetectionRange.x,
                    g_fkeyDetectionRange.y + maxLocation.y + g_typeDetectionRange.y,
                    g_typeDetectionRange.width,
                    g_typeDetectionRange.height,
                }),
                g_searchRangeCache,
                255,
                255,
                cv::THRESH_BINARY_INV | cv::THRESH_OTSU);

            for (const auto &filter : g_filterTypes)
            {
                cv::matchTemplate(g_searchRangeCache, filter, g_matchResult, cv::TM_CCOEFF_NORMED);
                cv::minMaxLoc(g_matchResult, &minValue, &maxValue, &minLocation, &maxLocation);
                if (g_similarity > maxValue)
                    continue;

                isFilterItem = true;
                break;
            }

            if (isFilterItem)
            {
                std::this_thread::sleep_for(std::chrono::milliseconds(1));
                continue;
            }

            OnDetected(x, y);
        }
    }
    catch (const std::exception &e)
    {
        std::string message = std::string{"[-] "} + e.what();
        ::MessageBoxA(nullptr, message.data(), "Error:", MB_OK | MB_ICONERROR);
    }

    return 0;
}
