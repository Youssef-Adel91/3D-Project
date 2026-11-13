#pragma once
#ifndef _H_EXPERIMENT5_
#define _H_EXPERIMENT5_

#include "Shader.h"
#include "BenchMesh.h"
#include "GLTimer.h"
#include "DualQuaternion.h"
#include "mat4.h"
#include <vector>
#include <string>

// Experiment 5: Vertex Order and Memory Bandwidth
// Evaluates:
//   - Coherent vs Shuffled (randomly permuted) vertex order
//   - Variants: LBS-pre, LBS-pre-3x4, DQS-pre, DQS-naive
//   - Vertex counts: 99,852 and 999,996 vertices
//   - Bone counts: 60 and 120 bones (4 influences)
//   - Pure read-only passthrough kernel (empirical bandwidth floor)
// Outputs: e5_vertex_order.csv, e5_bandwidth.csv, E5_REPORT.md

class Experiment5 {
public:
    Experiment5();
    ~Experiment5();

    bool Initialize();
    void Run(bool onlyShuffleMode = false);
    void RunE5Exact();
    void RunE5bChecks();
    void Shutdown();

    // Verify shuffled mesh equivalence: un-permuted positions match coherent within 1e-6
    static bool VerifyShuffledEquivalence(const BenchMesh& coherent, const BenchMesh& shuffled,
                                          const std::vector<int>& perm, double& outMaxError);

    // Create a shuffled copy of a BenchMesh
    static void CreateShuffledMesh(const BenchMesh& inCoherent, BenchMesh& outShuffled,
                                   std::vector<int>& outPerm, unsigned int seed = 42);

    // Create a block-permuted copy of a BenchMesh (permutes vertices only inside blocks of size blockSize)
    static void CreateBlockShuffledMesh(const BenchMesh& inCoherent, BenchMesh& outShuffled,
                                        std::vector<int>& outPerm, int blockSize, unsigned int seed = 42);

private:
    enum E5Variant {
        VAR_LBS_PRE_4X4 = 0,
        VAR_LBS_PRE_3X4 = 1,
        VAR_DQS_PRE     = 2,
        VAR_DQS_NAIVE   = 3,
        VAR_PASSTHROUGH = 4,
        NUM_E5_VARIANTS = 5
    };

    Shader* mShaders[NUM_E5_VARIANTS];

    struct OrderTimingResult {
        std::string variant;
        int vertices;
        int bones;
        int influences;
        std::string order; // "coherent" or "shuffled"
        int run;
        double gpuMsMedian;
        double p5;
        double p95;
    };
    std::vector<OrderTimingResult> mOrderResults;

    struct BandwidthResult {
        std::string variant;
        int vertices;
        int bones;
        std::string order;
        double gpuMsMedian;
        double dataBytes;
        double effectiveGBs;
        double nominalGBs;
        double percentOfNominal;
    };
    std::vector<BandwidthResult> mBandwidthResults;

    void RunVariant(int varIdx, BenchMesh& mesh, int bones, int influences,
                    int numWarmup, int numFrames, std::vector<double>& gpuTimes);

    std::vector<mat4>           MakeSyntheticLBSPalette(int bones, float t);
    std::vector<DualQuaternion> MakeSyntheticDQSPalette(int bones, float t);
    std::vector<mat4>           MakeSyntheticInvBind(int bones);
    std::vector<DualQuaternion> MakeSyntheticInvBindDQ(int bones);
    void BuildAnimatedLBS(const std::vector<mat4>& pose, const std::vector<mat4>& invBind, std::vector<mat4>& out);
    void BuildAnimated3x4(const std::vector<mat4>& animated4x4, std::vector<vec4>& out3x4Rows);
    void BuildAnimatedDQS(const std::vector<DualQuaternion>& pose, const std::vector<DualQuaternion>& invBind, std::vector<DualQuaternion>& out);
};

#endif

