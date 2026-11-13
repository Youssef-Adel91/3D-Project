#pragma warning(disable : 28251)
#pragma warning(disable : 28159)

#define _CRT_SECURE_NO_WARNINGS
#define WIN32_LEAN_AND_MEAN
#define WIN32_EXTRA_LEAN

#include "glad.h"
#undef APIENTRY
#include <windows.h>
#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <intrin.h>

#include "Application.h"
#include "Sample.h"
#include "Experiment1.h"
#include "Experiment1b.h"
#include "Experiment2.h"
#include "Experiment3.h"
#include "Experiment4.h"
#include "Experiment5.h"
#include "Experiment6.h"
#include "Experiment7.h"
#include "Experiment8.h"
#include "GLTFLoader.h"
#include "Uniform.h"

// Force high-performance discrete GPU on hybrid laptops (NVIDIA Optimus / AMD PowerXpress)
extern "C" {
    __declspec(dllexport) DWORD NvOptimusEnablement = 0x00000001;
    __declspec(dllexport) int AmdPowerXpressRequestHighPerformance = 1;
}

#pragma comment(lib, "opengl32.lib")

#define WGL_CONTEXT_MAJOR_VERSION_ARB     0x2091
#define WGL_CONTEXT_MINOR_VERSION_ARB     0x2092
#define WGL_CONTEXT_FLAGS_ARB             0x2094
#define WGL_CONTEXT_CORE_PROFILE_BIT_ARB  0x00000001
#define WGL_CONTEXT_PROFILE_MASK_ARB      0x9126
typedef HGLRC(WINAPI* PFNWGLCREATECONTEXTATTRIBSARBPROC) (HDC, HGLRC, const int*);
typedef const char* (WINAPI* PFNWGLGETEXTENSIONSSTRINGEXTPROC) (void);
typedef BOOL(WINAPI* PFNWGLSWAPINTERVALEXTPROC) (int);
typedef int (WINAPI* PFNWGLGETSWAPINTERVALEXTPROC) (void);

Application* gApplication = 0;
GLuint gVertexArrayObject = 0;

int WINAPI WinMain(HINSTANCE, HINSTANCE, PSTR, int);
LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);

int main(int argc, const char** argv) {
    std::string cmd;
    for (int i = 1; i < argc; ++i) {
        if (i > 1) cmd += " ";
        cmd += argv[i];
    }
    return WinMain(GetModuleHandle(NULL), NULL, (PSTR)cmd.c_str(), SW_SHOWDEFAULT);
}

// System information helpers
static std::string GetCPUName() {
    int cpuInfo[4] = { -1 };
    char cpuBrand[0x40] = { 0 };
    __cpuid(cpuInfo, 0x80000000);
    unsigned int nExIds = cpuInfo[0];
    if (nExIds >= 0x80000004) {
        __cpuid((int*)(cpuBrand), 0x80000002);
        __cpuid((int*)(cpuBrand + 16), 0x80000003);
        __cpuid((int*)(cpuBrand + 32), 0x80000004);
        return std::string(cpuBrand);
    }
    return "Unknown x86 CPU";
}

static std::string GetOSVersion() {
    HKEY hKey;
    if (RegOpenKeyExA(HKEY_LOCAL_MACHINE, "SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion", 0, KEY_READ, &hKey) == ERROR_SUCCESS) {
        char prodName[256] = { 0 };
        DWORD prodSize = sizeof(prodName);
        RegQueryValueExA(hKey, "ProductName", NULL, NULL, (LPBYTE)prodName, &prodSize);

        char displayVersion[64] = { 0 };
        DWORD dispSize = sizeof(displayVersion);
        RegQueryValueExA(hKey, "DisplayVersion", NULL, NULL, (LPBYTE)displayVersion, &dispSize);

        char buildNumber[64] = { 0 };
        DWORD buildSize = sizeof(buildNumber);
        RegQueryValueExA(hKey, "CurrentBuildNumber", NULL, NULL, (LPBYTE)buildNumber, &buildSize);

        RegCloseKey(hKey);
        std::string os = prodName;
        if (displayVersion[0]) { os += " ("; os += displayVersion; os += ")"; }
        if (buildNumber[0]) { os += " Build "; os += buildNumber; }
        return os;
    }
    return "Windows (unknown version)";
}

static void WriteEnvCSV(int vsyncVal) {
    CreateDirectoryA("Results", NULL);
    CreateDirectoryA("Screenshots", NULL);

    const char* renderer = (const char*)glGetString(GL_RENDERER);
    const char* version  = (const char*)glGetString(GL_VERSION);
    const char* vendor   = (const char*)glGetString(GL_VENDOR);

    GLint maxUniformComponents = 0;
    GLint maxUBOSize = 0;
    GLint maxTBOSize = 0;
    glGetIntegerv(GL_MAX_VERTEX_UNIFORM_COMPONENTS, &maxUniformComponents);
    glGetIntegerv(GL_MAX_UNIFORM_BLOCK_SIZE,         &maxUBOSize);
    glGetIntegerv(GL_MAX_TEXTURE_BUFFER_SIZE,        &maxTBOSize);

    std::string cpu = GetCPUName();
    std::string os  = GetOSVersion();

    std::cout << "\n========================================\n";
    std::cout << "           ENVIRONMENT INFO             \n";
    std::cout << "========================================\n";
    std::cout << "GL_RENDERER:                      " << (renderer ? renderer : "Unknown") << "\n";
    std::cout << "GL_VERSION:                       " << (version ? version : "Unknown") << "\n";
    std::cout << "GL_VENDOR:                        " << (vendor ? vendor : "Unknown") << "\n";
    std::cout << "CPU:                              " << cpu << "\n";
    std::cout << "OS:                               " << os << "\n";
    std::cout << "VSync:                            " << (vsyncVal ? "ON" : "OFF") << " (" << vsyncVal << ")\n";
    std::cout << "GL_MAX_VERTEX_UNIFORM_COMPONENTS: " << maxUniformComponents << "\n";
    std::cout << "GL_MAX_UNIFORM_BLOCK_SIZE:        " << maxUBOSize << " bytes (" << (maxUBOSize / 1024) << " KB)\n";
    std::cout << "GL_MAX_TEXTURE_BUFFER_SIZE:       " << maxTBOSize << " texels\n";
    std::cout << "========================================\n\n";

    // Write to env.csv in root and Results/
    for (const std::string& envPath : { "env.csv", "Results/env.csv" }) {
        std::ofstream f(envPath);
        if (f.is_open()) {
            f << "key,value\n";
            f << "GL_RENDERER,\"" << (renderer ? renderer : "") << "\"\n";
            f << "GL_VERSION,\"" << (version ? version : "") << "\"\n";
            f << "GL_VENDOR,\"" << (vendor ? vendor : "") << "\"\n";
            f << "driver_version,\"" << (version ? version : "") << "\"\n";
            f << "CPU,\"" << cpu << "\"\n";
            f << "OS,\"" << os << "\"\n";
            f << "active_gpu,\"" << (renderer ? renderer : "") << "\"\n";
            f << "vsync," << vsyncVal << "\n";
            f << "GL_MAX_VERTEX_UNIFORM_COMPONENTS," << maxUniformComponents << "\n";
            f << "GL_MAX_UNIFORM_BLOCK_SIZE," << maxUBOSize << "\n";
            f << "GL_MAX_TEXTURE_BUFFER_SIZE," << maxTBOSize << "\n";
        }
    }
}

// 24-bit uncompressed BMP screenshot saver
static bool SaveBMP(const std::string& path, int width, int height) {
    std::vector<unsigned char> pixels(width * height * 3);
    glPixelStorei(GL_PACK_ALIGNMENT, 1);
    glReadPixels(0, 0, width, height, GL_BGR, GL_UNSIGNED_BYTE, pixels.data());

    FILE* f = fopen(path.c_str(), "wb");
    if (!f) {
        std::cerr << "Failed to open screenshot file: " << path << "\n";
        return false;
    }

    int fileSize = 54 + width * height * 3;
    unsigned char bmpFileHeader[14] = {
        'B','M',
        (unsigned char)(fileSize),
        (unsigned char)(fileSize >> 8),
        (unsigned char)(fileSize >> 16),
        (unsigned char)(fileSize >> 24),
        0, 0, 0, 0,
        54, 0, 0, 0
    };
    unsigned char bmpInfoHeader[40] = {
        40, 0, 0, 0,
        (unsigned char)(width),
        (unsigned char)(width >> 8),
        (unsigned char)(width >> 16),
        (unsigned char)(width >> 24),
        (unsigned char)(height),
        (unsigned char)(height >> 8),
        (unsigned char)(height >> 16),
        (unsigned char)(height >> 24),
        1, 0,
        24, 0,
        0, 0, 0, 0,
        0, 0, 0, 0,
        0, 0, 0, 0,
        0, 0, 0, 0,
        0, 0, 0, 0,
        0, 0, 0, 0
    };

    fwrite(bmpFileHeader, 1, 14, f);
    fwrite(bmpInfoHeader, 1, 40, f);
    fwrite(pixels.data(), 1, width * height * 3, f);
    fclose(f);
    std::cout << "[Screenshot] Saved " << path << "\n";
    return true;
}

static void CaptureScreenshots(HDC hdc, int width, int height) {
    CreateDirectoryA("Screenshots_v2", NULL);
    CreateDirectoryA("Screenshots_v2/frames", NULL);
    std::cout << "\n=== Capturing Presentable Screenshots (Screenshots_v2) ===\n";

    Sample sample;
    sample.Initialize();

    // Warm-up frame
    sample.RenderSplit(width, height);
    SwapBuffers(hdc);

    struct ShotDef {
        const char* name;
        int model;   // 0=Bar, 1=Woman
        int motion;  // 0=Twist, 1=Bend
        float param; // angle or time
    };

    ShotDef shots[] = {
        { "bar_twist_0",   0, 0, 0.0f },
        { "bar_twist_90",  0, 0, 90.0f },
        { "bar_twist_180", 0, 0, 180.0f },
        { "bar_bend_0",    0, 1, 0.0f },
        { "bar_bend_60",   0, 1, 60.0f },
        { "bar_bend_120",  0, 1, 120.0f },
        { "woman_walk",    1, 0, 0.5f }
    };

    for (const auto& s : shots) {
        if (s.model == 0) {
            if (s.motion == 0) sample.SetBarTwist(s.param);
            else sample.SetBarBend(s.param);
        } else {
            sample.SetWomanTime(s.param);
        }

        sample.RenderSplit(width, height);
        glReadBuffer(GL_BACK);
        glFinish();

        char rawPath[256];
        sprintf(rawPath, "Screenshots_v2/raw_%s.bmp", s.name);
        SaveBMP(rawPath, width, height);

        SwapBuffers(hdc);
    }

    sample.Shutdown();
    std::cout << "[Screenshots] Raw split views saved to Screenshots_v2/\n";

    // Run python post-processor to annotate badges and output PNGs
    int ret = system("\"\"C:\\Users\\DELL\\AppData\\Local\\Programs\\Python\\Python312\\python.exe\" postprocess_visuals.py --annotate\"");
    if (ret != 0) {
        system("python postprocess_visuals.py --annotate");
    }
}

static void RecordVideos(HDC hdc, int width, int height, const std::string& recordOut) {
    CreateDirectoryA("Screenshots_v2", NULL);
    CreateDirectoryA("Screenshots_v2/frames", NULL);
    std::cout << "\n=== Recording Split View Video Loops (Screenshots_v2) ===\n";

    Sample sample;
    sample.Initialize();

    // Warm-up frame
    sample.RenderSplit(width, height);
    SwapBuffers(hdc);

    const int fps = 30;
    const int durationSec = 10;
    const int totalFrames = fps * durationSec; // 300 frames

    // 1. Bar Twist loop: 0 -> 180 -> 0 (10s @ 30fps)
    std::cout << "[Record] Generating Bar Twist loop (300 frames)...\n";
    for (int i = 0; i < totalFrames; ++i) {
        float t = (float)i / (float)totalFrames;
        float angle = 180.0f * 0.5f * (1.0f - cosf(t * 2.0f * 3.14159265f));
        sample.SetBarTwist(angle);
        sample.RenderSplit(width, height);
        glReadBuffer(GL_BACK);
        glFinish();

        char fpath[256];
        sprintf(fpath, "Screenshots_v2/frames/bar_twist_%04d.bmp", i);
        SaveBMP(fpath, width, height);

        SwapBuffers(hdc);
    }

    // 2. Woman Walk loop: 10s continuous walking
    std::cout << "[Record] Generating Woman Walk loop (300 frames)...\n";
    for (int i = 0; i < totalFrames; ++i) {
        float timeSec = (float)i / (float)fps;
        sample.SetWomanTime(timeSec);
        sample.RenderSplit(width, height);
        glReadBuffer(GL_BACK);
        glFinish();

        char fpath[256];
        sprintf(fpath, "Screenshots_v2/frames/woman_walk_%04d.bmp", i);
        SaveBMP(fpath, width, height);

        SwapBuffers(hdc);
    }

    sample.Shutdown();
    std::cout << "[Record] All frames recorded to Screenshots_v2/frames/\n";

    char encodeCmd[512];
    sprintf(encodeCmd, "\"\"C:\\Users\\DELL\\AppData\\Local\\Programs\\Python\\Python312\\python.exe\" postprocess_visuals.py --encode \"%s\"\"", recordOut.c_str());
    int ret = system(encodeCmd);
    if (ret != 0) {
        sprintf(encodeCmd, "python postprocess_visuals.py --encode \"%s\"", recordOut.c_str());
        system(encodeCmd);
    }
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, PSTR szCmdLine, int iCmdShow) {
    std::string cmd = szCmdLine ? szCmdLine : "";
    bool runE1 = ((cmd.find("--e1") != std::string::npos && cmd.find("--e1b") == std::string::npos) || (cmd.find("--all") != std::string::npos));
    bool runE1b = (cmd.find("--e1b") != std::string::npos);
    bool runE2Audit = (cmd.find("--e2-audit") != std::string::npos);
    bool runE2Reversed = (cmd.find("--e2-reversed") != std::string::npos);
    bool runE2 = ((cmd.find("--e2") != std::string::npos && !runE2Audit && !runE2Reversed) || (cmd.find("--all") != std::string::npos));
    bool runE3 = (cmd.find("--e3") != std::string::npos) || (cmd.find("--all") != std::string::npos);
    bool runScreenshots = (cmd.find("--screenshots") != std::string::npos) || (cmd.find("--all") != std::string::npos);
    bool runDeadCode = (cmd.find("--dead-code") != std::string::npos);
    bool runRecord = (cmd.find("--record") != std::string::npos);
    bool runE4 = (cmd.find("--e4") != std::string::npos);
    bool runE5b = (cmd.find("--e5b") != std::string::npos);
    bool runE5Exact = (cmd.find("--e5-exact") != std::string::npos);
    bool runE5 = ((cmd.find("--e5") != std::string::npos && !runE5b && !runE5Exact) || (cmd.find("--all") != std::string::npos));
    bool runE6 = (cmd.find("--e6") != std::string::npos);
    bool runE7 = (cmd.find("--e7") != std::string::npos);
    bool runE8 = (cmd.find("--e8") != std::string::npos);
    bool runComparePaths = (cmd.find("--compare-paths") != std::string::npos);
    bool shuffleVertices = (cmd.find("--shuffle-vertices") != std::string::npos);
    bool isInteractive = (!runE1 && !runE1b && !runE2 && !runE3 && !runScreenshots && !runDeadCode && !runRecord && !runE4 && !runE5 && !runE5b && !runE5Exact && !runE6 && !runE7 && !runE8 && !runComparePaths && !runE2Audit && !runE2Reversed && !shuffleVertices) || (cmd.find("--interactive") != std::string::npos);

    std::string recordOut = "Screenshots_v2/out.mp4";
    size_t recPos = cmd.find("--record");
    if (recPos != std::string::npos) {
        std::string sub = cmd.substr(recPos + 8);
        while (!sub.empty() && sub[0] == ' ') sub.erase(0, 1);
        size_t endPos = sub.find(' ');
        std::string token = (endPos != std::string::npos) ? sub.substr(0, endPos) : sub;
        if (!token.empty() && token[0] != '-') {
            recordOut = token;
        }
    }

    WNDCLASSEX wndclass;
    wndclass.cbSize = sizeof(WNDCLASSEX);
    wndclass.style = CS_HREDRAW | CS_VREDRAW;
    wndclass.lpfnWndProc = WndProc;
    wndclass.cbClsExtra = 0;
    wndclass.cbWndExtra = 0;
    wndclass.hInstance = hInstance;
    wndclass.hIcon = LoadIcon(NULL, IDI_APPLICATION);
    wndclass.hIconSm = LoadIcon(NULL, IDI_APPLICATION);
    wndclass.hCursor = LoadCursor(NULL, IDC_ARROW);
    wndclass.hbrBackground = (HBRUSH)(COLOR_BTNFACE + 1);
    wndclass.lpszMenuName = 0;
    wndclass.lpszClassName = "Win32 Game Window";
    RegisterClassEx(&wndclass);

    int screenWidth = GetSystemMetrics(SM_CXSCREEN);
    int screenHeight = GetSystemMetrics(SM_CYSCREEN);
    int clientWidth = (screenWidth >= 1360) ? 1280 : 1024;
    int clientHeight = (screenHeight >= 768) ? 720 : 576;
    RECT windowRect;
    SetRect(&windowRect, (screenWidth / 2) - (clientWidth / 2), (screenHeight / 2) - (clientHeight / 2),
            (screenWidth / 2) + (clientWidth / 2), (screenHeight / 2) + (clientHeight / 2));

    DWORD style = (WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX | WS_MAXIMIZEBOX);
    AdjustWindowRectEx(&windowRect, style, FALSE, 0);
    HWND hwnd = CreateWindowEx(0, wndclass.lpszClassName, "GPU Skinning: LBS vs DQS Split View",
                               style, windowRect.left, windowRect.top,
                               windowRect.right - windowRect.left, windowRect.bottom - windowRect.top,
                               NULL, NULL, hInstance, szCmdLine);
    HDC hdc = GetDC(hwnd);

    PIXELFORMATDESCRIPTOR pfd;
    memset(&pfd, 0, sizeof(PIXELFORMATDESCRIPTOR));
    pfd.nSize = sizeof(PIXELFORMATDESCRIPTOR);
    pfd.nVersion = 1;
    pfd.dwFlags = PFD_SUPPORT_OPENGL | PFD_DRAW_TO_WINDOW | PFD_DOUBLEBUFFER;
    pfd.iPixelType = PFD_TYPE_RGBA;
    pfd.cColorBits = 24;
    pfd.cDepthBits = 32;
    pfd.cStencilBits = 8;
    pfd.iLayerType = PFD_MAIN_PLANE;
    int pixelFormat = ChoosePixelFormat(hdc, &pfd);
    SetPixelFormat(hdc, pixelFormat, &pfd);

    HGLRC tempRC = wglCreateContext(hdc);
    wglMakeCurrent(hdc, tempRC);
    PFNWGLCREATECONTEXTATTRIBSARBPROC wglCreateContextAttribsARB =
        (PFNWGLCREATECONTEXTATTRIBSARBPROC)wglGetProcAddress("wglCreateContextAttribsARB");

    const int attribList[] = {
        WGL_CONTEXT_MAJOR_VERSION_ARB, 3,
        WGL_CONTEXT_MINOR_VERSION_ARB, 3,
        WGL_CONTEXT_FLAGS_ARB, 0,
        WGL_CONTEXT_PROFILE_MASK_ARB, WGL_CONTEXT_CORE_PROFILE_BIT_ARB,
        0,
    };

    HGLRC hglrc = wglCreateContextAttribsARB(hdc, 0, attribList);
    wglMakeCurrent(NULL, NULL);
    wglDeleteContext(tempRC);
    wglMakeCurrent(hdc, hglrc);

    if (!gladLoadGL()) {
        std::cerr << "Could not initialize GLAD\n";
        return -1;
    }

    // Disable VSync explicitly
    int vsynch = 0;
    PFNWGLGETEXTENSIONSSTRINGEXTPROC _wglGetExtensionsStringEXT =
        (PFNWGLGETEXTENSIONSSTRINGEXTPROC)wglGetProcAddress("wglGetExtensionsStringEXT");
    if (_wglGetExtensionsStringEXT && strstr(_wglGetExtensionsStringEXT(), "WGL_EXT_swap_control") != 0) {
        PFNWGLSWAPINTERVALEXTPROC wglSwapIntervalEXT =
            (PFNWGLSWAPINTERVALEXTPROC)wglGetProcAddress("wglSwapIntervalEXT");
        PFNWGLGETSWAPINTERVALEXTPROC wglGetSwapIntervalEXT =
            (PFNWGLGETSWAPINTERVALEXTPROC)wglGetProcAddress("wglGetSwapIntervalEXT");

        if (wglSwapIntervalEXT) {
            wglSwapIntervalEXT(0); // VSync OFF
            if (wglGetSwapIntervalEXT) {
                vsynch = wglGetSwapIntervalEXT();
            }
        }
    }

    glGenVertexArrays(1, &gVertexArrayObject);
    glBindVertexArray(gVertexArrayObject);

    ShowWindow(hwnd, SW_SHOW);
    UpdateWindow(hwnd);
    DisableProcessWindowsGhosting();

    // Write hardware and environment settings
    WriteEnvCSV(vsynch);

    // Run automated experiments if requested
    if (runE1) {
        Experiment1 e1;
        if (e1.Initialize()) {
            e1.Run();
            e1.Shutdown();
        }
    }

    if (runE1b) {
        Experiment1b e1b;
        if (e1b.Initialize()) {
            e1b.Run();
            e1b.Shutdown();
        }
    }

    if (runE2) {
        Experiment2 e2;
        if (e2.Initialize()) {
            e2.Run();
            e2.Shutdown();
        }
    }

    if (runE3) {
        Experiment3 e3;
        if (e3.Initialize()) {
            e3.Run();
            e3.Shutdown();
        }
    }

    if (runDeadCode) {
        Experiment2 e2;
        if (e2.Initialize()) {
            e2.RunDeadCodeBenchmark();
            e2.Shutdown();
        }
    }

    if (runScreenshots) {
        CaptureScreenshots(hdc, clientWidth, clientHeight);
    }

    if (runRecord) {
        RecordVideos(hdc, clientWidth, clientHeight, recordOut);
    }

    if (runE4) {
        Experiment4 e4;
        if (e4.Initialize()) {
            e4.Run();
            e4.Shutdown();
        }
    }

    if (runComparePaths) {
        Experiment4 e4;
        if (e4.Initialize()) {
            e4.RunComparePaths();
            e4.Shutdown();
        }
    }

    if (runE2Audit) {
        Experiment4 e4;
        if (e4.Initialize()) {
            e4.RunE2SweepAudit(false);
            e4.Shutdown();
        }
    }

    if (runE2Reversed) {
        Experiment4 e4;
        if (e4.Initialize()) {
            e4.RunE2SweepAudit(true);
            e4.Shutdown();
        }
    }

    if (runE5 || shuffleVertices) {
        Experiment5 e5;
        if (e5.Initialize()) {
            e5.Run(shuffleVertices);
            e5.Shutdown();
        }
    }

    if (runE5b) {
        Experiment5 e5;
        if (e5.Initialize()) {
            e5.RunE5bChecks();
            e5.Shutdown();
        }
    }

    if (runE5Exact) {
        Experiment5 e5;
        if (e5.Initialize()) {
            e5.RunE5Exact();
            e5.Shutdown();
        }
    }

    if (runE6) {
        Experiment6 e6;
        if (e6.Initialize()) {
            e6.Run();
            e6.Shutdown();
        }
    }

    if (runE7) {
        Experiment7 e7;
        if (e7.Initialize()) {
            e7.Run();
            e7.Shutdown();
        }
    }

    if (runE8) {
        Experiment8 e8;
        if (e8.Initialize()) {
            e8.Run();
            e8.Shutdown();
        }
    }

    if (!isInteractive) {
        std::cout << "\n[Done] All requested tasks complete. Exiting.\n";
        glBindVertexArray(0);
        glDeleteVertexArrays(1, &gVertexArrayObject);
        wglMakeCurrent(NULL, NULL);
        wglDeleteContext(hglrc);
        ReleaseDC(hwnd, hdc);
        DestroyWindow(hwnd);
        return 0;
    }

    // Interactive mode
    std::cout << "\n=======================================================\n";
    std::cout << "        GPU SKINNING DEMO (LBS vs DQS SPLIT VIEW)       \n";
    std::cout << "=======================================================\n";
    std::cout << "Interactive Controls:\n";
    std::cout << "  [Space]       Pause / Play animation\n";
    std::cout << "  [Left/Right]  Decrease / Increase twist or bend angle\n";
    std::cout << "  [B]           Toggle model: Cylinder Bar <-> Woman character\n";
    std::cout << "  [T]           Toggle motion: Twist <-> Bend (Bar model)\n";
    std::cout << "  [Esc]         Quit\n";
    std::cout << "=======================================================\n\n";

    gApplication = new Sample();
    gApplication->Initialize();

    DWORD lastTick = GetTickCount();
    MSG msg;
    while (true) {
        if (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) {
            if (msg.message == WM_QUIT) {
                break;
            }
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
        DWORD thisTick = GetTickCount();
        float deltaTime = float(thisTick - lastTick) * 0.001f;
        lastTick = thisTick;
        if (gApplication != 0) {
            gApplication->Update(deltaTime);
        }
        if (gApplication != 0) {
            RECT clientRect;
            GetClientRect(hwnd, &clientRect);
            clientWidth = clientRect.right - clientRect.left;
            clientHeight = clientRect.bottom - clientRect.top;
            glEnable(GL_DEPTH_TEST);
            glEnable(GL_CULL_FACE);
            glBindVertexArray(gVertexArrayObject);

            if (Sample* s = dynamic_cast<Sample*>(gApplication)) {
                s->RenderSplit(clientWidth, clientHeight);
            } else {
                glViewport(0, 0, clientWidth, clientHeight);
                glClearColor(0.5f, 0.6f, 0.7f, 1.0f);
                glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
                float aspect = (float)clientWidth / (float)clientHeight;
                gApplication->Render(aspect);
            }
        }
        if (gApplication != 0) {
            SwapBuffers(hdc);
            if (vsynch != 0) {
                glFinish();
            }
        }

        static int sFrameCount = 0;
        static DWORD sFpsLastTick = GetTickCount();
        static float sCurrentFps = 60.0f;
        sFrameCount++;
        if (thisTick - sFpsLastTick >= 250) {
            sCurrentFps = (float)sFrameCount / ((float)(thisTick - sFpsLastTick) * 0.001f);
            sFrameCount = 0;
            sFpsLastTick = thisTick;
            if (Sample* s = dynamic_cast<Sample*>(gApplication)) {
                std::string title = s->GetWindowTitle(sCurrentFps);
                SetWindowTextA(hwnd, title.c_str());
            }
        }
    }

    if (gApplication != 0) {
        gApplication->Shutdown();
        delete gApplication;
        gApplication = 0;
    }

    return (int)msg.wParam;
}

LRESULT CALLBACK WndProc(HWND hwnd, UINT iMsg, WPARAM wParam, LPARAM lParam) {
    switch (iMsg) {
    case WM_KEYDOWN:
        if (wParam == VK_ESCAPE) {
            PostQuitMessage(0);
            return 0;
        }
        if (gApplication != 0) {
            gApplication->OnKeyDown((int)wParam);
        }
        return 0;
    case WM_CLOSE:
        if (gApplication != 0) {
            gApplication->Shutdown();
            gApplication = 0;
            DestroyWindow(hwnd);
        }
        break;
    case WM_DESTROY:
        if (gVertexArrayObject != 0) {
            HDC hdc = GetDC(hwnd);
            HGLRC hglrc = wglGetCurrentContext();

            glBindVertexArray(0);
            glDeleteVertexArrays(1, &gVertexArrayObject);
            gVertexArrayObject = 0;

            wglMakeCurrent(NULL, NULL);
            wglDeleteContext(hglrc);
            ReleaseDC(hwnd, hdc);

            PostQuitMessage(0);
        }
        break;
    case WM_PAINT:
        ValidateRect(hwnd, NULL);
        return 0;
    case WM_ERASEBKGND:
        return 0;
    }
    return DefWindowProc(hwnd, iMsg, wParam, lParam);
}
