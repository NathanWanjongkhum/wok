cmake_minimum_required(VERSION 3.16)
project(wok)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

find_package(OpenCV REQUIRED)
find_package(X11 REQUIRED)
find_package(PkgConfig REQUIRED)
pkg_check_modules(TESSERACT REQUIRED tesseract)
pkg_check_modules(LEPTONICA REQUIRED lept)
pkg_check_modules(XCOMPOSITE REQUIRED xcomposite)

add_executable(wok
    src/main.cpp
    src/window_picker.cpp
    src/shm_capture.cpp
    src/regions.cpp
    src/ocr_engine.cpp
    src/logger.cpp
    src/window_redirect.cpp
    src/overlay_window.cpp
)

target_include_directories(wok PRIVATE
    ${X11_INCLUDE_DIR}
    ${TESSERACT_INCLUDE_DIRS}
    ${LEPTONICA_INCLUDE_DIRS}
    ${XCOMPOSITE_INCLUDE_DIRS}
)

target_link_libraries(wok PRIVATE
    ${OpenCV_LIBS}
    ${X11_LIBRARIES}
    Xext
    ${TESSERACT_LIBRARIES}
    ${LEPTONICA_LIBRARIES}
    ${XCOMPOSITE_LIBRARIES}
)

add_executable(calibrate
    src/calibrate.cpp
    src/window_picker.cpp
    src/overlay_window.cpp
)

target_include_directories(calibrate PRIVATE ${X11_INCLUDE_DIR})
target_link_libraries(calibrate PRIVATE ${X11_LIBRARIES} Xext)