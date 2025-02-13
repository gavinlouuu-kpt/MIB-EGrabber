add_executable(circularity_visualizer
    circularity_visualizer.cpp
)

target_include_directories(circularity_visualizer
    PRIVATE
    ${CMAKE_SOURCE_DIR}/include
    ${OpenCV_INCLUDE_DIRS}
)

target_link_libraries(circularity_visualizer
    PRIVATE
    ${OpenCV_LIBS}
    image_processing
) 