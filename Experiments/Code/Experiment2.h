#pragma once
#ifndef _H_EXPERIMENT2_
#define _H_EXPERIMENT2_

#include "Shader.h"
#include "Mesh.h"
#include "Skeleton.h"
#include "Pose.h"
#include "BenchMesh.h"
#include "GLTimer.h"
#include <vector>
#include <string>

// Experiment 2: GPU timing of four skinning variants.
// Synthetic meshes (D3 dataset) + Woman.gltf.
// 300 warm-up + 2000 measured frames per config.
// 5 runs per config with shuffled variant order.
// Primary measurement: GL_RASTERIZER_DISCARD to isolate vertex stage.
// One validation point with full rendering.

class Experiment2 {
public:
    Experiment2();
    ~Experiment2();

    bool Initialize();
    void Run();
    void RunDeadCodeBenchmark();
    void Shutdown();

private:
    // Variant indices (for shuffling)
    enum Variant { LBS_NAIVE=0, LBS_PRE=1, DQS_NAIVE=2, DQS_PRE=3, NUM_VARIANTS=4 };

    // Shaders for each variant
    Shader* mShaders[NUM_VARIANTS];

    // Woman.gltf model
    std::vector<Mesh> mWomanMeshes;
    Skeleton           mWomanSkeleton;
    Pose               mWomanPose;
    std::vector<Pose>  mWomanClipPoses;  // precomputed poses

    // Benchmark configuration
    struct Config {
        int     vertices;
        int     bones;
        int     influences;
        bool    isWoman;
        std::string dataset;
    };

    // Run a single configuration for all variants.
    struct TimingResult {
        std::string variant;
        std::string dataset;
        int vertices;
        int bones;
        int influences;
        int run;
        double gpuMsMedian;
        double gpuMsP5;
        double gpuMsP95;
        double cpuPaletteMs;
        double uploadMs;
    };
    std::vector<TimingResult> mResults;

    // Run one config for one variant, return vector of per-frame gpu_ms.
    void RunVariant(Variant v, BenchMesh& mesh, int bones, int influences,
                    int numWarmup, int numFrames,
                    bool rastDiscard, bool useWoman,
                    std::vector<double>& gpuTimes,
                    double& cpuPaletteMs, double& uploadMs);

    // For Woman.gltf
    void RunVariantWoman(Variant v, int numWarmup, int numFrames, bool rastDiscard,
                          std::vector<double>& gpuTimes,
                          double& cpuPaletteMs, double& uploadMs);

    // Build animated (LBS-pre) or combined DQ (DQS-pre) palettes.
    void BuildAnimatedLBS(const std::vector<mat4>& posePalette,
                           const std::vector<mat4>& invBind,
                           std::vector<mat4>& animated);

    void BuildAnimatedDQS(const std::vector<DualQuaternion>& poseDQ,
                           const std::vector<DualQuaternion>& invBindDQ,
                           std::vector<DualQuaternion>& animated);

    // Synthetic pose: a simple rotating bone chain.
    // Uses a fixed time step (not wall-clock).
    std::vector<mat4>          MakeSyntheticLBSPalette(int bones, float t);
    std::vector<DualQuaternion> MakeSyntheticDQSPalette(int bones, float t);
    // Inverse bind for synthetic bone chain (identity for rest pose bones)
    std::vector<mat4>          MakeSyntheticInvBind(int bones);
    std::vector<DualQuaternion> MakeSyntheticInvBindDQ(int bones);

    // Sanity check: time must increase with vertex count.
    bool SanityCheckVertexScaling(const std::vector<TimingResult>& results);

    static double Median(std::vector<double>& v);
    static double Percentile(std::vector<double>& v, double pct);

    void WriteCSV(const std::string& path) const;
};

#endif
