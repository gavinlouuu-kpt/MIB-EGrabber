#include <iostream>
#include <string>
#include <limits>
#include "image_processing/image_processing.h"
#include "CircularBuffer/CircularBuffer.h"
#include "menu_system/menu_system.h"
#include <EGrabber.h>
#include <ftxui/dom/elements.hpp>
#include <ftxui/screen/screen.hpp>
#include <ftxui/component/component.hpp>
#include <ftxui/component/screen_interactive.hpp>
#include <filesystem>

namespace MenuSystem
{

    void clearInputBuffer()
    {
        std::cin.clear();
        std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
    }

    void displayMenu()
    {
        std::cout << "\n=== Cell Analysis Menu ===\n";
        std::cout << "1. Run Mock Sample\n";
        std::cout << "2. Run Live Sample\n";
        std::cout << "3. Convert Saved Images\n";
        std::cout << "4. Exit\n";
        std::cout << "Enter your choice: ";
    }

    void runMockSample()
    {
        std::string imageDirectory;
        std::cout << "Enter the image directory path: ";
        std::getline(std::cin, imageDirectory);

        try
        {
            ImageParams params = initializeImageParams(imageDirectory);
            CircularBuffer cameraBuffer(params.bufferCount, params.imageSize);
            CircularBuffer circularBuffer(params.bufferCount, params.imageSize);

            loadImages(imageDirectory, cameraBuffer, true);

            SharedResources shared;
            initializeMockBackgroundFrame(shared, params, cameraBuffer);
            shared.roi = cv::Rect(0, 0, static_cast<int>(params.width), static_cast<int>(params.height));

            temp_mockSample(params, cameraBuffer, circularBuffer, shared);

            std::cout << "Mock sampling completed.\n";
        }
        catch (const std::exception &e)
        {
            std::cerr << "Error: " << e.what() << std::endl;
        }
    }

    void runLiveSample()
    {
        // Placeholder for live sampling implementation

        std::cout << "Live sampling not implemented yet.\n";
    }

    void convertSavedImages()
    {
        std::string saveDirectory;

        std::cout << "Enter the path to the save directory containing batch folders: ";
        std::getline(std::cin, saveDirectory);

        try
        {
            processAllBatches(saveDirectory);
        }
        catch (const std::exception &e)
        {
            std::cerr << "Error: " << e.what() << std::endl;
        }
    }

    void processAllBatches(const std::string &saveDirectory)
    {
        namespace fs = std::filesystem;

        for (const auto &entry : fs::directory_iterator(saveDirectory))
        {
            if (entry.is_directory() && entry.path().filename().string().find("batch_") == 0)
            {
                std::string batchPath = entry.path().string();
                std::string imagesBinPath = batchPath + "/images.bin";

                if (fs::exists(imagesBinPath))
                {
                    std::cout << "Processing: " << imagesBinPath << std::endl;
                    try
                    {
                        convertSavedImagesToStandardFormat(imagesBinPath, batchPath);
                    }
                    catch (const std::exception &e)
                    {
                        std::cerr << "Error processing " << imagesBinPath << ": " << e.what() << std::endl;
                    }
                }
                else
                {
                    std::cout << "Skipping " << batchPath << ": images.bin not found" << std::endl;
                }
            }
        }

        std::cout << "Finished processing all batches." << std::endl;
    }

    int runMenu()
    {
        using namespace ftxui;

        int selected = 0;
        std::vector<std::string> entries = {
            "Run Mock Sample",
            "Run Live Sample",
            "Convert Saved Images",
            "Exit"};

        auto menu = Menu(&entries, &selected);

        auto screen = ScreenInteractive::TerminalOutput();

        bool quit = false;

        auto renderer = Renderer(menu, [&]
                                 { return vbox({
                                       text("=== Cell Analysis Menu ===") | bold | color(Color::Blue),
                                       separator(),
                                       menu->Render() | frame | border,
                                   }); });

        auto event_handler = CatchEvent(renderer, [&](Event event)
                                        {
            if (event == Event::Return) {
                quit = true;
                screen.ExitLoopClosure()();
                return true;
            }
            return false; });

        screen.Loop(event_handler);

        if (quit)
        {
            switch (selected)
            {
            case 0:
                runMockSample();
                break;
            case 1:
                runLiveSample();
                break;
            case 2:
                convertSavedImages();
                break;
            case 3:
                std::cout << "Exiting program.\n";
                return 0;
            }
        }

        return runMenu(); // Recursive call to show menu again after action
    }

} // namespace MenuSystem
