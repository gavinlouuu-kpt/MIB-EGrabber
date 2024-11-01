#include "image_processing/image_processing.h"
#include "CircularBuffer/CircularBuffer.h"
#include "mib_grabber/mib_grabber.h"
#include <chrono>
#include <iostream>
#include <conio.h>
#include <filesystem>
#include <fstream>   // Add this for file operations
#include <string>    // Add this for std::string
#include <stdexcept> // Add this for std::runtime_error
#include <nlohmann/json.hpp>
#include <ftxui/dom/elements.hpp>
#include <ftxui/screen/screen.hpp>
#include <ftxui/screen/string.hpp>
#include <matplot/matplot.h>
#include <thread>
#include <atomic>
#ifdef _WIN32
#include <windows.h>
#else
#include <pthread.h>
#include <sched.h>
#endif

using json = nlohmann::json;

namespace fs = std::filesystem;
void simulateCameraThread(
    CircularBuffer &cameraBuffer, SharedResources &shared,
    const ImageParams &params)
{
    using clock = std::chrono::high_resolution_clock;

    size_t currentIndex = 0;
    size_t totalFrames = cameraBuffer.size();
    auto lastFrameTime = clock::now();
    auto fpsStartTime = clock::now();
    size_t frameCount = 0;
    const int targetFPS = 5000;
    const std::chrono::nanoseconds frameInterval(1000000000 / targetFPS);

    while (!shared.done)
    {
        auto now = clock::now();
        if (!shared.paused && (now - lastFrameTime) >= frameInterval)
        {
            const uint8_t *imageData = cameraBuffer.getPointer(currentIndex);
            if (imageData != nullptr)
            {
                shared.latestCameraFrame.store(currentIndex, std::memory_order_release);
                currentIndex = (currentIndex + 1) % totalFrames;
                lastFrameTime = now;
                if (++frameCount % 5000 == 0)
                {
                }
            }
            else
            {
                std::cout << "Invalid frame at index: " << currentIndex << std::endl;
            }
        }

        if (std::chrono::duration_cast<std::chrono::seconds>(now - fpsStartTime).count() >= 5)
        {
            double fps = frameCount / std::chrono::duration<double>(now - fpsStartTime).count();
            shared.currentFPS.store(fps, std::memory_order_release);
            frameCount = 0;
            fpsStartTime = now;
        }
        shared.updated = true;
    }
    std::cout << "Camera thread interrupted." << std::endl;
}

void metricDisplayThread(SharedResources &shared, const ProcessingConfig &processingConfig)
{
    using namespace ftxui;
    std::this_thread::sleep_for(std::chrono::milliseconds(100)); // sleep for 1ms to allow other things to be printed first

    auto calculateProcessingMetrics = [](const CircularBuffer &processingTimes)
    {
        double avgTime = 0.0, maxTime = 0.0, minTime = std::numeric_limits<double>::max();
        double instantTime = 0.0;
        size_t highLatencyCount = 0;            // Count of times above 200us
        const double LATENCY_THRESHOLD = 200.0; // 200us threshold

        size_t count = processingTimes.size();
        if (count > 0)
        {
            // Get latest value for instant time
            instantTime = *reinterpret_cast<const double *>(processingTimes.getPointer(0));

            // Calculate statistics
            for (size_t i = 0; i < count; i++)
            {
                const double *timePtr = reinterpret_cast<const double *>(processingTimes.getPointer(i));
                double time = *timePtr;
                avgTime += time;
                maxTime = std::max(maxTime, time);
                minTime = std::min(minTime, time);
                if (time > LATENCY_THRESHOLD)
                {
                    highLatencyCount++;
                }
            }
            avgTime /= count;
        }

        double highLatencyPercentage = count > 0 ? (highLatencyCount * 100.0) / count : 0.0;
        return std::make_tuple(instantTime, avgTime, maxTime, minTime, highLatencyPercentage);
    };
    auto calculateDeformabilityBufferRate = [](const SharedResources &shared)
    {
        // Calculate the rate of new sets added to the deformability buffer
        static auto lastCheckTime = std::chrono::steady_clock::now();
        static size_t lastBufferCount = 0;

        auto now = std::chrono::steady_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::seconds>(now - lastCheckTime).count();

        size_t currentBufferCount = shared.deformabilityBuffer.size();
        size_t addedCount = currentBufferCount - lastBufferCount;

        double rate = duration > 0 ? static_cast<double>(addedCount) / duration : 0.0;

        lastCheckTime = now;
        lastBufferCount = currentBufferCount;

        return rate;
    };

    auto render_processing_metrics = [&]()
    {
        auto [instantTime, avgTime, maxTime, minTime, highLatencyPct] = calculateProcessingMetrics(shared.processingTimes);
        double rate = calculateDeformabilityBufferRate(shared);

        return window(text("Processing Metrics"), vbox({hbox({text("Avg Processing Time: "), text(std::to_string((int)avgTime) + " us")}),
                                                        hbox({text("Max Processing Time: "), text(std::to_string((int)maxTime) + " us")}),
                                                        hbox({text("High Latency (>200us): "), text(std::to_string(highLatencyPct) + "%")}),
                                                        hbox({text("Processing Queue Size: "), text(std::to_string(shared.framesToProcess.size()) + " frames")}),
                                                        hbox({text("Deformability Buffer Size: "), text(std::to_string(shared.deformabilityBuffer.size()) + " sets")}),
                                                        hbox({text("Deformability: "), text(std::to_string(shared.frameDeformabilities.load()))}),
                                                        hbox({text("Area: "), text(std::to_string(shared.frameAreas.load()))})}));
    };

    auto render_config_metrics = [&]()
    {
        return window(text("Configuration"), vbox({hbox({text("Current FPS: "),
                                                         text(std::to_string((int)shared.currentFPS.load()))}),
                                                   hbox({text("Binary Threshold: "),
                                                         text(std::to_string(processingConfig.bg_subtract_threshold))}),
                                                   hbox({text("Contour Min Threshold: "),
                                                         text(std::to_string(processingConfig.contour_threshold_min))}),
                                                   hbox({text("Contour Max Threshold: "),
                                                         text(std::to_string(processingConfig.contour_threshold_max))})}));
    };

    auto render_status = [&]()
    {
        return window(text("Status"), vbox({
                                          hbox({text("Running: "),
                                                text(shared.running.load() ? "Yes" : "No")}),
                                          hbox({text("Paused: "),
                                                text(shared.paused.load() ? "Yes" : "No")}),
                                          hbox({text("Overlay Mode: "),
                                                text(shared.overlayMode.load() ? "Yes" : "No")}),
                                          hbox({text("Current Frame Index: "),
                                                text(std::to_string(shared.currentFrameIndex.load()))}),
                                          hbox({text("Saving Speed: "),
                                                text(std::to_string((int)shared.diskSaveTime.load()) + " ms")}),

                                      }));
    };

    auto render_keyboard_instructions = [&]()
    {
        return window(text("Keyboard Instructions"), vbox({
                                                         text("ESC: Exit program"),
                                                         text("Space: Pause/Resume live feed"),
                                                         text("When Paused:"),
                                                         text("  B: Set current frame as background"),
                                                         text("  A: Next frame"),
                                                         text("  D: Previous frame"),
                                                         text("Display Options:"),
                                                         text("  P: Toggle processed image overlay"),
                                                         text("  Q: Clear deformability buffer"),
                                                         text("Data Management:"),
                                                         text("  R: Toggle data recording"),
                                                         text("  S: Save all frames to disk"),
                                                         text("  F: Configure eGrabber settings"),
                                                         text("ROI: Click and drag to select region"),
                                                     }));
    };

    std::string reset_position;
    while (!shared.done)
    {
        if (shared.updated)
        {
            auto document = hbox({
                render_processing_metrics(),
                render_config_metrics(),
                // render_roi(),
                render_status(),
                render_keyboard_instructions(),
            });

            auto screen = Screen::Create(Dimension::Full(), Dimension::Fit(document));
            Render(screen, document);
            std::cout << reset_position;
            screen.Print();
            reset_position = screen.ResetPosition();
            shared.updated = false;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
}

void processingThreadTask(
    std::mutex &processingQueueMutex,
    std::condition_variable &processingQueueCondition,
    std::queue<size_t> &framesToProcess,
    const CircularBuffer &processingBuffer,
    size_t width, size_t height, SharedResources &shared, const ProcessingConfig &processingConfig)
{
    // Pre-allocate memory for images
    cv::Mat inputImage(height, width, CV_8UC1);
    cv::Mat processedImage(height, width, CV_8UC1);
    ThreadLocalMats mats = initializeThreadMats(height, width, processingConfig);
    const size_t BUFFER_THRESHOLD = 1000; // Adjust as needed
    const size_t contour_threshold = 10;
    const uint8_t processedColor = 255; // grey scaled cell color

    while (!shared.done)
    {
        std::unique_lock<std::mutex> lock(processingQueueMutex);
        processingQueueCondition.wait(lock, [&]()
                                      { return !framesToProcess.empty() || shared.done || shared.paused; });

        if (shared.done)
            break;

        if (!framesToProcess.empty() && !shared.paused)
        {
            // size_t frame = framesToProcess.front(); //retrieve content of queue
            framesToProcess.pop();
            lock.unlock();

            auto startTime = std::chrono::high_resolution_clock::now();

            auto imageData = processingBuffer.get(0);
            inputImage = cv::Mat(height, width, CV_8UC1, imageData.data());

            // Check if ROI is the same as the full image
            if (static_cast<size_t>(shared.roi.width) != width && static_cast<size_t>(shared.roi.height) != height)
            {
                // Preprocess Image using the optimized processFrame function
                processFrame(inputImage, shared, processedImage, mats, processingConfig);
                bool touchesBorder = false;
                bool hasContent = false;
                // Check left and right edges
                cv::Mat roiImage = processedImage(shared.roi);
                for (int y = 0; y < roiImage.rows && !touchesBorder; y++)
                {
                    if (roiImage.at<uint8_t>(y, 0) == processedColor || // Left edge of ROI
                        roiImage.at<uint8_t>(y, roiImage.cols - 1) == processedColor)
                    { // Right edge of ROI
                        touchesBorder = true;
                    }
                }

                // Only proceed with contour detection if no border pixels were found
                if (!touchesBorder)
                {
                    // Find contour
                    ContourResult contourResult = findContours(processedImage); // Assuming findContours is modified to accept pre-allocated contours
                    std::vector<std::vector<cv::Point>> contours = contourResult.contours;
                    double contourFindTime = contourResult.findTime;

                    // Calculate deformability and circularity for each contour
                    {
                        std::lock_guard<std::mutex> circularitiesLock(shared.deformabilityBufferMutex);
                        // bool qualifiedContourFound = false;
                        for (const auto &contour : contours)
                        {
                            if (contour.size() >= processingConfig.contour_threshold_min &&
                                contour.size() <= processingConfig.contour_threshold_max)
                            {

                                auto [deformability, area] = calculateMetrics(contour);
                                auto metrics = std::make_tuple(deformability, area);
                                shared.deformabilityBuffer.push(reinterpret_cast<const uint8_t *>(&metrics));
                                shared.newScatterDataAvailable = true;
                                shared.scatterDataCondition.notify_one();
                                // create a flag to check if user wants to save
                                if (shared.running)
                                {
                                    // qualifiedContourFound = true;
                                    QualifiedResult qualifiedResult;
                                    qualifiedResult.timestamp = std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
                                    qualifiedResult.deformability = deformability;
                                    qualifiedResult.area = area;
                                    // qualifiedResult.contourResult = contourResult;
                                    qualifiedResult.originalImage = inputImage.clone();

                                    std::lock_guard<std::mutex> qualifiedResultsLock(shared.qualifiedResultsMutex);
                                    auto &currentBuffer = shared.usingBuffer1 ? shared.qualifiedResultsBuffer1 : shared.qualifiedResultsBuffer2;
                                    currentBuffer.push_back(std::move(qualifiedResult));

                                    // Check if we need to switch buffers
                                    if (currentBuffer.size() >= BUFFER_THRESHOLD && !shared.savingInProgress)
                                    {
                                        shared.usingBuffer1 = !shared.usingBuffer1;
                                        shared.savingInProgress = true;
                                        shared.savingCondition.notify_one();
                                    }
                                }
                            }
                        }
                    }
                }
            }

            auto endTime = std::chrono::high_resolution_clock::now();
            auto duration = std::chrono::duration_cast<std::chrono::microseconds>(endTime - startTime);
            double processingTime = duration.count();

            // Just store the processing time
            shared.processingTimes.push(reinterpret_cast<const uint8_t *>(&processingTime));
            shared.updated = true;
        }
        else
        {
            lock.unlock();
        }
    }
    std::cout << "Processing thread interrupted." << std::endl;
}

void displayThreadTask(
    std::queue<size_t> &framesToDisplay,
    std::mutex &displayQueueMutex,
    const CircularBuffer &circularBuffer,
    size_t width,
    size_t height,
    size_t bufferCount,
    SharedResources &shared, const ProcessingConfig &processingConfig)
{
    const double displayFPS = 60.0;

    const std::chrono::duration<double> frameDuration(1.0 / displayFPS); // Increase to 60 FPS for smoother response
    auto nextFrameTime = std::chrono::steady_clock::now();
    ThreadLocalMats mats = initializeThreadMats(height, width, processingConfig);

    // Pre-allocate memory for images
    cv::Mat image(height, width, CV_8UC1);
    cv::Mat processedImage(height, width, CV_8UC1);
    cv::Mat displayImage(height, width, CV_8UC3);

    cv::namedWindow("Live Feed", cv::WINDOW_NORMAL);
    cv::resizeWindow("Live Feed", width, height);

    int trackbarPos = 0;
    cv::createTrackbar("Frame", "Live Feed", &trackbarPos, bufferCount - 1, onTrackbar, &shared);

    auto updateDisplay = [&](const cv::Mat &originalImage, const cv::Mat &processedImage)
    {
        // Convert original image to color
        cv::cvtColor(originalImage, displayImage, cv::COLOR_GRAY2BGR);

        if (shared.overlayMode)
        {
            // Create a mask from the processed image
            cv::Mat mask = (processedImage > 0);

            // Overlay processed image in red
            displayImage.setTo(cv::Scalar(0, 0, 255), mask);
        }

        // Draw ROI rectangle
        {
            std::lock_guard<std::mutex> roiLock(shared.roiMutex);
            cv::rectangle(displayImage, shared.roi, cv::Scalar(0, 255, 0), 1);
        }

        cv::imshow("Live Feed", displayImage);
    };

    // Function to handle mouse events for ROI selection
    auto onMouse = [](int event, int x, int y, int flags, void *userdata)
    {
        static cv::Point startPoint;
        auto *sharedResources = static_cast<SharedResources *>(userdata);

        if (event == cv::EVENT_LBUTTONDOWN)
        {
            startPoint = cv::Point(x, y);
        }
        else if (event == cv::EVENT_LBUTTONUP)
        {
            cv::Point endPoint(x, y);
            double distance = cv::norm(startPoint - endPoint);

            if (distance > 5)
            {
                cv::Rect newRoi(startPoint, endPoint);
                std::lock_guard<std::mutex> lock(sharedResources->roiMutex);
                sharedResources->roi = newRoi;
                sharedResources->displayNeedsUpdate = true;
            }
        }
    };

    cv::setMouseCallback("Live Feed", onMouse, &shared);

    while (!shared.done)
    {
        auto now = std::chrono::steady_clock::now();
        bool shouldUpdate = false;

        if (!shared.paused)
        {
            if (now >= nextFrameTime)
            {
                std::unique_lock<std::mutex> lock(displayQueueMutex);
                if (!framesToDisplay.empty())
                {
                    framesToDisplay.pop();
                    lock.unlock();

                    auto imageData = circularBuffer.get(0);
                    image = cv::Mat(height, width, CV_8UC1, imageData.data());
                    processFrame(image, shared, processedImage, mats, processingConfig);
                    updateDisplay(image, processedImage);
                    shouldUpdate = true;

                    nextFrameTime += std::chrono::duration_cast<std::chrono::steady_clock::duration>(frameDuration);
                    if (nextFrameTime < now)
                    {
                        nextFrameTime = now + std::chrono::duration_cast<std::chrono::steady_clock::duration>(frameDuration);
                    }
                }
            }
        }
        else
        {
            if (shared.displayNeedsUpdate)
            {
                int index = shared.currentFrameIndex;
                if (index >= 0 && index < circularBuffer.size())
                {
                    auto imageData = circularBuffer.get(index);
                    if (!imageData.empty())
                    {
                        // read config to enable hot reloading of image processing parameters
                        auto config = readConfig("config.json");

                        ProcessingConfig newConfig{
                            config["image_processing"]["gaussian_blur_size"],
                            config["image_processing"]["bg_subtract_threshold"],
                            config["image_processing"]["morph_kernel_size"],
                            config["image_processing"]["morph_iterations"],
                            config["image_processing"]["contour_threshold_min"],
                            config["image_processing"]["contour_threshold_max"]};

                        image = cv::Mat(height, width, CV_8UC1, imageData.data());
                        processFrame(image, shared, processedImage, mats, newConfig);
                        // find contours and calculate deformabilities and areas
                        ContourResult contourResult = findContours(processedImage);
                        std::vector<std::vector<cv::Point>> contours = contourResult.contours;
                        // TODO: only display the largest contour
                        if (contours.empty())
                        {
                            // No contours found at all
                            processedImage = cv::Mat::zeros(processedImage.size(), CV_8UC1);
                            updateDisplay(image, processedImage);
                        }
                        else
                        {
                            for (const auto &contour : contours)
                            {
                                if (contour.size() >= newConfig.contour_threshold_min &&
                                    contour.size() <= newConfig.contour_threshold_max)
                                {
                                    auto [deformability, area] = calculateMetrics(contour);
                                    shared.frameDeformabilities.store(deformability);
                                    shared.frameAreas.store(area);
                                    updateDisplay(image, processedImage);
                                }
                                else
                                {
                                    processedImage = cv::Mat::zeros(processedImage.size(), CV_8UC1);
                                    updateDisplay(image, processedImage);
                                }
                            }
                        }

                        cv::setTrackbarPos("Frame", "Live Feed", index);
                        shouldUpdate = true;
                    }
                }
                shared.displayNeedsUpdate = false;
            }
        }

        // Handle keyboard input more frequently
        for (int i = 0; i < 5; i++)
        { // Check multiple times per frame
            int key = cv::waitKey(1) & 0xFF;
            if (key != -1 && shared.keyboardCallback)
            {
                std::lock_guard<std::mutex> lock(shared.keyboardMutex);
                shared.keyboardCallback(key);
            }

            // If we're not updating the display, add a small sleep
            if (!shouldUpdate)
            {
                std::this_thread::sleep_for(std::chrono::milliseconds(1));
            }
        }

        // Only call imshow when we actually need to update the display
        if (shouldUpdate)
        {
            cv::imshow("Live Feed", displayImage);
        }
    }

    cv::destroyAllWindows();
    std::cout << "Display thread interrupted." << std::endl;
}

void onTrackbar(int pos, void *userdata)
{
    auto *shared = static_cast<SharedResources *>(userdata);
    shared->currentFrameIndex = pos;
    shared->displayNeedsUpdate = true;
}

void updateScatterPlot(SharedResources &shared)
{
    using namespace matplot;

    // Create figure and axes only once
    auto f = figure(true);
    f->quiet_mode(true); // Reduce console output
    auto ax = f->current_axes();

    // Pre-allocate vectors with a reasonable capacity
    std::vector<double> x, y;
    x.reserve(1000);
    y.reserve(1000);

    auto sc = ax->scatter(x, y);
    ax->xlabel("Area");
    ax->ylabel("Deformability");
    ax->title("Deformability vs Area");

    // Reduce update frequency
    const auto updateInterval = std::chrono::milliseconds(5000);
    auto lastUpdateTime = std::chrono::steady_clock::now();

    while (!shared.done)
    {
        auto now = std::chrono::steady_clock::now();
        if (now - lastUpdateTime < updateInterval)
        {
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
            continue;
        }

        bool needsUpdate = false;
        {
            std::lock_guard<std::mutex> lock(shared.deformabilityBufferMutex);
            if (shared.newScatterDataAvailable)
            {
                x.clear();
                y.clear();

                // Reserve exact capacity to avoid reallocations
                size_t size = shared.deformabilityBuffer.size();
                if (x.capacity() < size)
                {
                    x.reserve(size);
                    y.reserve(size);
                }

                // Read from circular buffer
                for (size_t i = 0; i < size; i++)
                {
                    const auto *metrics = reinterpret_cast<const std::tuple<double, double> *>(
                        shared.deformabilityBuffer.getPointer(i));
                    if (metrics)
                    {
                        x.push_back(std::get<1>(*metrics)); // area
                        y.push_back(std::get<0>(*metrics)); // deformability
                    }
                }

                shared.newScatterDataAvailable = false;
                needsUpdate = true;
            }
        }

        if (needsUpdate)
        {
            sc->x_data(x);
            sc->y_data(y);
            f->draw();
            lastUpdateTime = now;
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
}

void keyboardHandlingThread(
    const CircularBuffer &circularBuffer,
    size_t bufferCount, size_t width, size_t height,
    SharedResources &shared)
{
    auto handleKeypress = [&](int key)
    {
        if (key == 27)
        { // ESC key
            shared.done = true;
            shared.displayQueueCondition.notify_all();
            shared.processingQueueCondition.notify_all();
            shared.savingCondition.notify_all();
        }
        else if (key == 32)
        { // Space bar
            shared.paused = !shared.paused;
            if (shared.paused)
            {
                shared.currentFrameIndex = 0;
            }
        }
        else if ((key == 'a' || key == 'A') && shared.paused && shared.currentFrameIndex < circularBuffer.size() - 1)
        {
            shared.currentFrameIndex++;
            shared.displayNeedsUpdate = true;
        }
        else if ((key == 'd' || key == 'D') && shared.paused && shared.currentFrameIndex > 0)
        {
            shared.currentFrameIndex--;
            shared.displayNeedsUpdate = true;
        }
        else if (key == 'f' || key == 'F')
        {
            configure_js("egrabberConfig.js");
        }
        else if (key == 'p' || key == 'P')
        {
            shared.overlayMode = !shared.overlayMode;
        }
        else if (key == 'q' || key == 'Q')
        {
            // std::lock_guard<std::mutex> lock(shared.deformabilityBufferMutex);
            // shared.deformabilityBuffer.clear();
        }
        else if (key == 's' || key == 'S')
        {
            std::filesystem::path outputDir = "stream_output";
            if (!std::filesystem::exists(outputDir))
            {
                std::filesystem::create_directory(outputDir);
            }
            // Find the next available numbered folder
            int folderNum = 1;
            while (std::filesystem::exists(outputDir / std::to_string(folderNum)))
            {
                folderNum++;
            }

            // Create the numbered subfolder
            std::filesystem::path currentSaveDir = outputDir / std::to_string(folderNum);
            std::filesystem::create_directory(currentSaveDir);

            size_t frameCount = circularBuffer.size();
            // std::cout << "Saving " << frameCount << " frames..." << std::endl;

            for (size_t i = 0; i < frameCount; ++i)
            {
                auto imageData = circularBuffer.get(frameCount - 1 - i); // Start from oldest frame
                cv::Mat image(height, width, CV_8UC1, imageData.data());

                std::ostringstream oss;
                oss << "frame_" << std::setw(5) << std::setfill('0') << i << ".png";
                std::string filename = oss.str();

                std::filesystem::path fullPath = currentSaveDir / filename;

                cv::imwrite(fullPath.string(), image);
                // std::cout << "Saved frame " << (i + 1) << "/" << frameCount << ": " << filename << "\r" << std::flush;
            }
        }
        else if ((key == 'b' || key == 'B') && shared.paused)
        {
            auto backgroundImageData = circularBuffer.get(shared.currentFrameIndex);
            {
                std::lock_guard<std::mutex> lock(shared.backgroundFrameMutex);
                shared.backgroundFrame = cv::Mat(height, width, CV_8UC1, backgroundImageData.data()).clone();
                cv::GaussianBlur(shared.backgroundFrame, shared.blurredBackground, cv::Size(3, 3), 0);
            }
            shared.displayNeedsUpdate = true;
        }
        else if (key == 'r' || key == 'R')
        {
            shared.running = !shared.running;
        }
        shared.updated = true;
    };

    // Store the callback for use in displayThreadTask
    shared.keyboardCallback = handleKeypress;

    // Handle console input
    while (!shared.done)
    {
        if (_kbhit())
        {
            int ch = _getch();
            handleKeypress(ch);
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
    std::cout << "Keyboard handling thread interrupted." << std::endl;
}

void resultSavingThread(SharedResources &shared, const std::string &saveDirectory)
{
    while (!shared.done)
    {
        std::vector<QualifiedResult> bufferToSave;
        {
            std::unique_lock<std::mutex> lock(shared.qualifiedResultsMutex);
            shared.savingCondition.wait(lock, [&shared]()
                                        { return shared.savingInProgress || shared.done; });

            if (shared.done)
                break;

            // Swap the full buffer with our local vector
            if (shared.usingBuffer1)
            {
                bufferToSave.swap(shared.qualifiedResultsBuffer2);
            }
            else
            {
                bufferToSave.swap(shared.qualifiedResultsBuffer1);
            }
        }

        // Save the buffer to disk
        if (!bufferToSave.empty())
        {
            auto start = std::chrono::steady_clock::now();
            saveQualifiedResultsToDisk(bufferToSave, saveDirectory, shared);
            auto end = std::chrono::steady_clock::now();
            auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
            shared.diskSaveTime = duration.count();

            shared.totalSavedResults += bufferToSave.size();
            shared.lastSaveTime = end;

            // std::cout << "Saved " << bufferToSave.size() << " results to disk. "
            //           << "Total saved: " << shared.totalSavedResults
            //           << ". Time taken: " << duration.count() << " ms" << std::endl;
        }

        // Mark saving as complete
        {
            std::lock_guard<std::mutex> lock(shared.qualifiedResultsMutex);
            shared.savingInProgress = false;
        }
        shared.updated = true;
    }
    std::cout << "Result saving thread interrupted." << std::endl;
}

void commonSampleLogic(SharedResources &shared, const std::string &SAVE_DIRECTORY,
                       std::function<std::vector<std::thread>(SharedResources &, const std::string &, const ProcessingConfig &)> setupThreads)
{
    shared.done = false;
    shared.paused = false;
    shared.currentFrameIndex = -1;
    shared.displayNeedsUpdate = true;
    shared.deformabilityBuffer.clear();
    shared.qualifiedResults.clear();
    shared.totalSavedResults = 0;

    // Create output directory if it doesn't exist
    std::filesystem::path outputDir("output");
    if (!std::filesystem::exists(outputDir))
    {
        std::filesystem::create_directory(outputDir);
    }

    // Create egrabberConfig.js if it doesn't exist
    std::filesystem::path configFile("egrabberConfig.js");
    if (!std::filesystem::exists(configFile))
    {
        std::ofstream file(configFile);
        file << "// Decrease the resolution before increasing the frame rate\n\n"
             << "// var g = grabbers[0];\n"
             << "// g.RemotePort.set(\"Width\", 512);\n"
             << "// g.RemotePort.set(\"Height\", 96);\n"
             << "// g.RemotePort.set(\"AcquisitionFrameRate\", 5000);\n\n"
             << "// Decrease the frame rate before upscaling to 1920x1080\n\n"
             << "// var g = grabbers[0];\n"
             << "// g.RemotePort.set(\"AcquisitionFrameRate\", 25);\n"
             << "// g.RemotePort.set(\"Width\", 1920);\n"
             << "// g.RemotePort.set(\"Height\", 1080);\n";
        file.close();
    }
    // saving UI block
    json config = readConfig("config.json");
    // Initialize processing configuration
    ProcessingConfig processingConfig = ProcessingConfig{
        config["image_processing"]["gaussian_blur_size"],
        config["image_processing"]["bg_subtract_threshold"],
        config["image_processing"]["morph_kernel_size"],
        config["image_processing"]["morph_iterations"],
        config["image_processing"]["contour_threshold_min"],
        config["image_processing"]["contour_threshold_max"]};
    std::string saveDir = config["save_directory"];

    std::cout << "Current save directory: " << saveDir << std::endl;
    std::cout << "Choose save directory option:\n";
    std::cout << "1: Use current directory\n";
    std::cout << "2: Enter new directory\n";
    std::cout << "3: Use testing directory (will overwrite existing)\n";
    std::cout << "Choice: ";
    int choice;
    std::cin >> choice;

    if (choice == 2)
    {
        std::cout << "Enter new save directory name: ";
        std::cin >> saveDir;

        // Update the config file with the new base directory
        updateConfig("config.json", "save_directory", saveDir);
    }
    else if (choice == 3)
    {
        saveDir = "testing";
        // Remove existing testing directory if it exists
        std::filesystem::path testPath = outputDir / saveDir;
        if (std::filesystem::exists(testPath))
        {
            std::filesystem::remove_all(testPath);
        }
        // Update the config file with the testing directory
        updateConfig("config.json", "save_directory", saveDir);
    }

    // Create full path within output directory
    std::filesystem::path fullPath = outputDir / saveDir;
    std::string basePath = fullPath.string();
    shared.saveDirectory = basePath;

    // Automatically increment the directory name if it already exists
    int suffix = 1;
    while (std::filesystem::exists(fullPath))
    {
        fullPath = outputDir / (saveDir + "_" + std::to_string(suffix));
        suffix++;
    }

    // Create the subdirectory
    std::filesystem::create_directories(fullPath);

    std::cout << "Using save directory: " << fullPath.string() << std::endl;

    // Call the setup function passed as parameter
    std::vector<std::thread> threads = setupThreads(shared, fullPath.string(), processingConfig);

    // Wait for completion
    shared.displayQueueCondition.notify_all();
    shared.processingQueueCondition.notify_all();
    shared.savingCondition.notify_all();
    std::cout << "Joining threads..." << std::endl;
    for (auto &thread : threads)
    {
        thread.join();
    }
}

void setupCommonThreads(SharedResources &shared, const std::string &saveDir,
                        const CircularBuffer &circularBuffer, const CircularBuffer &processingBuffer, const ImageParams &params,
                        std::vector<std::thread> &threads, const ProcessingConfig &processingConfig)
{
    // Create processing thread first and set its priority
    threads.emplace_back(processingThreadTask,
                         std::ref(shared.processingQueueMutex), std::ref(shared.processingQueueCondition),
                         std::ref(shared.framesToProcess), std::ref(processingBuffer),
                         params.width, params.height, std::ref(shared), processingConfig);
    // Create remaining threads with normal priority
    threads.emplace_back(displayThreadTask, std::ref(shared.framesToDisplay),
                         std::ref(shared.displayQueueMutex), std::ref(circularBuffer),
                         params.width, params.height, params.bufferCount, std::ref(shared), processingConfig);

    threads.emplace_back(keyboardHandlingThread,
                         std::ref(circularBuffer), params.bufferCount, params.width, params.height, std::ref(shared));

    threads.emplace_back(resultSavingThread, std::ref(shared), saveDir);
    threads.emplace_back(metricDisplayThread, std::ref(shared), processingConfig);

    // Read from json to check if scatterplot is enabled
    json config = readConfig("config.json");
    bool scatterPlotEnabled = config.value("scatter_plot_enabled", false);

    if (scatterPlotEnabled)
    {
        threads.emplace_back(updateScatterPlot, std::ref(shared));
    }
}

void temp_mockSample(const ImageParams &params, CircularBuffer &cameraBuffer, CircularBuffer &circularBuffer, CircularBuffer &processingBuffer, SharedResources &shared)
{
    commonSampleLogic(shared, "default_save_directory", [&](SharedResources &shared, const std::string &saveDir, const ProcessingConfig &processingConfig)
                      {
                          std::vector<std::thread> threads;
                          setupCommonThreads(shared, saveDir, circularBuffer, processingBuffer, params, threads, processingConfig);

                          threads.emplace_back(simulateCameraThread,
                                               std::ref(cameraBuffer), std::ref(shared), std::ref(params));

                          size_t lastProcessedFrame = 0;
                          while (!shared.done)
                          {
                              if (shared.paused)
                              {
                                  std::this_thread::sleep_for(std::chrono::milliseconds(1));
                                  continue;
                              }
                              size_t latestFrame = shared.latestCameraFrame.load(std::memory_order_acquire);
                              if (latestFrame != lastProcessedFrame)
                              {
                                  const uint8_t *imageData = cameraBuffer.getPointer(latestFrame);
                                  if (imageData != nullptr)
                                  {
                                      circularBuffer.push(imageData);
                                      processingBuffer.push(imageData);
                                      {
                                          std::lock_guard<std::mutex> displayLock(shared.displayQueueMutex);
                                          std::lock_guard<std::mutex> processingLock(shared.processingQueueMutex);
                                          shared.framesToProcess.push(latestFrame);
                                          shared.framesToDisplay.push(latestFrame);
                                      }
                                      shared.displayQueueCondition.notify_one();
                                      shared.processingQueueCondition.notify_one();
                                      lastProcessedFrame = latestFrame;
                                  }
                              }
                          }
                          return threads; });
}
