#include <GLFW/glfw3.h>
#define GLFW_EXPOSE_NATIVE_WIN32
#include <GLFW/glfw3native.h>
#include <windows.h>
#include <iostream>

// Callback function to find the WorkerW window
BOOL CALLBACK EnumWindowsProc(HWND hwnd, LPARAM lParam) {
    HWND p = FindWindowEx(hwnd, NULL, "SHELLDLL_DefView", NULL);
    if (p != NULL) {
        // Find the next WorkerW window which is our target
        HWND* ret = (HWND*)lParam;
        *ret = FindWindowEx(NULL, hwnd, "WorkerW", NULL);
    }
    return TRUE;
}

HWND GetWorkerW() {
    // Get Progman window
    HWND progman = FindWindow("Progman", NULL);
    if (!progman) {
        std::cerr << "Could not find Progman window." << std::endl;
        return NULL;
    }

    // Send the magic message to Progman to spawn a WorkerW window behind desktop icons
    // 0x052C is the undocumented message
    SendMessageTimeout(progman, 0x052C, 0, 0, SMTO_NORMAL, 1000, nullptr);

    // Find the spawned WorkerW window
    HWND workerw = NULL;
    EnumWindows(EnumWindowsProc, (LPARAM)&workerw);

    return workerw;
}

int main() {
    if (!glfwInit()) {
        std::cerr << "Failed to initialize GLFW" << std::endl;
        return -1;
    }

    // Attempt to get monitor resolution to size window correctly
    GLFWmonitor* primaryMonitor = glfwGetPrimaryMonitor();
    const GLFWvidmode* mode = glfwGetVideoMode(primaryMonitor);

    // We want a window without borders
    glfwWindowHint(GLFW_DECORATED, GLFW_FALSE);
    
    // Create the window
    GLFWwindow* window = glfwCreateWindow(mode->width, mode->height, "Live Wallpaper", NULL, NULL);
    if (!window) {
        std::cerr << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return -1;
    }

    HWND hwnd = glfwGetWin32Window(window);
    HWND workerw = GetWorkerW();

    if (workerw != NULL) {
        // Set the parent of our window to the WorkerW window
        SetParent(hwnd, workerw);
        
        // Ensure our window covers the whole screen
        SetWindowPos(hwnd, HWND_TOP, 0, 0, mode->width, mode->height, SWP_SHOWWINDOW);
    } else {
        std::cerr << "Failed to find WorkerW, window will not be attached to desktop background." << std::endl;
    }

    glfwMakeContextCurrent(window);
    glfwSwapInterval(1); // Enable vsync

    // Main loop
    while (!glfwWindowShouldClose(window)) {
        // Render simple background color for testing (dark gray)
        glClearColor(0.15f, 0.15f, 0.15f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    glfwDestroyWindow(window);
    glfwTerminate();
    return 0;
}
