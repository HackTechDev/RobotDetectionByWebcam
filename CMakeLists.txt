cmake_minimum_required(VERSION 3.16)
project(RobotDetectionByWebcam LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)

find_package(Qt6 REQUIRED COMPONENTS Widgets Multimedia Network Concurrent)
find_package(OpenCV REQUIRED COMPONENTS core imgproc calib3d objdetect video tracking)

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
    src/positionsender.h
    src/positionsender.cpp
    src/floormapper.h
    src/floormapper.cpp
    src/camerasettingsdialog.h
    src/camerasettingsdialog.cpp
    src/v4l2controls.h
    src/v4l2controls.cpp
    src/trackermodels.h
    src/trackermodels.cpp
    src/zonemanager.h
    src/zonemanager.cpp
    src/playerwindow.h
    src/playerwindow.cpp
    src/robotzonegame.h
    src/robotzonegame.cpp
    src/soleilgame.h
    src/soleilgame.cpp
    src/treasuregame.h
    src/treasuregame.cpp
    src/gameframework.h
    src/gameframework.cpp
    src/racegame.h
    src/racegame.cpp
    src/parkinggame.h
    src/parkinggame.cpp
    src/objecttracker.h
    src/objecttracker.cpp
    src/motionfilter.h
    src/motionfilter.cpp
)

target_link_libraries(RobotDetectionByWebcam PRIVATE Qt6::Widgets Qt6::Multimedia Qt6::Network Qt6::Concurrent ${OpenCV_LIBS})
