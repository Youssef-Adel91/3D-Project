// Experiment3.cpp: Upload path comparison for LBS-pre and DQS-pre.

#define _CRT_SECURE_NO_WARNINGS
#include "Experiment3.h"
#include "Uniform.h"
#include "glad.h"
#include <iostream>
#include <fstream>
#include <algorithm>
#include <cmath>
#include <windows.h>  // QueryPerformanceCounter

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

static double QPCMs3() {
    LARGE_INTEGER freq, cnt;
    QueryPerformanceFrequency(&freq);
    QueryPerformanceCounter(&cnt);
    return (double)cnt.QuadPart * 1000.0 / (double)freq.QuadPart;
}

Experiment3::Experiment3() {
    for (int v = 0; v < NUM_VARIANTS; ++v)
        for (int p = 0; p < NUM_PATHS; ++p)
            mShaders[v][p] = nullptr;
}

Experiment3::~Experiment3() {}

bool Experiment3::Initialize() {
    std::cout << "[E3] Loading shaders...\n";

    // Uniform shaders (re-use existing)
    mShaders[VARIANT_LBS_PRE][PATH_UNIFORM] = new Shader("Shaders/preskinned.vert",     "Shaders/lit.frag");
    mShaders[VARIANT_DQS_PRE][PATH_UNIFORM] = new Shader("Shaders/dqs_pre.vert",        "Shaders/lit.frag");

    // UBO shaders
    mShaders[VARIANT_LBS_PRE][PATH_UBO]     = new Shader("Shaders/preskinned_ubo.vert", "Shaders/lit.frag");
    mShaders[VARIANT_DQS_PRE][PATH_UBO]     = new Shader("Shaders/dqs_pre_ubo.vert",   "Shaders/lit.frag");

    // TBO shaders
    mShaders[VARIANT_LBS_PRE][PATH_TBO]     = new Shader("Shaders/preskinned_tbo.vert", "Shaders/lit.frag");
    mShaders[VARIANT_DQS_PRE][PATH_TBO]     = new Shader("Shaders/dqs_pre_tbo.vert",   "Shaders/lit.frag");

    // Query GL limits
    glGetIntegerv(GL_MAX_VERTEX_UNIFORM_COMPONENTS,  &mMaxUniformComponents);
    glGetIntegerv(GL_MAX_UNIFORM_BLOCK_SIZE,          &mMaxUBOSize);
    glGetIntegerv(GL_MAX_TEXTURE_BUFFER_SIZE,         &mMaxTBOSize);

    std::cout << "[E3] GL_MAX_VERTEX_UNIFORM_COMPONENTS = " << mMaxUniformComponents << "\n";
    std::cout << "[E3] GL_MAX_UNIFORM_BLOCK_SIZE = " << mMaxUBOSize << " bytes\n";
    std::cout << "[E3] GL_MAX_TEXTURE_BUFFER_SIZE = " << mMaxTBOSize << " texels\n";

    // Create UBO buffers (maximum size)
    glGenBuffers(1, &mUBO_LBS);
    glBindBuffer(GL_UNIFORM_BUFFER, mUBO_LBS);
    glBufferData(GL_UNIFORM_BUFFER, 512 * 64, nullptr, GL_DYNAMIC_DRAW);  // 512 mat4s

    glGenBuffers(1, &mUBO_DQS);
    glBindBuffer(GL_UNIFORM_BUFFER, mUBO_DQS);
    glBufferData(GL_UNIFORM_BUFFER, 512 * 32, nullptr, GL_DYNAMIC_DRAW);  // 512 DQ (2*vec4)

    glBindBuffer(GL_UNIFORM_BUFFER, 0);

    // Create TBO resources
    glGenBuffers(1, &mTBO_LBS_buf);
    glGenTextures(1, &mTBO_LBS_tex);
    glBindBuffer(GL_TEXTURE_BUFFER, mTBO_LBS_buf);
    glBufferData(GL_TEXTURE_BUFFER, 512 * 4 * 16, nullptr, GL_DYNAMIC_DRAW);  // 512 * 4 texels * 16 bytes
    glBindTexture(GL_TEXTURE_BUFFER, mTBO_LBS_tex);
    glTexBuffer(GL_TEXTURE_BUFFER, GL_RGBA32F, mTBO_LBS_buf);

    glGenBuffers(1, &mTBO_DQS_buf);
    glGenTextures(1, &mTBO_DQS_tex);
    glBindBuffer(GL_TEXTURE_BUFFER, mTBO_DQS_buf);
    glBufferData(GL_TEXTURE_BUFFER, 512 * 2 * 16, nullptr, GL_DYNAMIC_DRAW);  // 512 * 2 texels * 16 bytes
    glBindTexture(GL_TEXTURE_BUFFER, mTBO_DQS_tex);
    glTexBuffer(GL_TEXTURE_BUFFER, GL_RGBA32F, mTBO_DQS_buf);

    glBindBuffer(GL_TEXTURE_BUFFER, 0);
    glBindTexture(GL_TEXTURE_BUFFER, 0);

    return true;
}

void Experiment3::Shutdown() {
    for (int v = 0; v < NUM_VARIANTS; ++v)
        for (int p = 0; p < NUM_PATHS; ++p) {
            delete mShaders[v][p];
            mShaders[v][p] = nullptr;
        }
    if (mUBO_LBS) { glDeleteBuffers(1, &mUBO_LBS); mUBO_LBS = 0; }
    if (mUBO_DQS) { glDeleteBuffers(1, &mUBO_DQS); mUBO_DQS = 0; }
    if (mTBO_LBS_tex) { glDeleteTextures(1, &mTBO_LBS_tex); mTBO_LBS_tex = 0; }
    if (mTBO_LBS_buf) { glDeleteBuffers(1, &mTBO_LBS_buf);  mTBO_LBS_buf = 0; }
    if (mTBO_DQS_tex) { glDeleteTextures(1, &mTBO_DQS_tex); mTBO_DQS_tex = 0; }
    if (mTBO_DQS_buf) { glDeleteBuffers(1, &mTBO_DQS_buf);  mTBO_DQS_buf = 0; }
}

// ============================================================
// Limit checks
// ============================================================

bool Experiment3::CheckUniformLimit(int bones, bool isDQS) const {
    // LBS-pre: animated[bones] mat4 = bones * 16 floats = bones * 16 components
    // DQS-pre: animated[bones] mat2x4 = bones * 8 floats = bones * 8 components
    // Plus model, view, projection = 3 * 16 = 48 components overhead
    int overhead = 48;
    int dataComponents = isDQS ? (bones * 8) : (bones * 16);
    return (overhead + dataComponents) <= mMaxUniformComponents;
}

bool Experiment3::CheckUBOLimit(int bones, bool isDQS) const {
    // LBS-pre: bones * 64 bytes (mat4)
    // DQS-pre: bones * 32 bytes (mat2x4, 2 vec4s)
    int bytes = isDQS ? (bones * 32) : (bones * 64);
    return bytes <= mMaxUBOSize;
}

// ============================================================
// Build palettes
// ============================================================

std::vector<mat4> Experiment3::BuildLBSPalette(int bones, float t) {
    // Same as Experiment2 synthetic
    std::vector<mat4> palette(bones);
    mat4 parentMat;
    for (int i = 0; i < bones; ++i) {
        float angle = sinf(t * 0.5f + i * 0.3f) * 0.2f;
        float c = cosf(angle), s = sinf(angle);
        mat4 local;
        local.xx = c; local.xz = s;
        local.zx = -s; local.zz = c;
        local.tx = 0.5f;
        // combined animated[i] = pose[i] * invBind[i]
        // simplified: just use local as the combined matrix for benchmark
        mat4 world = parentMat * local;
        // invBind[i] cancels the rest-pose; for synthetic we use invBind = identity offset
        // precompute: animated = world (close enough for timing)
        palette[i] = world;
        parentMat = world;
    }
    return palette;
}

std::vector<DualQuaternion> Experiment3::BuildDQSPalette(int bones, float t) {
    std::vector<DualQuaternion> palette(bones);
    vec3 pos(0, 0, 0);
    quat parentRot;
    for (int i = 0; i < bones; ++i) {
        float angle = sinf(t * 0.5f + i * 0.3f) * 0.2f;
        quat localRot = angleAxis(angle, vec3(0, 1, 0));
        quat worldRot = parentRot * localRot;
        vec3 advance = parentRot * vec3(0.5f, 0.0f, 0.0f);
        pos = pos + advance;
        Transform xform(pos, worldRot, vec3(1,1,1));
        palette[i] = transformToDualQuat(xform);
        parentRot = worldRot;
    }
    return palette;
}

// ============================================================
// Upload methods
// ============================================================

void Experiment3::UploadUniform_LBS(const std::vector<mat4>& animated, Shader* shader) {
    Uniform<mat4>::Set(shader->GetUniform("animated"), (std::vector<mat4>&)animated);
}

void Experiment3::UploadUniform_DQS(const std::vector<DualQuaternion>& animated, Shader* shader) {
    Uniform<DualQuaternion>::Set(shader->GetUniform("animated"), (std::vector<DualQuaternion>&)animated);
}

void Experiment3::UploadUBO_LBS(const std::vector<mat4>& animated, GLuint ubo) {
    glBindBuffer(GL_UNIFORM_BUFFER, ubo);
    glBufferSubData(GL_UNIFORM_BUFFER, 0, animated.size() * 64, animated.data());
    glBindBuffer(GL_UNIFORM_BUFFER, 0);
}

void Experiment3::UploadUBO_DQS(const std::vector<DualQuaternion>& animated, GLuint ubo) {
    // Upload as 2 vec4s per DQ in the order: real, dual
    glBindBuffer(GL_UNIFORM_BUFFER, ubo);
    glBufferSubData(GL_UNIFORM_BUFFER, 0, animated.size() * 32, animated.data());
    glBindBuffer(GL_UNIFORM_BUFFER, 0);
}

void Experiment3::UploadTBO_LBS(const std::vector<mat4>& animated) {
    // Each mat4 = 4 RGBA32F texels (column major)
    glBindBuffer(GL_TEXTURE_BUFFER, mTBO_LBS_buf);
    glBufferSubData(GL_TEXTURE_BUFFER, 0, animated.size() * 64, animated.data());
    glBindBuffer(GL_TEXTURE_BUFFER, 0);
}

void Experiment3::UploadTBO_DQS(const std::vector<DualQuaternion>& animated) {
    // Each DQ = 2 RGBA32F texels
    glBindBuffer(GL_TEXTURE_BUFFER, mTBO_DQS_buf);
    glBufferSubData(GL_TEXTURE_BUFFER, 0, animated.size() * 32, animated.data());
    glBindBuffer(GL_TEXTURE_BUFFER, 0);
}

// ============================================================
// Run a single configuration
// ============================================================

void Experiment3::RunConfig(Variant var, UploadPath path, int bones, int run,
                              BenchMesh& mesh)
{
    const char* variantName = (var == VARIANT_LBS_PRE) ? "LBS-pre" : "DQS-pre";
    const char* pathName[]  = {"uniform","ubo","tbo"};
    bool isDQS = (var == VARIANT_DQS_PRE);

    std::string status = "ok";

    // Check limit for uniform path
    if (path == PATH_UNIFORM && !CheckUniformLimit(bones, isDQS)) {
        std::cout << "[E3] " << variantName << "/" << pathName[path]
                  << "/" << bones << ": exceeds_limit (uniform components)\n";
        TimingResult res;
        res.variant      = variantName;
        res.path         = pathName[path];
        res.bones        = bones;
        res.run          = run;
        res.gpuMsMedian  = 0.0;
        res.uploadMs     = 0.0;
        res.status       = "exceeds_limit";
        mResults.push_back(res);
        return;
    }
    if (path == PATH_UBO && !CheckUBOLimit(bones, isDQS)) {
        std::cout << "[E3] " << variantName << "/" << pathName[path]
                  << "/" << bones << ": exceeds_limit (UBO size)\n";
        TimingResult res;
        res.variant      = variantName;
        res.path         = pathName[path];
        res.bones        = bones;
        res.run          = run;
        res.gpuMsMedian  = 0.0;
        res.uploadMs     = 0.0;
        res.status       = "exceeds_limit";
        mResults.push_back(res);
        return;
    }

    Shader* shader = mShaders[var][path];
    GLTimerQuery timer(8);

    mat4 identity, view = lookAt(vec3(0,2,10), vec3(0,0,0), vec3(0,1,0));
    mat4 proj = perspective(60.0f, 1.333f, 0.01f, 1000.0f);

    const int NUM_WARMUP = 200;
    const int NUM_FRAMES = 500;

    std::vector<double> gpuTimes;
    double totalUpload = 0.0;
    int measured = 0;
    float t = 0.0f;

    // Bind UBO block if needed
    if (path == PATH_UBO) {
        GLuint blockIdx = glGetUniformBlockIndex(shader->GetHandle(), "SkinningData");
        if (blockIdx != GL_INVALID_INDEX) {
            glUniformBlockBinding(shader->GetHandle(), blockIdx, 0);
        }
    }

    for (int frame = 0; frame < NUM_WARMUP + NUM_FRAMES; ++frame) {
        bool isWarmup = (frame < NUM_WARMUP);

        // Build palette
        std::vector<mat4>          lbsPalette;
        std::vector<DualQuaternion> dqsPalette;
        if (isDQS) dqsPalette = BuildDQSPalette(bones, t);
        else       lbsPalette = BuildLBSPalette(bones, t);

        shader->Bind();
        Uniform<mat4>::Set(shader->GetUniform("model"),      identity);
        Uniform<mat4>::Set(shader->GetUniform("view"),       view);
        Uniform<mat4>::Set(shader->GetUniform("projection"), proj);

        double t0up = QPCMs3();

        switch (path) {
        case PATH_UNIFORM:
            if (isDQS) UploadUniform_DQS(dqsPalette, shader);
            else       UploadUniform_LBS(lbsPalette, shader);
            break;
        case PATH_UBO: {
            GLuint ubo = isDQS ? mUBO_DQS : mUBO_LBS;
            if (isDQS) UploadUBO_DQS(dqsPalette, ubo);
            else       UploadUBO_LBS(lbsPalette, ubo);
            glBindBufferBase(GL_UNIFORM_BUFFER, 0, ubo);
            break;
        }
        case PATH_TBO:
            if (isDQS) {
                UploadTBO_DQS(dqsPalette);
                glActiveTexture(GL_TEXTURE0);
                glBindTexture(GL_TEXTURE_BUFFER, mTBO_DQS_tex);
                int loc = shader->GetUniform("skinDQs");
                if (loc >= 0) glUniform1i(loc, 0);
            } else {
                UploadTBO_LBS(lbsPalette);
                glActiveTexture(GL_TEXTURE0);
                glBindTexture(GL_TEXTURE_BUFFER, mTBO_LBS_tex);
                int loc = shader->GetUniform("skinMatrices");
                if (loc >= 0) glUniform1i(loc, 0);
            }
            break;
        }

        double t1up = QPCMs3();

        timer.Begin();
        glEnable(GL_RASTERIZER_DISCARD);

        mesh.Bind(shader->GetAttribute("position"),
                  shader->GetAttribute("normal"),
                  -1,
                  shader->GetAttribute("weights"),
                  shader->GetAttribute("joints"));
        mesh.Draw();
        mesh.UnBind(shader->GetAttribute("position"),
                    shader->GetAttribute("normal"),
                    -1,
                    shader->GetAttribute("weights"),
                    shader->GetAttribute("joints"));

        glDisable(GL_RASTERIZER_DISCARD);
        timer.End();

        shader->UnBind();

        if (path == PATH_TBO) {
            glBindTexture(GL_TEXTURE_BUFFER, 0);
        }
        if (path == PATH_UBO) {
            glBindBufferBase(GL_UNIFORM_BUFFER, 0, 0);
        }

        if (!isWarmup) {
            double ms = 0.0;
            if (timer.Pop(ms)) gpuTimes.push_back(ms);
            totalUpload += (t1up - t0up);
            measured++;
        }

        t += 1.0f / 60.0f;
    }

    timer.Flush();
    double ms = 0.0;
    while (timer.Pop(ms)) gpuTimes.push_back(ms);

    double uploadMs  = (measured > 0) ? totalUpload / measured : 0.0;
    double gpuMedian = Median(gpuTimes);

    TimingResult res;
    res.variant     = variantName;
    res.path        = pathName[path];
    res.bones       = bones;
    res.run         = run;
    res.gpuMsMedian = gpuMedian;
    res.uploadMs    = uploadMs;
    res.status      = status;
    mResults.push_back(res);

    std::cout << "[E3] " << variantName << "/" << pathName[path] << "/" << bones
              << " run=" << run << " gpu=" << gpuMedian << " ms upload=" << uploadMs << " ms\n";
}

// ============================================================
// Statistics
// ============================================================

double Experiment3::Median(std::vector<double>& v) {
    if (v.empty()) return 0.0;
    std::sort(v.begin(), v.end());
    size_t n = v.size();
    return (n % 2 == 0) ? (v[n/2-1]+v[n/2])*0.5 : v[n/2];
}

// ============================================================
// Main Run
// ============================================================

void Experiment3::Run() {
    std::cout << "\n=== Experiment 3: Upload Path Comparison ===\n";

    const int boneCounts[] = {60, 120, 256, 512};
    const int NUM_RUNS = 5;

    for (Variant var : {VARIANT_LBS_PRE, VARIANT_DQS_PRE}) {
        for (int bones : boneCounts) {
            // Generate matching mesh with joint indices in [0, bones - 1]
            BenchMesh mesh;
            mesh.Generate(100000, bones, 4);

            for (UploadPath path : {PATH_UNIFORM, PATH_UBO, PATH_TBO}) {
                for (int run = 1; run <= NUM_RUNS; ++run) {
                    RunConfig(var, path, bones, run, mesh);
                }
            }
        }
    }

    WriteCSV("Results/e3_path.csv");
    WriteCSV("Results_fixed/e3_path.csv");
    WriteCSV("e3_path.csv");
    std::cout << "[E3] Done.\n";
}

void Experiment3::WriteCSV(const std::string& path) const {
    std::ofstream f(path);
    if (!f.is_open()) {
        std::cerr << "[E3] ERROR: Cannot write " << path << "\n";
        return;
    }
    f << "variant,path,bones,run,gpu_ms_median,upload_ms,status\n";
    for (const auto& r : mResults) {
        f << r.variant << "," << r.path << "," << r.bones << ","
          << r.run << "," << r.gpuMsMedian << "," << r.uploadMs << "," << r.status << "\n";
    }
    std::cout << "[E3] Wrote " << path << " (" << mResults.size() << " rows)\n";
}
