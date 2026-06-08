cmake_minimum_required(VERSION 3.14)
project(LiveWallpaperAudio)

set(CMAKE_CXX_STANDARD 17)

# Fetch GLFW
include(FetchContent)
FetchContent_Declare(
    glfw
    GIT_REPOSITORY https://github.com/glfw/glfw.git
    GIT_TAG        3.4
)
# Disable building GLFW tests and examples
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
set(GLFW_INSTALL OFF CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(glfw)

add_executable(LiveWallpaper src/main.cpp)

target_link_libraries(LiveWallpaper PRIVATE glfw)

# Windows specific flags and libraries
if(WIN32)
    target_link_libraries(LiveWallpaper PRIVATE user32 gdi32 shell32 opengl32)
endif()
