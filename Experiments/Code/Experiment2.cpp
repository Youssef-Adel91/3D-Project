// Experiment2.cpp: GPU timing of four skinning variants.

#define _CRT_SECURE_NO_WARNINGS
#include "Experiment2.h"
#include "Uniform.h"
#include "GLTFLoader.h"
#include "glad.h"
#include <iostream>
#include <fstream>
#include <algorithm>
#include <numeric>
#include <random>
#include <cmath>
#include <windows.h>  // QueryPerformanceCounter

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

// ============================================================
// Construction / Destruction
// ============================================================

Experiment2::Experiment2() {
    for (int i = 0; i < NUM_VARIANTS; ++i) mShaders[i] = nullptr;
}

Experiment2::~Experiment2() {}

bool Experiment2::Initialize() {
    std::cout << "[E2] Loading shaders...\n";
    mShaders[LBS_NAIVE] = new Shader("Shaders/skinned.vert",          "Shaders/lit.frag");
    mShaders[LBS_PRE]   = new Shader("Shaders/preskinned.vert",       "Shaders/lit.frag");
    mShaders[DQS_NAIVE] = new Shader("Shaders/dualquaternion.vert",   "Shaders/lit.frag");
    mShaders[DQS_PRE]   = new Shader("Shaders/dqs_pre.vert",          "Shaders/lit.frag");

    std::cout << "[E2] Loading Woman.gltf...\n";
    cgltf_data* gltf = LoadGLTFFile("Assets/Woman.gltf");
    if (!gltf) {
        std::cerr << "[E2] WARNING: Could not load Woman.gltf - skipping woman benchmark\n";
    } else {
        mWomanMeshes   = LoadMeshes(gltf);
        mWomanSkeleton = LoadSkeleton(gltf);
        mWomanPose     = mWomanSkeleton.GetRestPose();
        FreeGLTFFile(gltf);
        std::cout << "[E2] Woman: " << mWomanMeshes.size() << " meshes, "
                  << mWomanSkeleton.GetRestPose().Size() << " bones\n";
    }

    return true;
}

void Experiment2::Shutdown() {
    for (int i = 0; i < NUM_VARIANTS; ++i) {
        delete mShaders[i];
        mShaders[i] = nullptr;
    }
    mWomanMeshes.clear();
}

// ============================================================
// Synthetic palette generation
// ============================================================

// Fixed time step: animate with t = frame * (1/60)
static const float FIXED_DT = 1.0f / 60.0f;

std::vector<mat4> Experiment2::MakeSyntheticLBSPalette(int bones, float t) {
    // Simple chain: each bone rotates slightly about Y axis.
    std::vector<mat4> palette(bones);
    mat4 parentMat;  // identity
    for (int i = 0; i < bones; ++i) {
        float angle = sinf(t * 0.5f + i * 0.3f) * 0.2f;
        // Local transform: rotate Y + translate along X by bone_length=0.5
        float c = cosf(angle), s = sinf(angle);
        mat4 local;
        local.xx = c;  local.xz = s;
        local.zx = -s; local.zz = c;
        local.tx = 0.5f;  // bone length
        palette[i] = parentMat * local;
        parentMat = palette[i];
    }
    return palette;
}

std::vector<DualQuaternion> Experiment2::MakeSyntheticDQSPalette(int bones, float t) {
    std::vector<DualQuaternion> palette(bones);
    vec3 pos(0, 0, 0);
    quat parentRot;  // identity

    for (int i = 0; i < bones; ++i) {
        float angle = sinf(t * 0.5f + i * 0.3f) * 0.2f;
        quat localRot = angleAxis(angle, vec3(0, 1, 0));
        quat worldRot = parentRot * localRot;

        // Translate along bone direction
        vec3 advance = parentRot * vec3(0.5f, 0.0f, 0.0f);
        pos = pos + advance;

        // Build dual quaternion for this world transform
        Transform xform(pos, worldRot, vec3(1,1,1));
        palette[i] = transformToDualQuat(xform);

        parentRot = worldRot;
    }
    return palette;
}

std::vector<mat4> Experiment2::MakeSyntheticInvBind(int bones) {
    // Rest pose: each bone just translates along X by 0.5 per bone.
    std::vector<mat4> invBind(bones);
    for (int i = 0; i < bones; ++i) {
        mat4 bind;
        bind.tx = -0.5f * (i + 1);  // inverse of translate(0.5*(i+1), 0, 0)
        invBind[i] = bind;
    }
    return invBind;
}

std::vector<DualQuaternion> Experiment2::MakeSyntheticInvBindDQ(int bones) {
    std::vector<DualQuaternion> invBind(bones);
    for (int i = 0; i < bones; ++i) {
        vec3 pos(-0.5f * (i+1), 0, 0);
        Transform xform(pos, quat(), vec3(1,1,1));
        invBind[i] = conjugate(transformToDualQuat(xform));
    }
    return invBind;
}

// ============================================================
// Build animated palettes (LBS-pre, DQS-pre)
// ============================================================

void Experiment2::BuildAnimatedLBS(const std::vector<mat4>& posePalette,
                                     const std::vector<mat4>& invBind,
                                     std::vector<mat4>& animated) {
    animated.resize(posePalette.size());
    for (size_t i = 0; i < posePalette.size(); ++i) {
        animated[i] = posePalette[i] * invBind[i];
    }
}

void Experiment2::BuildAnimatedDQS(const std::vector<DualQuaternion>& poseDQ,
                                     const std::vector<DualQuaternion>& invBindDQ,
                                     std::vector<DualQuaternion>& animated) {
    animated.resize(poseDQ.size());
    for (size_t i = 0; i < poseDQ.size(); ++i) {
        DualQuaternion l = normalized(invBindDQ[i]);
        DualQuaternion r = normalized(poseDQ[i]);
        animated[i] = DualQuaternion(l.real * r.real, l.real * r.dual + l.dual * r.real);
    }
}

// ============================================================
// QPC helper for CPU timing
// ============================================================

static double QPCMs() {
    LARGE_INTEGER freq, cnt;
    QueryPerformanceFrequency(&freq);
    QueryPerformanceCounter(&cnt);
    return (double)cnt.QuadPart * 1000.0 / (double)freq.QuadPart;
}

// ============================================================
// Run a single variant for numWarmup + numFrames frames
// ============================================================

void Experiment2::RunVariant(Variant v, BenchMesh& mesh, int bones, int influences,
                               int numWarmup, int numFrames,
                               bool rastDiscard, bool /*useWoman*/,
                               std::vector<double>& gpuTimes,
                               double& cpuPaletteMs, double& uploadMs)
{
    Shader* shader = mShaders[v];
    GLTimerQuery timer(8);

    // Fixed time seed for reproducible poses
    float t = 0.0f;

    // Precompute inv-bind for synthetic mesh (once)
    std::vector<mat4>          invBind   = MakeSyntheticInvBind(bones);
    std::vector<DualQuaternion> invBindDQ = MakeSyntheticInvBindDQ(bones);

    // Uniforms (dummy view/proj)
    mat4 identity, view = lookAt(vec3(0,2,10), vec3(0,0,0), vec3(0,1,0));
    mat4 proj = perspective(60.0f, 1.333f, 0.01f, 1000.0f);

    double totalCPU = 0.0, totalUpload = 0.0;
    int measured = 0;

    int totalFrames = numWarmup + numFrames;
    for (int frame = 0; frame < totalFrames; ++frame) {
        bool isWarmup = (frame < numWarmup);

        // --- CPU palette computation ---
        double t0cpu = QPCMs();

        std::vector<mat4>          posePalette, animatedLBS;
        std::vector<DualQuaternion> poseDQ, invBindDQ2, animatedDQS;

        switch (v) {
        case LBS_NAIVE:
            posePalette = MakeSyntheticLBSPalette(bones, t);
            break;
        case LBS_PRE:
            posePalette = MakeSyntheticLBSPalette(bones, t);
            BuildAnimatedLBS(posePalette, invBind, animatedLBS);
            break;
        case DQS_NAIVE:
            poseDQ    = MakeSyntheticDQSPalette(bones, t);
            invBindDQ2 = invBindDQ;
            break;
        case DQS_PRE:
            poseDQ    = MakeSyntheticDQSPalette(bones, t);
            BuildAnimatedDQS(poseDQ, invBindDQ, animatedDQS);
            break;
        default: break;
        }

        double t1cpu = QPCMs();

        // --- Upload ---
        double t0up = QPCMs();
        shader->Bind();
        Uniform<mat4>::Set(shader->GetUniform("model"),      identity);
        Uniform<mat4>::Set(shader->GetUniform("view"),       view);
        Uniform<mat4>::Set(shader->GetUniform("projection"), proj);

        switch (v) {
        case LBS_NAIVE:
            Uniform<mat4>::Set(shader->GetUniform("pose"),       posePalette);
            Uniform<mat4>::Set(shader->GetUniform("invBindPose"), invBind);
            break;
        case LBS_PRE:
            Uniform<mat4>::Set(shader->GetUniform("animated"), animatedLBS);
            break;
        case DQS_NAIVE:
            Uniform<DualQuaternion>::Set(shader->GetUniform("pose"),       poseDQ);
            Uniform<DualQuaternion>::Set(shader->GetUniform("invBindPose"), invBindDQ2);
            break;
        case DQS_PRE:
            Uniform<DualQuaternion>::Set(shader->GetUniform("animated"), animatedDQS);
            break;
        }
        double t1up = QPCMs();

        // --- GPU Draw ---
        timer.Begin();
        if (rastDiscard) glEnable(GL_RASTERIZER_DISCARD);

        mesh.Bind(shader->GetAttribute("position"),
                  shader->GetAttribute("normal"),
                  -1,  // texCoord not needed
                  shader->GetAttribute("weights"),
                  shader->GetAttribute("joints"));
        mesh.Draw();
        mesh.UnBind(shader->GetAttribute("position"),
                    shader->GetAttribute("normal"),
                    -1,
                    shader->GetAttribute("weights"),
                    shader->GetAttribute("joints"));

        if (rastDiscard) glDisable(GL_RASTERIZER_DISCARD);
        timer.End();

        shader->UnBind();

        if (!isWarmup) {
            double ms = 0.0;
            if (timer.Pop(ms)) gpuTimes.push_back(ms);
            totalCPU    += (t1cpu - t0cpu);
            totalUpload += (t1up  - t0up);
            measured++;
        }

        t += FIXED_DT;
    }

    // Drain remaining queries
    timer.Flush();
    double ms = 0.0;
    while (timer.Pop(ms)) gpuTimes.push_back(ms);

    cpuPaletteMs = (measured > 0) ? totalCPU    / measured : 0.0;
    uploadMs     = (measured > 0) ? totalUpload / measured : 0.0;
}

void Experiment2::RunVariantWoman(Variant v, int numWarmup, int numFrames, bool rastDiscard,
                                    std::vector<double>& gpuTimes,
                                    double& cpuPaletteMs, double& uploadMs)
{
    if (mWomanMeshes.empty()) return;

    Shader* shader = mShaders[v];
    GLTimerQuery timer(8);

    Pose pose = mWomanSkeleton.GetRestPose();
    std::vector<mat4>          invBind   = mWomanSkeleton.GetInvBindPose();
    std::vector<DualQuaternion> invBindDQ;
    mWomanSkeleton.GetInvBindPose(invBindDQ);

    mat4 identity, view = lookAt(vec3(0,2,4), vec3(0,1,0), vec3(0,1,0));
    mat4 proj = perspective(60.0f, 1.333f, 0.01f, 1000.0f);

    double totalCPU = 0.0, totalUpload = 0.0;
    int measured = 0;
    float t = 0.0f;

    for (int frame = 0; frame < numWarmup + numFrames; ++frame) {
        bool isWarmup = (frame < numWarmup);

        double t0cpu = QPCMs();

        std::vector<mat4>          posePalette, animatedLBS;
        std::vector<DualQuaternion> poseDQ, animatedDQS;

        switch (v) {
        case LBS_NAIVE:
            pose.GetMatrixPalette(posePalette);
            break;
        case LBS_PRE:
            pose.GetMatrixPalette(posePalette);
            BuildAnimatedLBS(posePalette, invBind, animatedLBS);
            break;
        case DQS_NAIVE:
            pose.GetDualQuaternionPalette(poseDQ);
            break;
        case DQS_PRE:
            pose.GetDualQuaternionPalette(poseDQ);
            BuildAnimatedDQS(poseDQ, invBindDQ, animatedDQS);
            break;
        }

        double t1cpu = QPCMs();

        double t0up = QPCMs();
        shader->Bind();
        Uniform<mat4>::Set(shader->GetUniform("model"),      identity);
        Uniform<mat4>::Set(shader->GetUniform("view"),       view);
        Uniform<mat4>::Set(shader->GetUniform("projection"), proj);

        switch (v) {
        case LBS_NAIVE:
            Uniform<mat4>::Set(shader->GetUniform("pose"),       posePalette);
            Uniform<mat4>::Set(shader->GetUniform("invBindPose"), invBind);
            break;
        case LBS_PRE:
            Uniform<mat4>::Set(shader->GetUniform("animated"), animatedLBS);
            break;
        case DQS_NAIVE:
            Uniform<DualQuaternion>::Set(shader->GetUniform("pose"),       poseDQ);
            Uniform<DualQuaternion>::Set(shader->GetUniform("invBindPose"), invBindDQ);
            break;
        case DQS_PRE:
            Uniform<DualQuaternion>::Set(shader->GetUniform("animated"), animatedDQS);
            break;
        }
        double t1up = QPCMs();

        timer.Begin();
        if (rastDiscard) glEnable(GL_RASTERIZER_DISCARD);

        for (Mesh& m : mWomanMeshes) {
            m.Bind(shader->GetAttribute("position"),
                   shader->GetAttribute("normal"),
                   -1,
                   shader->GetAttribute("weights"),
                   shader->GetAttribute("joints"));
            m.Draw();
            m.UnBind(shader->GetAttribute("position"),
                     shader->GetAttribute("normal"),
                     -1,
                     shader->GetAttribute("weights"),
                     shader->GetAttribute("joints"));
        }

        if (rastDiscard) glDisable(GL_RASTERIZER_DISCARD);
        timer.End();

        shader->UnBind();

        if (!isWarmup) {
            double ms = 0.0;
            if (timer.Pop(ms)) gpuTimes.push_back(ms);
            totalCPU    += (t1cpu - t0cpu);
            totalUpload += (t1up  - t0up);
            measured++;
        }

        t += FIXED_DT;
    }

    timer.Flush();
    double ms = 0.0;
    while (timer.Pop(ms)) gpuTimes.push_back(ms);

    cpuPaletteMs = (measured > 0) ? totalCPU    / measured : 0.0;
    uploadMs     = (measured > 0) ? totalUpload / measured : 0.0;
}

// ============================================================
// Statistics
// ============================================================

double Experiment2::Median(std::vector<double>& v) {
    if (v.empty()) return 0.0;
    std::sort(v.begin(), v.end());
    size_t n = v.size();
    return (n % 2 == 0) ? (v[n/2-1]+v[n/2])*0.5 : v[n/2];
}

double Experiment2::Percentile(std::vector<double>& v, double pct) {
    if (v.empty()) return 0.0;
    std::sort(v.begin(), v.end());
    double idx = pct * (v.size() - 1);
    int lo = (int)idx;
    int hi = lo + 1;
    if (hi >= (int)v.size()) return v.back();
    double frac = idx - lo;
    return v[lo] + frac * (v[hi] - v[lo]);
}

// ============================================================
// Main Run
// ============================================================

void Experiment2::Run() {
    std::cout << "\n=== Experiment 2: GPU Timing ===\n";

    const int NUM_WARMUP  = 300;
    const int NUM_FRAMES  = 2000;
    const int NUM_RUNS    = 5;
    const bool RAST_DISCARD = true;

    // D3 dataset configurations
    int vertCounts[]  = {10000, 100000, 1000000};
    int boneCounts[]  = {20, 60, 120};
    int influCounts[] = {1, 2, 4};

    std::mt19937 rng(42);

    int totalConfigs = 3 * 3 * 3;  // 27 non-woman configs
    int currentConfig = 0;

    // D3 grid
    for (int vc : vertCounts) {
        for (int bc : boneCounts) {
            for (int ic : influCounts) {
                currentConfig++;
                std::cout << "[E2] Config " << currentConfig << "/" << totalConfigs
                          << ": " << vc << " verts, " << bc << " bones, "
                          << ic << " influences\n";

                // Build the synthetic mesh once for this config
                BenchMesh mesh;
                mesh.Generate(vc, bc, ic);
                int actualVerts = mesh.VertexCount();

                // Variant order to shuffle between runs
                std::vector<int> variantOrder = {LBS_NAIVE, LBS_PRE, DQS_NAIVE, DQS_PRE};
                const char* variantNames[] = {"LBS-naive","LBS-pre","DQS-naive","DQS-pre"};

                for (int run = 1; run <= NUM_RUNS; ++run) {
                    // Shuffle variant order
                    std::shuffle(variantOrder.begin(), variantOrder.end(), rng);

                    for (int vi : variantOrder) {
                        Variant v = (Variant)vi;
                        std::vector<double> gpuTimes;
                        double cpuMs = 0.0, upMs = 0.0;
                        RunVariant(v, mesh, bc, ic, NUM_WARMUP, NUM_FRAMES,
                                   RAST_DISCARD, false, gpuTimes, cpuMs, upMs);
                        TimingResult res;
                        res.variant      = variantNames[vi];
                        res.dataset      = "D3";
                        res.vertices     = actualVerts;
                        res.bones        = bc;
                        res.influences   = ic;
                        res.run          = run;
                        res.gpuMsMedian  = Median(gpuTimes);
                        res.gpuMsP5      = Percentile(gpuTimes, 0.05);
                        res.gpuMsP95     = Percentile(gpuTimes, 0.95);
                        res.cpuPaletteMs = cpuMs;
                        res.uploadMs     = upMs;
                        mResults.push_back(res);
                    }
                }
            }
        }
    }

    // Sanity check: time should increase with vertex count.
    // Check for LBS-naive, 60 bones, 4 influences (run 1).
    std::cout << "[E2] Sanity check: vertex scaling...\n";
    bool sanityOK = true;
    double prevMs = -1.0;
    for (int vc : vertCounts) {
        for (auto& r : mResults) {
            if (r.variant == "LBS-naive" && r.bones == 60 && r.influences == 4 && r.vertices == vc && r.run == 1) {
                std::cout << "[E2]   " << vc << " verts: " << r.gpuMsMedian << " ms\n";
                if (prevMs > 0 && r.gpuMsMedian < prevMs * 0.5) {
                    std::cout << "[E2]   WARNING: time didn't scale with vertex count! "
                              << "Driver may be skipping work. Falling back is outside current scope.\n";
                    sanityOK = false;
                }
                prevMs = r.gpuMsMedian;
                break;
            }
        }
    }
    if (sanityOK) std::cout << "[E2] Sanity check: PASS\n";
    else           std::cout << "[E2] Sanity check: FAIL — results may be unreliable\n";

    // Full-rendering confirmation point: 100k verts, 60 bones, 4 influences
    std::cout << "[E2] Full-rendering confirmation point (100k, 60b, 4i)...\n";
    {
        BenchMesh mesh;
        mesh.Generate(100000, 60, 4);
        std::vector<int> variantOrder = {LBS_NAIVE, LBS_PRE, DQS_NAIVE, DQS_PRE};
        const char* variantNames[] = {"LBS-naive","LBS-pre","DQS-naive","DQS-pre"};
        for (int vi : variantOrder) {
            Variant v = (Variant)vi;
            std::vector<double> gpuTimes;
            double cpuMs = 0.0, upMs = 0.0;
            RunVariant(v, mesh, 60, 4, 50, 200, false, false, gpuTimes, cpuMs, upMs);
            TimingResult res;
            res.variant      = std::string(variantNames[vi]) + "_fullrender";
            res.dataset      = "D3_fullrender";
            res.vertices     = mesh.VertexCount();
            res.bones        = 60;
            res.influences   = 4;
            res.run          = 1;
            res.gpuMsMedian  = Median(gpuTimes);
            res.gpuMsP5      = Percentile(gpuTimes, 0.05);
            res.gpuMsP95     = Percentile(gpuTimes, 0.95);
            res.cpuPaletteMs = cpuMs;
            res.uploadMs     = upMs;
            mResults.push_back(res);
            std::cout << "[E2]   " << variantNames[vi] << " full: " << res.gpuMsMedian << " ms\n";
        }
    }

    // Woman.gltf per variant
    if (!mWomanMeshes.empty()) {
        std::cout << "[E2] Woman.gltf benchmarks...\n";
        const char* variantNames[] = {"LBS-naive","LBS-pre","DQS-naive","DQS-pre"};
        int womanVerts = 0;
        for (auto& m : mWomanMeshes) womanVerts += (int)m.GetPosition().size();
        int womanBones = (int)mWomanSkeleton.GetRestPose().Size();

        for (int run = 1; run <= NUM_RUNS; ++run) {
            std::vector<int> variantOrder = {LBS_NAIVE, LBS_PRE, DQS_NAIVE, DQS_PRE};
            std::shuffle(variantOrder.begin(), variantOrder.end(), rng);

            for (int vi : variantOrder) {
                std::vector<double> gpuTimes;
                double cpuMs = 0.0, upMs = 0.0;
                RunVariantWoman((Variant)vi, NUM_WARMUP, NUM_FRAMES, RAST_DISCARD,
                                 gpuTimes, cpuMs, upMs);
                TimingResult res;
                res.variant      = variantNames[vi];
                res.dataset      = "Woman";
                res.vertices     = womanVerts;
                res.bones        = womanBones;
                res.influences   = 4;  // Woman.gltf uses 4 influences
                res.run          = run;
                res.gpuMsMedian  = Median(gpuTimes);
                res.gpuMsP5      = Percentile(gpuTimes, 0.05);
                res.gpuMsP95     = Percentile(gpuTimes, 0.95);
                res.cpuPaletteMs = cpuMs;
                res.uploadMs     = upMs;
                mResults.push_back(res);
                if (run == 1) {
                    std::cout << "[E2]   Woman " << variantNames[vi] << ": " << res.gpuMsMedian << " ms\n";
                }
            }
        }
    }

    WriteCSV("Results/e2_gpu.csv");
    WriteCSV("Results_fixed/e2_gpu.csv");
    WriteCSV("e2_gpu.csv");
    std::cout << "[E2] Done.\n";
}

void Experiment2::WriteCSV(const std::string& path) const {
    std::ofstream f(path);
    if (!f.is_open()) {
        std::cerr << "[E2] ERROR: Cannot write " << path << "\n";
        return;
    }
    f << "variant,dataset,vertices,bones,influences,run,gpu_ms_median,gpu_ms_p5,gpu_ms_p95,cpu_palette_ms,upload_ms\n";
    for (const auto& r : mResults) {
        f << r.variant << "," << r.dataset << "," << r.vertices << ","
          << r.bones << "," << r.influences << "," << r.run << ","
          << r.gpuMsMedian << "," << r.gpuMsP5 << "," << r.gpuMsP95 << ","
          << r.cpuPaletteMs << "," << r.uploadMs << "\n";
    }
    std::cout << "[E2] Wrote " << path << " (" << mResults.size() << " rows)\n";
}

void Experiment2::RunDeadCodeBenchmark() {
    std::cout << "\n=== Dead-Code Risk Benchmark: Discard vs Full Rendering ===\n";
    std::cout << "Comparing Discard (GL_RASTERIZER_DISCARD) vs Full Rendering (normal & fragPos consumed in lit.frag)\n";
    std::cout << "5 runs each at 60 bones, 4 influences\n\n";

    int targetVerts[] = { 100000, 1000000 };
    const char* variantNames[] = { "LBS-naive", "LBS-pre", "DQS-naive", "DQS-pre" };

    struct DeadCodeRow {
        std::string mode;
        int vertices;
        std::string variant;
        double runMedians[5];
        double median;
        double minVal;
        double maxVal;
    };
    std::vector<DeadCodeRow> rows;

    for (int targetVc : targetVerts) {
        BenchMesh mesh;
        mesh.Generate(targetVc, 60, 4);
        int actualVerts = mesh.VertexCount();
        std::cout << "[DeadCode] Mesh generated: " << actualVerts << " vertices, 60 bones, 4 influences\n";

        for (bool rastDiscard : { true, false }) {
            std::string mode = rastDiscard ? "Discard" : "FullRender";
            std::cout << "\n--- Mode: " << mode << " (" << actualVerts << " vertices) ---\n";

            for (int vi = 0; vi < NUM_VARIANTS; ++vi) {
                Variant v = (Variant)vi;
                DeadCodeRow row;
                row.mode = mode;
                row.vertices = actualVerts;
                row.variant = variantNames[vi];

                std::vector<double> runs;
                for (int run = 0; run < 5; ++run) {
                    std::vector<double> gpuTimes;
                    double cpuMs = 0.0, upMs = 0.0;
                    RunVariant(v, mesh, 60, 4, 50, 200, rastDiscard, false, gpuTimes, cpuMs, upMs);
                    double med = Median(gpuTimes);
                    row.runMedians[run] = med;
                    runs.push_back(med);
                }
                std::sort(runs.begin(), runs.end());
                row.minVal = runs.front();
                row.maxVal = runs.back();
                row.median = runs[2];
                rows.push_back(row);

                std::cout << "  " << row.variant << ": median=" << row.median 
                          << " ms [" << row.minVal << " - " << row.maxVal << "]\n";
            }
        }
    }

    for (const std::string& p : { "Results_fixed/dead_code_bench.csv", "Results/dead_code_bench.csv", "dead_code_bench.csv" }) {
        std::ofstream f(p);
        if (f.is_open()) {
            f << "mode,vertices,variant,run1,run2,run3,run4,run5,median,min,max\n";
            for (const auto& r : rows) {
                f << r.mode << "," << r.vertices << "," << r.variant << ","
                  << r.runMedians[0] << "," << r.runMedians[1] << "," << r.runMedians[2] << ","
                  << r.runMedians[3] << "," << r.runMedians[4] << ","
                  << r.median << "," << r.minVal << "," << r.maxVal << "\n";
            }
        }
    }
}
