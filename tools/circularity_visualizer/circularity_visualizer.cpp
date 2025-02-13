#include <opencv2/opencv.hpp>
#include <vector>
#include <cmath>
#include <fstream>
#include "image_processing/image_processing.h"
#include <iomanip>
#include <sstream>

class CircularityVisualizer
{
private:
    cv::Mat canvas;
    std::string window_name;
    int canvas_size;
    double deformation;
    int num_points;
    double base_radius;

    static void onTrackbar(int, void *userdata)
    {
        auto *visualizer = static_cast<CircularityVisualizer *>(userdata);
        visualizer->updateDisplay();
    }

    std::vector<cv::Point> generateDeformedCircle()
    {
        std::vector<cv::Point> contour;
        double center_x = canvas_size / 2;
        double center_y = canvas_size * 0.625; // Move down by 25% (0.5 + 0.125 = 0.625)

        // Calculate vertical scale to maintain constant area
        // Area = π * a * b, where a and b are semi-major and semi-minor axes
        // If we stretch horizontally by (1 + deformation), we need to shrink vertically by 1/(1 + deformation)
        double horizontal_scale = 1.0 + deformation;
        double vertical_scale = 1.0 / horizontal_scale; // This maintains constant area

        for (int i = 0; i < num_points; i++)
        {
            double angle = 2 * M_PI * i / num_points;
            // Scale x and y differently to maintain constant area
            int x = static_cast<int>(center_x + base_radius * cos(angle) * horizontal_scale);
            int y = static_cast<int>(center_y + base_radius * sin(angle) * vertical_scale);
            contour.push_back(cv::Point(x, y));
        }
        return contour;
    }

    void saveMetrics(const std::string &basename, const std::vector<cv::Point> &contour,
                     double circularity, double deformability, double area)
    {
        std::ofstream file(basename + "_metrics.txt");
        if (file.is_open())
        {
            // Save metrics
            file << "Metrics:" << std::endl
                 << "-----------------" << std::endl
                 << "Stretch: " << deformation << std::endl
                 << "Circularity: " << circularity << std::endl
                 << "Deformability: " << deformability << std::endl
                 << "Area: " << area << " pixels" << std::endl
                 << "Base Area: " << M_PI * base_radius * base_radius << " pixels" << std::endl
                 << std::endl;

            // Save coordinates
            file << "Contour Coordinates:" << std::endl
                 << "-----------------" << std::endl
                 << "x,y" << std::endl;

            for (const auto &point : contour)
            {
                file << point.x << "," << point.y << std::endl;
            }
            file.close();
        }
    }

public:
    CircularityVisualizer(int size = 400)
        : canvas_size(size),
          window_name("Circularity Visualizer"),
          deformation(0.0),
          num_points(360),
          base_radius(size / 4) // Store the base radius
    {
        canvas = cv::Mat::zeros(canvas_size, canvas_size, CV_8UC3);
        cv::namedWindow(window_name);

        // Create deformation trackbar
        int max_deform = 100;
        cv::createTrackbar("Horizontal Stretch %", window_name, nullptr, max_deform, onTrackbar, this);
        cv::setTrackbarPos("Horizontal Stretch %", window_name, 0);
    }

    void updateDisplay()
    {
        canvas = cv::Mat::zeros(canvas_size, canvas_size, CV_8UC3);

        // Get deformation value from trackbar (0-100) and convert to 0.0-1.0
        deformation = cv::getTrackbarPos("Horizontal Stretch %", window_name) / 100.0;

        // Generate and draw contour
        auto contour = generateDeformedCircle();
        std::vector<std::vector<cv::Point>> contours = {contour};
        cv::drawContours(canvas, contours, 0, cv::Scalar(0, 255, 0), 2);

        // Calculate and display metrics
        auto [deformability, area] = calculateMetrics(contour);
        double circularity = 1.0 - deformability;
        double base_area = M_PI * base_radius * base_radius;

        // Format numbers to 3 decimal places
        auto formatNumber = [](double value)
        {
            std::stringstream ss;
            ss << std::fixed << std::setprecision(3) << value;
            return ss.str();
        };

        // Display metrics with padding from the left
        int left_padding = 20;
        int line_height = 30;
        int y_pos = 30;

        // Add background rectangle for text
        cv::Rect text_bg(10, 10, 250, 130);

        // Display metrics on the background
        cv::putText(canvas, "Circularity: " + formatNumber(circularity),
                    cv::Point(left_padding, y_pos),
                    cv::FONT_HERSHEY_SIMPLEX, 0.7, cv::Scalar(255, 255, 255), 2);

        y_pos += line_height;
        cv::putText(canvas, "Deformability: " + formatNumber(deformability),
                    cv::Point(left_padding, y_pos),
                    cv::FONT_HERSHEY_SIMPLEX, 0.7, cv::Scalar(255, 255, 255), 2);

        y_pos += line_height;
        cv::putText(canvas, "Area: " + formatNumber(area) + " px",
                    cv::Point(left_padding, y_pos),
                    cv::FONT_HERSHEY_SIMPLEX, 0.7, cv::Scalar(255, 255, 255), 2);

        y_pos += line_height;
        double area_ratio = area / base_area;
        cv::putText(canvas, "Area Ratio: " + formatNumber(area_ratio),
                    cv::Point(left_padding, y_pos),
                    cv::FONT_HERSHEY_SIMPLEX, 0.7, cv::Scalar(255, 255, 255), 2);

        cv::imshow(window_name, canvas);
    }

    void run()
    {
        updateDisplay();
        while (true)
        {
            // Use a shorter wait time (1ms) and check all possible keys
            char key = cv::waitKey(1) & 0xFF; // Mask to handle different platforms consistently

            switch (key)
            {
            case 27: // ESC
                return;

            case 's': // Save
            {
                // Generate base filename without extension
                std::string basename = "shape_stretch" + std::to_string(deformation);

                // Save image
                cv::imwrite(basename + ".png", canvas);

                // Get contour and calculate metrics
                auto contour = generateDeformedCircle();
                auto [deformability, area] = calculateMetrics(contour);
                double circularity = 1.0 - deformability;

                // Save metrics and coordinates to text file
                saveMetrics(basename, contour, circularity, deformability, area);
                break;
            }

            case ' ': // Spacebar - reset to circle
            {
                cv::setTrackbarPos("Horizontal Stretch %", window_name, 0);
                updateDisplay();
                break;
            }

            case 'r': // Reset to circle (alternative)
            {
                cv::setTrackbarPos("Horizontal Stretch %", window_name, 0);
                updateDisplay();
                break;
            }

            default:
                // Check if trackbar was modified
                if (cv::getTrackbarPos("Horizontal Stretch %", window_name) != deformation * 100)
                {
                    updateDisplay();
                }
                break;
            }
        }
    }
};

int main()
{
    CircularityVisualizer visualizer;
    visualizer.run();
    return 0;
}