cmake_minimum_required(VERSION 3.10)
project(MIB_Studio)

include(CTest)
enable_testing()

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Matplot++ CONFIG REQUIRED)
find_package(ftxui CONFIG REQUIRED)
find_package(nlohmann_json CONFIG REQUIRED)
find_package(GTest CONFIG REQUIRED)

# Set OpenCV_DIR to the correct location
set(OpenCV_DIR "${VCPKG_INSTALLED_DIR}/${VCPKG_TARGET_TRIPLET}/share/opencv4")
find_package(OpenCV REQUIRED)

# List all your source files here
set(SOURCES
    src/main.cpp
    src/image_processing/image_processing_core.cpp
    src/image_processing/image_processing_utils.cpp
    src/image_processing/image_processing_threads.cpp
    src/menu_system/menu_system.cpp
    src/CircularBuffer/CircularBuffer.cpp
    src/mib_grabber/mib_grabber.cpp
    # Add other source files here
)

# Add main executable
add_executable(${PROJECT_NAME} ${SOURCES})

# Include directories
target_include_directories(${PROJECT_NAME} PRIVATE 
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${OpenCV_INCLUDE_DIRS}
)
# Separately include the EGrabber folder for the nested files
include_directories(${PROJECT_SOURCE_DIR}/include/EGrabber) # does not work with camera only for testing
# include_directories("C:/Program Files/Euresys/eGrabber/include") # required for camera
# Link libraries
target_link_libraries(${PROJECT_NAME} PRIVATE 
    Matplot++::matplot
    ftxui::screen
    ftxui::dom
    ftxui::component
    nlohmann_json::nlohmann_json
    ${OpenCV_LIBS}
)

# Create libraries for testing
add_library(image_processing
    src/image_processing/image_processing_core.cpp
    src/image_processing/image_processing_utils.cpp
    src/image_processing/image_processing_threads.cpp
)

add_library(mib_grabber
    src/mib_grabber/mib_grabber.cpp
)

# Set include directories for libraries
target_include_directories(image_processing
    PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${OpenCV_INCLUDE_DIRS}
)

target_include_directories(mib_grabber
    PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${OpenCV_INCLUDE_DIRS}
)

# Link dependencies to libraries
target_link_libraries(image_processing
    PUBLIC
    ${OpenCV_LIBS}
    Matplot++::matplot
    nlohmann_json::nlohmann_json
)

target_link_libraries(mib_grabber
    PUBLIC
    ${OpenCV_LIBS}
    nlohmann_json::nlohmann_json
)

# Build test executables
file(GLOB TEST_SOURCES "src/tests/*.cpp")
foreach(test_source ${TEST_SOURCES})
    get_filename_component(test_name ${test_source} NAME_WE)
    add_executable(${test_name} ${test_source}
        src/image_processing/image_processing_core.cpp
        src/image_processing/image_processing_utils.cpp
        src/image_processing/image_processing_threads.cpp
        src/menu_system/menu_system.cpp
        src/CircularBuffer/CircularBuffer.cpp
        src/mib_grabber/mib_grabber.cpp

    )
    target_include_directories(${test_name} PRIVATE 
        ${CMAKE_CURRENT_SOURCE_DIR}/include
        ${CMAKE_CURRENT_SOURCE_DIR}/src
        ${CMAKE_CURRENT_SOURCE_DIR}/include/EGrabber
        # "C:/Program Files/Euresys/eGrabber/include"
        ${OpenCV_INCLUDE_DIRS}
    )
    target_link_libraries(${test_name} PRIVATE 
        Matplot++::matplot
        ftxui::screen
        ftxui::dom
        ftxui::component
        nlohmann_json::nlohmann_json
        ${OpenCV_LIBS}
    )
endforeach()

# Add testing directory
add_subdirectory(tests)

# Add tools directory
add_subdirectory(tools)
