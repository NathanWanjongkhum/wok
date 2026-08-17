cmake_minimum_required(VERSION 3.16)
project(phonecam)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

find_package(OpenCV REQUIRED)
find_package(X11 REQUIRED)
find_package(PkgConfig REQUIRED)
pkg_check_modules(TESSERACT REQUIRED tesseract)
pkg_check_modules(LEPTONICA REQUIRED lept)

add_executable(phonecam
    src/main.cpp
    src/window_picker.cpp
    src/shm_capture.cpp
    src/regions.cpp
    src/ocr_engine.cpp
    src/logger.cpp
)

target_include_directories(phonecam PRIVATE
    ${X11_INCLUDE_DIR}
    ${TESSERACT_INCLUDE_DIRS}
    ${LEPTONICA_INCLUDE_DIRS}
)

target_link_libraries(phonecam PRIVATE
    ${OpenCV_LIBS}
    ${X11_LIBRARIES}
    Xext
    ${TESSERACT_LIBRARIES}
    ${LEPTONICA_LIBRARIES}
)