#include <iostream>
#include <filesystem>
#include <EGrabber.h>
#include <opencv2/opencv.hpp>
#include <FormatConverter.h>
#include <vector>
#include <atomic>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <queue>
#include <conio.h>
#include <chrono>
#include <iomanip>
#include <CircularBuffer/CircularBuffer.h>
#include <tuple>
#include <opencv2/highgui.hpp>
#include <opencv2/imgproc.hpp>
#include <image_processing/image_processing.h>
#include <menu_system/menu_system.h>
#include <nlohmann/json.hpp>
// #include <mib_grabber/mib_grabber.h>
// #include <../egrabber_config.h>

#define M_PI 3.14159265358979323846 // pi

cv::Mat backgroundFrame;

using namespace Euresys;

struct GrabberParams
{
    size_t width;
    size_t height;
    uint64_t pixelFormat;
    size_t imageSize;
    size_t bufferCount;
};

void configure(EGrabber<CallbackOnDemand> &grabber)
{
    grabber.setInteger<RemoteModule>("Width", 512);
    grabber.setInteger<RemoteModule>("Height", 96);
    grabber.setInteger<RemoteModule>("AcquisitionFrameRate", 4700);
    // ... (other configuration settings)
}

void configure_js(std::string config_path)
{
    Euresys::EGenTL gentl;
    Euresys::EGrabber<> grabber(gentl);
    if (config_path.substr(config_path.size() - 3) != ".js")
    {
        throw std::runtime_error("Config path must end with .js");
    }
    grabber.runScript(config_path);
    std::cout << "Config script executed successfully" << std::endl;
}

ImageParams initializeGrabber(EGrabber<CallbackOnDemand> &grabber)
{
    grabber.reallocBuffers(3);
    grabber.start(1);
    ScopedBuffer firstBuffer(grabber);

    ImageParams params;
    params.width = firstBuffer.getInfo<size_t>(gc::BUFFER_INFO_WIDTH);
    params.height = firstBuffer.getInfo<size_t>(gc::BUFFER_INFO_HEIGHT);
    params.pixelFormat = firstBuffer.getInfo<uint64_t>(gc::BUFFER_INFO_PIXELFORMAT);
    params.imageSize = firstBuffer.getInfo<size_t>(gc::BUFFER_INFO_SIZE);
    params.bufferCount = 5000; // You can adjust this as needed

    grabber.stop();
    return params;
}

void initializeBackgroundFrame(SharedResources &shared, const ImageParams &params)
{
    std::lock_guard<std::mutex> lock(shared.backgroundFrameMutex);
    shared.backgroundFrame = cv::Mat(static_cast<int>(params.height), static_cast<int>(params.width), CV_8UC1, cv::Scalar(255));
    cv::GaussianBlur(shared.backgroundFrame, shared.blurredBackground, cv::Size(3, 3), 0);
}

void temp_sample(EGrabber<CallbackOnDemand> &grabber, const ImageParams &params, CircularBuffer &circularBuffer, SharedResources &shared)
{
    commonSampleLogic(shared, "default_save_directory", [&](SharedResources &shared, const std::string &saveDir)
                      {
                          std::vector<std::thread> threads;
                          setupCommonThreads(shared, saveDir, circularBuffer, params, threads);
                          grabber.start();
                          size_t frameCount = 0;
                          uint64_t lastFrameId = 0;
                          uint64_t duplicateCount = 0;
                          while (!shared.done)
                          {
                              if (shared.paused)
                              {
                                  std::this_thread::sleep_for(std::chrono::milliseconds(1));
                                  cv::waitKey(1);
                                  continue;
                              }

                              ScopedBuffer buffer(grabber);
                              uint8_t *imagePointer = buffer.getInfo<uint8_t *>(gc::BUFFER_INFO_BASE);
                              uint64_t frameId = buffer.getInfo<uint64_t>(gc::BUFFER_INFO_FRAMEID);
                              uint64_t timestamp = buffer.getInfo<uint64_t>(gc::BUFFER_INFO_TIMESTAMP);
                              bool isIncomplete = buffer.getInfo<bool>(gc::BUFFER_INFO_IS_INCOMPLETE);
                              size_t sizeFilled = buffer.getInfo<size_t>(gc::BUFFER_INFO_SIZE_FILLED);

                              if (!isIncomplete)
                              {
                                  if (frameId <= lastFrameId)
                                  {
                                      ++duplicateCount;
                                      //   std::cout << "Duplicate frame detected: FrameID=" << frameId << ", Timestamp=" << timestamp << std::endl;
                                  }
                                  else
                                  {
                                      circularBuffer.push(imagePointer);
                                      {
                                          std::lock_guard<std::mutex> displayLock(shared.displayQueueMutex);
                                          std::lock_guard<std::mutex> processingLock(shared.processingQueueMutex);
                                          shared.framesToProcess.push(frameCount);
                                          shared.framesToDisplay.push(frameCount);
                                      }
                                      shared.displayQueueCondition.notify_one();
                                      shared.processingQueueCondition.notify_one();
                                      frameCount++;
                                  }
                                  lastFrameId = frameId;
                              }
                              else
                              {
                                  //   std::cout << "Incomplete frame received: FrameID=" << frameId << std::endl;
                              }
                          }

                          grabber.stop();

                          return threads; });
}

int mib_grabber_main()
{
    try
    {

        EGenTL genTL;
        // EGrabberDiscovery egrabberDiscovery(genTL);
        // egrabberDiscovery.discover();
        EGrabber<CallbackOnDemand> grabber(genTL);
        // grabber.runScript("check-config.js"); // figureout how to load config script

        // configure(grabber);
        ImageParams params = initializeGrabber(grabber);

        CircularBuffer circularBuffer(params.bufferCount, params.imageSize);
        SharedResources shared;
        initializeBackgroundFrame(shared, params);
        shared.roi = cv::Rect(0, 0, params.width, params.height);

        temp_sample(grabber, params, circularBuffer, shared);
    }
    catch (const std::exception &e)
    {
        std::cout << "error: " << e.what() << std::endl;
    }
    return 0;
}