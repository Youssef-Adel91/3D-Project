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
#include "Experiment2.h"
#include "Experiment3.h"
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
    CreateDirectoryA("Screenshots", NULL);
    std::cout << "\n=== Capturing Screenshots ===\n";

    // 1. dq.gltf
    cgltf_data* gltf = LoadGLTFFile("Assets/dq.gltf");
    if (!gltf) {
        std::cerr << "Cannot load Assets/dq.gltf for screenshots\n";
        return;
    }
    std::vector<Mesh> meshes = LoadMeshes(gltf);
    Skeleton skeleton = LoadSkeleton(gltf);
    FreeGLTFFile(gltf);

    Texture* tex = new Texture("Assets/dq.png");
    Shader* lbsShader = new Shader("Shaders/skinned.vert", "Shaders/lit.frag");
    Shader* dqsShader = new Shader("Shaders/dualquaternion.vert", "Shaders/lit.frag");

    float aspect = (float)width / (float)height;
    mat4 projection = perspective(60.0f, aspect, 0.01f, 1000.0f);
    mat4 view = lookAt(vec3(0, 3, 14), vec3(0, 0, 0), vec3(0, 1, 0));
    Transform model(vec3(0, 0, 0), quat(), vec3(1, 1, 1));

    float angles[] = { 0.0f, 90.0f, 180.0f };
    for (float deg : angles) {
        // Twist pose: rotate second bone (Bone 0) around bar axis (local Y)
        Pose pose = skeleton.GetRestPose();
        float rad = (float)(deg * 3.14159265358979323846 / 180.0);
        Transform local = pose.GetLocalTransform(0);
        local.rotation = local.rotation * angleAxis(-rad, vec3(0, 1, 0));
        pose.SetLocalTransform(0, local);

        // Render LBS
        glViewport(0, 0, width, height);
        glClearColor(0.15f, 0.18f, 0.22f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        std::vector<mat4> pp;
        pose.GetMatrixPalette(pp);
        std::vector<mat4> ib = skeleton.GetInvBindPose();

        lbsShader->Bind();
        Uniform<mat4>::Set(lbsShader->GetUniform("model"), transformToMat4(model));
        Uniform<mat4>::Set(lbsShader->GetUniform("view"), view);
        Uniform<mat4>::Set(lbsShader->GetUniform("projection"), projection);
        Uniform<vec3>::Set(lbsShader->GetUniform("light"), vec3(1, 1, 1));
        Uniform<mat4>::Set(lbsShader->GetUniform("pose"), pp);
        Uniform<mat4>::Set(lbsShader->GetUniform("invBindPose"), ib);
        tex->Set(lbsShader->GetUniform("tex0"), 0);

        for (auto& m : meshes) {
            m.Bind(lbsShader->GetAttribute("position"), lbsShader->GetAttribute("normal"),
                   lbsShader->GetAttribute("texCoord"), lbsShader->GetAttribute("weights"),
                   lbsShader->GetAttribute("joints"));
            m.Draw();
            m.UnBind(lbsShader->GetAttribute("position"), lbsShader->GetAttribute("normal"),
                     lbsShader->GetAttribute("texCoord"), lbsShader->GetAttribute("weights"),
                     lbsShader->GetAttribute("joints"));
        }
        tex->UnSet(0);
        lbsShader->UnBind();

        SwapBuffers(hdc);
        char fname[128];
        sprintf(fname, "Screenshots/dq_twist_%d_lbs.bmp", (int)deg);
        SaveBMP(fname, width, height);

        // Render DQS
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        std::vector<DualQuaternion> ppDQ, ibDQ;
        pose.GetDualQuaternionPalette(ppDQ);
        skeleton.GetInvBindPose(ibDQ);

        dqsShader->Bind();
        Uniform<mat4>::Set(dqsShader->GetUniform("model"), transformToMat4(model));
        Uniform<mat4>::Set(dqsShader->GetUniform("view"), view);
        Uniform<mat4>::Set(dqsShader->GetUniform("projection"), projection);
        Uniform<vec3>::Set(dqsShader->GetUniform("light"), vec3(1, 1, 1));
        Uniform<DualQuaternion>::Set(dqsShader->GetUniform("pose"), ppDQ);
        Uniform<DualQuaternion>::Set(dqsShader->GetUniform("invBindPose"), ibDQ);
        tex->Set(dqsShader->GetUniform("tex0"), 0);

        for (auto& m : meshes) {
            m.Bind(dqsShader->GetAttribute("position"), dqsShader->GetAttribute("normal"),
                   dqsShader->GetAttribute("texCoord"), dqsShader->GetAttribute("weights"),
                   dqsShader->GetAttribute("joints"));
            m.Draw();
            m.UnBind(dqsShader->GetAttribute("position"), dqsShader->GetAttribute("normal"),
                     dqsShader->GetAttribute("texCoord"), dqsShader->GetAttribute("weights"),
                     dqsShader->GetAttribute("joints"));
        }
        tex->UnSet(0);
        dqsShader->UnBind();

        SwapBuffers(hdc);
        sprintf(fname, "Screenshots/dq_twist_%d_dqs.bmp", (int)deg);
        SaveBMP(fname, width, height);
    }

    delete tex;
    delete lbsShader;
    delete dqsShader;
    meshes.clear();

    // 2. Woman.gltf
    cgltf_data* womanGltf = LoadGLTFFile("Assets/Woman.gltf");
    if (womanGltf) {
        std::vector<Mesh> womanMeshes = LoadMeshes(womanGltf);
        Skeleton womanSkeleton = LoadSkeleton(womanGltf);
        std::vector<Clip> clips = LoadAnimationClips(womanGltf);
        FreeGLTFFile(womanGltf);

        Texture* womanTex = new Texture("Assets/Woman.png");
        Shader* wLBS = new Shader("Shaders/skinned.vert", "Shaders/lit.frag");
        Shader* wDQS = new Shader("Shaders/dualquaternion.vert", "Shaders/lit.frag");

        Pose wPose = womanSkeleton.GetRestPose();
        if (!clips.empty()) {
            clips[0].Sample(wPose, 0.5f);
        }

        mat4 wView = lookAt(vec3(0, 1.2f, 3.2f), vec3(0, 1.0f, 0), vec3(0, 1, 0));
        Transform wModel(vec3(0, 0, 0), quat(), vec3(1, 1, 1));

        // Woman LBS
        glClearColor(0.15f, 0.18f, 0.22f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        std::vector<mat4> wpp; wPose.GetMatrixPalette(wpp);
        std::vector<mat4> wib = womanSkeleton.GetInvBindPose();

        wLBS->Bind();
        Uniform<mat4>::Set(wLBS->GetUniform("model"), transformToMat4(wModel));
        Uniform<mat4>::Set(wLBS->GetUniform("view"), wView);
        Uniform<mat4>::Set(wLBS->GetUniform("projection"), projection);
        Uniform<vec3>::Set(wLBS->GetUniform("light"), vec3(1, 1, 1));
        Uniform<mat4>::Set(wLBS->GetUniform("pose"), wpp);
        Uniform<mat4>::Set(wLBS->GetUniform("invBindPose"), wib);
        womanTex->Set(wLBS->GetUniform("tex0"), 0);

        for (auto& m : womanMeshes) {
            m.Bind(wLBS->GetAttribute("position"), wLBS->GetAttribute("normal"),
                   wLBS->GetAttribute("texCoord"), wLBS->GetAttribute("weights"),
                   wLBS->GetAttribute("joints"));
            m.Draw();
            m.UnBind(wLBS->GetAttribute("position"), wLBS->GetAttribute("normal"),
                     wLBS->GetAttribute("texCoord"), wLBS->GetAttribute("weights"),
                     wLBS->GetAttribute("joints"));
        }
        womanTex->UnSet(0);
        wLBS->UnBind();

        SwapBuffers(hdc);
        SaveBMP("Screenshots/woman_lbs.bmp", width, height);

        // Woman DQS
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        std::vector<DualQuaternion> wppDQ, wibDQ;
        wPose.GetDualQuaternionPalette(wppDQ);
        womanSkeleton.GetInvBindPose(wibDQ);

        wDQS->Bind();
        Uniform<mat4>::Set(wDQS->GetUniform("model"), transformToMat4(wModel));
        Uniform<mat4>::Set(wDQS->GetUniform("view"), wView);
        Uniform<mat4>::Set(wDQS->GetUniform("projection"), projection);
        Uniform<vec3>::Set(wDQS->GetUniform("light"), vec3(1, 1, 1));
        Uniform<DualQuaternion>::Set(wDQS->GetUniform("pose"), wppDQ);
        Uniform<DualQuaternion>::Set(wDQS->GetUniform("invBindPose"), wibDQ);
        womanTex->Set(wDQS->GetUniform("tex0"), 0);

        for (auto& m : womanMeshes) {
            m.Bind(wDQS->GetAttribute("position"), wDQS->GetAttribute("normal"),
                   wDQS->GetAttribute("texCoord"), wDQS->GetAttribute("weights"),
                   wDQS->GetAttribute("joints"));
            m.Draw();
            m.UnBind(wDQS->GetAttribute("position"), wDQS->GetAttribute("normal"),
                     wDQS->GetAttribute("texCoord"), wDQS->GetAttribute("weights"),
                     wDQS->GetAttribute("joints"));
        }
        womanTex->UnSet(0);
        wDQS->UnBind();

        SwapBuffers(hdc);
        SaveBMP("Screenshots/woman_dqs.bmp", width, height);

        delete womanTex;
        delete wLBS;
        delete wDQS;
    }
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, PSTR szCmdLine, int iCmdShow) {
    std::string cmd = szCmdLine ? szCmdLine : "";
    bool runE1 = (cmd.find("--e1") != std::string::npos) || (cmd.find("--all") != std::string::npos);
    bool runE2 = (cmd.find("--e2") != std::string::npos) || (cmd.find("--all") != std::string::npos);
    bool runE3 = (cmd.find("--e3") != std::string::npos) || (cmd.find("--all") != std::string::npos);
    bool runScreenshots = (cmd.find("--screenshots") != std::string::npos) || (cmd.find("--all") != std::string::npos);
    bool runDeadCode = (cmd.find("--dead-code") != std::string::npos);
    bool isInteractive = (!runE1 && !runE2 && !runE3 && !runScreenshots && !runDeadCode) || (cmd.find("--interactive") != std::string::npos);

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
    int clientWidth = 800;
    int clientHeight = 600;
    RECT windowRect;
    SetRect(&windowRect, (screenWidth / 2) - (clientWidth / 2), (screenHeight / 2) - (clientHeight / 2),
            (screenWidth / 2) + (clientWidth / 2), (screenHeight / 2) + (clientHeight / 2));

    DWORD style = (WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX | WS_MAXIMIZEBOX);
    AdjustWindowRectEx(&windowRect, style, FALSE, 0);
    HWND hwnd = CreateWindowEx(0, wndclass.lpszClassName, "GPU Skinning Experiments (LBS vs DQS)",
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

    if (!isInteractive) {
        std::cout << "\n[Done] All requested benchmark runs complete. Exiting.\n";
        glBindVertexArray(0);
        glDeleteVertexArrays(1, &gVertexArrayObject);
        wglMakeCurrent(NULL, NULL);
        wglDeleteContext(hglrc);
        ReleaseDC(hwnd, hdc);
        DestroyWindow(hwnd);
        return 0;
    }

    // Interactive mode
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
            glViewport(0, 0, clientWidth, clientHeight);
            glEnable(GL_DEPTH_TEST);
            glEnable(GL_CULL_FACE);
            glPointSize(5.0f);
            glBindVertexArray(gVertexArrayObject);

            glClearColor(0.5f, 0.6f, 0.7f, 1.0f);
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);

            float aspect = (float)clientWidth / (float)clientHeight;
            gApplication->Render(aspect);
        }
        if (gApplication != 0) {
            SwapBuffers(hdc);
            if (vsynch != 0) {
                glFinish();
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
    case WM_ERASEBKGND:
        return 0;
    }
    return DefWindowProc(hwnd, iMsg, wParam, lParam);
}
