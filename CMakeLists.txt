cmake_minimum_required(VERSION 3.14)
project(LiveWallpaperAudio)

set(CMAKE_CXX_STANDARD 17)

include(FetchContent)

# Fetch GLFW
FetchContent_Declare(
    glfw
    GIT_REPOSITORY https://github.com/glfw/glfw.git
    GIT_TAG        3.4
)
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
set(GLFW_INSTALL OFF CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(glfw)

# Fetch ImGui
FetchContent_Declare(
    imgui
    GIT_REPOSITORY https://github.com/ocornut/imgui.git
    GIT_TAG        v1.91.8
    GIT_SHALLOW    TRUE
)
FetchContent_MakeAvailable(imgui)

set(IMGUI_DIR ${imgui_SOURCE_DIR})
set(IMGUI_SOURCES
    ${IMGUI_DIR}/imgui.cpp
    ${IMGUI_DIR}/imgui_draw.cpp
    ${IMGUI_DIR}/imgui_tables.cpp
    ${IMGUI_DIR}/imgui_widgets.cpp
    ${IMGUI_DIR}/backends/imgui_impl_glfw.cpp
    ${IMGUI_DIR}/backends/imgui_impl_opengl3.cpp
)

# Fetch KissFFT
FetchContent_Declare(
    kissfft
    GIT_REPOSITORY https://github.com/mborgerding/kissfft.git
    GIT_TAG        131.1.0
    GIT_SHALLOW    TRUE
)
FetchContent_MakeAvailable(kissfft)

set(KISSFFT_DIR ${kissfft_SOURCE_DIR})
set(KISSFFT_SOURCES ${KISSFFT_DIR}/kiss_fft.c)

# Download stb_image.h
set(STB_DIR ${CMAKE_BINARY_DIR}/stb)
file(MAKE_DIRECTORY ${STB_DIR})
if(NOT EXISTS ${STB_DIR}/stb_image.h)
    file(DOWNLOAD https://raw.githubusercontent.com/nothings/stb/master/stb_image.h ${STB_DIR}/stb_image.h)
endif()

add_executable(LiveWallpaper WIN32 src/main.cpp src/AudioCapture.cpp src/AudioAnalyzer.cpp src/MediaColorExtractor.cpp ${IMGUI_SOURCES} ${KISSFFT_SOURCES})

target_include_directories(LiveWallpaper PRIVATE ${IMGUI_DIR} ${IMGUI_DIR}/backends ${KISSFFT_DIR} ${STB_DIR})
target_link_libraries(LiveWallpaper PRIVATE glfw)

# Windows specific flags and libraries
if(WIN32)
    target_compile_definitions(LiveWallpaper PRIVATE NOMINMAX)
    if(MSVC)
        target_compile_options(LiveWallpaper PRIVATE /await)
    endif()
    target_link_libraries(LiveWallpaper PRIVATE user32 gdi32 shell32 opengl32 ole32 windowsapp)
endif()
