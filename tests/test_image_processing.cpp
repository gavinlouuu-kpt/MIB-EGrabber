#include <gtest/gtest.h>
#include "image_processing/image_processing.h"
#include <opencv2/opencv.hpp>
#include <fstream>
#include <nlohmann/json.hpp>

using json = nlohmann::json;

class ImageProcessingTest : public ::testing::Test
{
protected:
    json contour_data;

    void SetUp() override
    {
        // Load the JSON file once during setup
        std::ifstream file("test_data/test_contours.json");
        if (!file.is_open())
        {
            throw std::runtime_error("Could not open test_contours.json");
        }
        file >> contour_data;
    }

    std::vector<cv::Point> getContourFromJson(const std::string &contour_name)
    {
        std::vector<cv::Point> contour;
        const auto &points = contour_data[contour_name]["points"];

        for (const auto &point : points)
        {
            contour.push_back(cv::Point(point["x"], point["y"]));
        }
        return contour;
    }

    void TearDown() override
    {
        // Cleanup code that will be called after each test
    }

    // Helper function to check if two doubles are approximately equal
    bool isApproximatelyEqual(double a, double b, double epsilon = 0.001)
    {
        return std::abs(a - b) < epsilon;
    }

    // Add helper function to visualize contour
    void visualizeContour(const std::vector<cv::Point> &contour, const std::string &window_name)
    {
        // Create a black image
        cv::Mat image = cv::Mat::zeros(50, 50, CV_8UC1);

        // Draw the contour in white
        std::vector<std::vector<cv::Point>> contours = {contour};
        cv::drawContours(image, contours, 0, cv::Scalar(255), 1);

        // Save the image (optional)
        cv::imwrite(window_name + ".png", image);

        // Uncomment below lines for interactive visualization
        cv::imshow(window_name, image);
        cv::waitKey(0);

        // Also save the contour points
        saveContourToFile(contour, window_name);
    }

    void saveContourToFile(const std::vector<cv::Point> &contour, const std::string &filename)
    {
        std::ofstream file(filename + ".txt");
        if (!file.is_open())
        {
            std::cerr << "Failed to open file: " << filename << ".txt" << std::endl;
            return;
        }

        file << "x,y" << std::endl; // CSV header
        for (const auto &point : contour)
        {
            file << point.x << "," << point.y << std::endl;
        }
        file.close();
    }
};

TEST_F(ImageProcessingTest, CircleMetrics)
{
    // Create a perfect circle contour
    std::vector<cv::Point> circle;
    double radius = 10;
    int points = 360; // number of points to approximate circle
    for (int i = 0; i < points; i++)
    {
        double angle = 2 * M_PI * i / points;
        int x = static_cast<int>(radius * cos(angle) + radius);
        int y = static_cast<int>(radius * sin(angle) + radius);
        circle.push_back(cv::Point(x, y));
    }

    // Visualize the circle
    visualizeContour(circle, "perfect_circle");

    auto [deformability, area] = calculateMetrics(circle);

    // For a perfect circle:
    // - circularity should be very close to 1.0
    // - therefore deformability should be very close to 0.0
    EXPECT_TRUE(isApproximatelyEqual(deformability, 0.0, 0.1));

    // Area should be approximately π*r²
    double expected_area = M_PI * radius * radius;
    EXPECT_TRUE(isApproximatelyEqual(area, expected_area, expected_area * 0.1));
}

TEST_F(ImageProcessingTest, SpecificDeformabilities)
{
    struct TestCase
    {
        std::string name;
        double expected_deform;
    };

    std::vector<TestCase> test_cases = {
        {"perfect_circle", 0.0},
        {"slight_deform", 0.3},
        {"more_deform", 0.6},
        {"high_deform", 0.9}};

    for (const auto &test : test_cases)
    {
        auto contour = getContourFromJson(test.name);

        // Visualize the contour
        visualizeContour(contour, test.name);

        // Save contour points to text file
        saveContourToFile(contour, test.name);

        auto [deformability, area] = calculateMetrics(contour);
        EXPECT_TRUE(isApproximatelyEqual(deformability, test.expected_deform, 0.1))
            << "Failed for " << test.name
            << ". Expected deformability: " << test.expected_deform
            << ", got: " << deformability;
    }
}