cmake_minimum_required(VERSION 3.16)
project(RobotDetectionByWebcam LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)

find_package(Qt6 REQUIRED COMPONENTS Widgets Multimedia)
find_package(OpenCV REQUIRED COMPONENTS core imgproc calib3d objdetect)

add_executable(RobotDetectionByWebcam
    src/main.cpp
    src/mainwindow.h
    src/mainwindow.cpp
    src/videowidget.h
    src/videowidget.cpp
    src/squaredetector.h
    src/squaredetector.cpp
    src/markerdetector.h
    src/markerdetector.cpp
    src/cameracalibration.h
    src/cameracalibration.cpp
)

target_link_libraries(RobotDetectionByWebcam PRIVATE Qt6::Widgets Qt6::Multimedia ${OpenCV_LIBS})
