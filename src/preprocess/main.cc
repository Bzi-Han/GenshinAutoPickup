#include <opencv2/opencv.hpp>

#include <iostream>

// search F key range
//   1090, 430, 60, 300
// search type range
//   +65, -12, 50, 55

void PreProcess(const char *originImage, const cv::Rect &rect, const char *outputImage = nullptr)
{
    auto image = cv::imread(originImage, cv::IMREAD_GRAYSCALE);

    cv::Mat threshold;
    cv::threshold(image(rect), threshold, 255, 255, cv::THRESH_BINARY_INV | cv::THRESH_OTSU);

    cv::rectangle(image, rect, {255, 255, 0}, 2);
    cv::imshow("PreProcess", image);
    cv::waitKey();

    if (nullptr != outputImage)
        cv::imwrite(outputImage, threshold);

    cv::imshow("PreProcess", threshold);
    cv::waitKey();
}

void Detection(const char *originImage, const char *templateFKey, const char *templateType = nullptr, const cv::Rect &rectToSearchFKey = {}, double similarity = 0.9)
{
    auto image = cv::imread(originImage);
    auto fkey = cv::imread(templateFKey, -1);

    // Convert to grayscale
    cv::Mat grayImage;
    cv::cvtColor(image, grayImage, cv::COLOR_BGR2GRAY);

    // Get search range
    cv::Mat searchFKeyRange;
    if (rectToSearchFKey.empty())
        cv::threshold(grayImage, searchFKeyRange, 255, 255, cv::THRESH_BINARY_INV | cv::THRESH_OTSU);
    else
        cv::threshold(grayImage(rectToSearchFKey), searchFKeyRange, 255, 255, cv::THRESH_BINARY_INV | cv::THRESH_OTSU);

    // Match template
    cv::Mat searchFKeyResult;
    cv::matchTemplate(searchFKeyRange, fkey, searchFKeyResult, cv::TM_CCOEFF_NORMED);

    // Take result
    double minValue, maxValue;
    cv::Point minLocation, maxLocation;
    cv::minMaxLoc(searchFKeyResult, &minValue, &maxValue, &minLocation, &maxLocation);
    if (similarity > maxValue)
    {
        cv::imshow("Detection", image);
        cv::waitKey();
        return;
    }

    // Draw rectangle
    cv::Rect fkeyRect = rectToSearchFKey.empty() ? cv::Rect{maxLocation.x, maxLocation.y, fkey.cols, fkey.rows} : cv::Rect{maxLocation.x + rectToSearchFKey.x, maxLocation.y + rectToSearchFKey.y, fkey.cols, fkey.rows};
    cv::rectangle(image, fkeyRect, {0, 0, 255}, 2);

    // Check if need match type
    if (nullptr == templateType)
    {
        cv::imshow("Detection", image);
        cv::waitKey();
        return;
    }
    auto type = cv::imread(templateType, -1);

    // Get search range
    cv::Mat searchTypeRange;
    cv::threshold(grayImage({fkeyRect.x + 65, fkeyRect.y - 12, 50, 55}), searchTypeRange, 255, 255, cv::THRESH_BINARY_INV | cv::THRESH_OTSU);

    // Match template
    cv::Mat searchTypeResult;
    cv::matchTemplate(searchTypeRange, type, searchTypeResult, cv::TM_CCOEFF_NORMED);

    // Take result
    minValue, maxValue;
    minLocation, maxLocation;
    cv::minMaxLoc(searchTypeResult, &minValue, &maxValue, &minLocation, &maxLocation);
    if (similarity > maxValue)
    {
        cv::imshow("Detection", image);
        cv::waitKey();
        return;
    }

    // Draw rectangle
    cv::rectangle(image, {maxLocation.x + fkeyRect.x + 65, maxLocation.y + fkeyRect.y - 12, type.cols, type.rows}, {255, 255, 0}, 2);

    cv::imshow("Detection", image);
    cv::waitKey();
}

void AutoPreProcess(const char *originImage, const char *outputImage = nullptr)
{
    auto image = cv::imread(originImage);
    auto fkey = cv::imread("resources/FKey.png", -1);

    // Convert to grayscale
    cv::Mat grayImage;
    cv::cvtColor(image, grayImage, cv::COLOR_BGR2GRAY);

    // Get search range
    cv::Mat searchFKeyRange;
    cv::threshold(grayImage({1090, 430, 60, 300}), searchFKeyRange, 255, 255, cv::THRESH_BINARY_INV | cv::THRESH_OTSU);

    // Match template
    cv::Mat searchFKeyResult;
    cv::matchTemplate(searchFKeyRange, fkey, searchFKeyResult, cv::TM_CCOEFF_NORMED);

    // Take result
    double minValue, maxValue;
    cv::Point minLocation, maxLocation;
    cv::minMaxLoc(searchFKeyResult, &minValue, &maxValue, &minLocation, &maxLocation);
    if (0.9 > maxValue)
    {
        cv::imshow("AutoPreProcess", image);
        cv::waitKey();
        return;
    }

    // Draw rectangle
    cv::Rect fkeyRect = cv::Rect{maxLocation.x + 1090, maxLocation.y + 430, fkey.cols, fkey.rows};
    cv::rectangle(image, fkeyRect, {0, 0, 255}, 2);

    cv::Rect typeRect = cv::Rect{fkeyRect.x + 71, fkeyRect.y - 5, 38, 40};
    cv::rectangle(image, typeRect, {255, 255, 0}, 2);

    // Get type template
    cv::Mat templateImage;
    cv::threshold(grayImage(typeRect), templateImage, 255, 255, cv::THRESH_BINARY_INV | cv::THRESH_OTSU);

    if (nullptr != outputImage)
        cv::imwrite(outputImage, templateImage);

    cv::imshow("AutoPreProcess", image);
    cv::waitKey();
    cv::imshow("AutoPreProcess", templateImage);
    cv::waitKey();
}

void Cut(const char *originImage, const cv::Rect &rect, const char *outputImage = nullptr)
{
    auto image = cv::imread(originImage, -1);
    auto result = image(rect);

    if (nullptr != outputImage)
        cv::imwrite(outputImage, result);

    cv::imshow("PreProcess", result);
    cv::waitKey();
}

int main()
{
    // PreProcess("sample/00.jpg", {1170, 450, 40, 35});
    // Detection("sample/00.jpg", "resources/FKey.png", "resources/Interactable.png", {1090, 430, 60, 300});
    // AutoPreProcess("sample/00.jpg", "resources/Test.png");
    // Cut("sample/00.jpg", {2, 1, 35, 33}, "sample/00.jpg");

    return 0;
}