# Cell Analysis Project

## Getting Started

To start the project, follow these steps:

In CMakeLists.txt select the EGrabber that is for testing if you don't have a camera to work with.

Before starting the project, you will need to install Visual Studio 2022 community edition with C++ development tools. A package manager called vcpkg is included in the installation; add the tool's path to PATH and create a system variable "VCPKG_ROOT" that corresponds to the path to vcpkg. Then, modify the CMakeUserPreset to point the vcpkg path to the path on your system.

1. Navigate to the project's root, where CMakeLists.txt and README.md are located.

2. Run the following command to set up vcpkg and install the relevant libraries:

   ```
   cmake --preset=default
   ```

   This will start vcpkg to install the required libraries into the build folder.

3. Enter the build folder:

   ```
   cd build
   ```

4. Generate the build files:

   ```
   cmake ..
   ```

5. Build the project in Release configuration:

   ```
   cmake --build . --config Release
   ```
## Project Structure

The project is organized into several key components:

1. **Main Application** (`src/main.cpp`): The entry point of the application, which sets up and runs the menu system.

2. **Menu System** (`src/menu_system/menu_system.cpp`): Handles user interaction and directs the program flow based on user choices.

3. **Image Processing** (`src/image_processing/`):
   - `image_processing_core.cpp`: Contains core image processing functions.
   - `image_processing_threads.cpp`: Implements multi-threaded image processing tasks.

4. **Circular Buffer** (`src/CircularBuffer/`): A custom circular buffer implementation for efficient image data management.

## Features

1. **Mock Sample**: Allows processing of pre-recorded images for testing and development purposes.
2. **Live Sample**: Real-time image capture and processing with camera hardware.
3. **Hybrid Sample**: Test environment using image files to simulate camera behavior and test triggering.
4. **Convert Saved Images**: Converts binary image files to a standard format for further analysis.

## Usage

1. Run the application.
2. Choose from the following options in the menu:
   - Run Mock Sample
   - Run Live Sample
   - Run Hybrid Sample
   - Convert Saved Images
   - Exit

### Running Samples (Mock/Live/Hybrid)

1. Select the desired sample mode from the menu.
2. For Mock Sample: Enter the path to the directory containing the sample images when prompted.
3. The program will process the images, displaying results in real-time.
4. Use keyboard controls during processing:
   - ESC: Stop capture
   - Space: Pause/Resume
   - 'a': Move to older frame
   - 'd': Move to newer frame
   - 'q': Clear circularities vector
   - 's': Save current frames
   - 'b': Acquire background (when paused)
   - 'r': Start/Stop saving results

### Real-time Display Features

The application provides real-time monitoring of various metrics:

1. **Processing Statistics**
   - Processing time (average/max/min)
   - Current Camera FPS
   - Number of images in queue
   - Exposure time

2. **Analysis Information**
   - ROI size and position
   - Circularity statistics (min/max/mean)
   - Detailed rejection reasons for cells
   - Scatter plot with density visualization

3. **Image Display**
   - Original image with overlay
   - Maintains aspect ratio
   - Intermediate processing views with color-coded error states

### Converting Saved Images

1. Select "Convert Saved Images" from the menu.
2. Enter the path to the binary image file when prompted.
3. Enter the output directory for converted images.
4. The program will convert the binary images to a standard format.

## Dependencies

- vcpkg (for managing most libraries)
- EGrabber (embedded)
- Matplotplusplus (requires gnuplot)
- OpenCV
- nlohmann/json
- Google Test (for unit testing)

**Note:** Gnuplot needs to be installed separately and added to your system's PATH. After installation, restart your computer to ensure the plotting function works correctly.

## Testing

The project uses Google Test framework for unit testing. To run the tests:

1. Build the project as described above
2. From the build directory, you can run tests in two ways:

   ```bash
   # Run all tests using CTest
   ctest --output-on-failure

   # OR run the test executable directly for more detailed output
   ./tests/Debug/unit_tests.exe    # Windows
   ./tests/unit_tests             # Linux/Mac
   ```

### Adding New Tests

Tests are located in the `tests/` directory. The main test files are:
- `test_image_processing.cpp`: Tests for image processing functions
- `test_mib_grabber.cpp`: Tests for MIB grabber functionality

To add new tests:
1. Create a new test file in the `tests/` directory
2. Add the file to `tests/CMakeLists.txt`
3. Follow the Google Test framework pattern:
   ```cpp
   TEST_F(TestSuiteName, TestName) {
       // Your test code here
       EXPECT_EQ(expected, actual);
   }
   ```

## Building

1. Ensure vcpkg is installed and properly configured in CMakeUserPresets.json.
2. Run `cmake --preset=default` in the project root directory.
3. Build the project using your preferred method (e.g., Visual Studio, command-line tools).

## Configuration

The application supports hot-reloading of configuration parameters through a JSON file. When in display mode, you can modify the configuration file to tune parameters in real-time.

## Notes

- Configuration changes require re-entering the display mode to take effect
- The system supports 1 microsecond trigger pulses for external hardware synchronization
- For any issues or questions, please refer to the project documentation or contact the development team.

## Tools

### Circularity Visualizer
A tool to visualize and understand how circularity/deformability metrics change with shape deformation:

1. Run the visualizer:
   ```bash
   ./tools/circularity_visualizer/circularity_visualizer
   ```

2. Features:
   - Interactive horizontal stretching using slider
   - Real-time display of:
     - Circularity
     - Deformability
     - Area
     - Area Ratio
   - Keyboard controls:
     - 's': Save current shape (PNG) with metrics (TXT)
     - 'r' or Space: Reset to perfect circle
     - ESC: Exit

3. Output files:
   - `shape_stretch{value}.png`: Image of the shape
   - `shape_stretch{value}_metrics.txt`: Contains metrics and contour coordinates
