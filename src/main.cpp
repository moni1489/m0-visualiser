#include <GLFW/glfw3.h>
#define GLFW_EXPOSE_NATIVE_WIN32
#include <GLFW/glfw3native.h>
#include <windows.h>
#include <iostream>
using namespace std;
// Callback function to find the WorkerW window
BOOL CALLBACK EnumWindowsProc(HWND hwnd, LPARAM lParam) {
    char className[256];
    GetClassNameA(hwnd, className, sizeof(className));
    
    // We are looking for a window of class WorkerW
    if (strcmp(className, "WorkerW") == 0) {
        // The background WorkerW does NOT have a SHELLDLL_DefView child
        HWND defView = FindWindowEx(hwnd, NULL, "SHELLDLL_DefView", NULL);
        if (defView == NULL) {
            HWND* ret = (HWND*)lParam;
            *ret = hwnd;
            std::cout << "SUCCESS: Found background WorkerW (might be hidden)!" << std::endl;
            return FALSE; // Stop enumerating, we found it!
        }
    }
    return TRUE;
}

HWND GetWorkerW() {
    // Get Progman window
    HWND progman = FindWindow("Progman", NULL);
    if (!progman) {
        cerr << "Could not find Progman window." << std::endl;
        return NULL;
    }

    // Send the magic message to Progman to spawn a WorkerW window behind desktop icons
    // 0x052C is the undocumented message
    SendMessageTimeout(progman, 0x052C, 0, 0, SMTO_NORMAL, 1000, nullptr);

    // Give Windows time to create the window (important on Windows 11)
    Sleep(100);

    // Find the spawned WorkerW window, retry a few times if not found
    HWND workerw = NULL;
    for (int i = 0; i < 10; ++i) {
        EnumWindows(EnumWindowsProc, (LPARAM)&workerw);
        if (workerw != NULL) {
            return workerw;
        }
        Sleep(100);
    }

    // Windows 11 24H2 Fallback: The WorkerW hack might not work anymore.
    // Instead, we can attach our window directly to Progman and push it to the bottom.
    std::cout << "WorkerW not found, falling back to Progman (Windows 11 24H2 mode)." << std::endl;
    return progman;
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
        // Crucial for Windows 11: The spawned WorkerW might be hidden!
        // We MUST force it to be visible.
        ShowWindow(workerw, SW_SHOW);

        // Set the parent of our window to the WorkerW or Progman window
        SetParent(hwnd, workerw);
        
        // Use HWND_TOP to place it at the top of the WorkerW children
        SetWindowPos(hwnd, HWND_TOP, 0, 0, mode->width, mode->height, SWP_SHOWWINDOW);
    } else {
        cerr << "Failed to find WorkerW or Progman, window will not be attached to desktop background." << std::endl;
    }

    glfwMakeContextCurrent(window);
    glfwSwapInterval(1); // Enable vsync

    // Main loop
    while (!glfwWindowShouldClose(window)) {
        int width, height;
        glfwGetFramebufferSize(window, &width, &height);
        glViewport(0, 0, width, height);

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
