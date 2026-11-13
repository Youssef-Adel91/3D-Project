#pragma once
#ifndef _H_EXPERIMENT4_
#define _H_EXPERIMENT4_

#include "Shader.h"
#include "Mesh.h"
#include "BenchMesh.h"
#include "GLTimer.h"
#include "DualQuaternion.h"
#include "mat4.h"
#include <vector>
#include <string>

// Experiment 4: Stronger LBS Baselines
// Evaluates:
//   - LBS-pre-3x4 (Uniform, UBO, TBO)
//   - LBS-naive-opt (Uniform)
//   - LBS-blend-once (Uniform)
// Against baseline: LBS-pre (4x4), LBS-naive (4x4), DQS-pre, DQS-naive.
// Mesh sizes: 9,996 / 99,852 / 999,996 vertices, 60 bones, 4 influences.
// Protocol: 300 warm-up + 2,000 frames, 5 runs with shuffled variant order, GL_RASTERIZER_DISCARD.
// Transform feedback validation at 30, 90, 150 degrees, tolerance 1e-4.

class Experiment4 {
public:
    Experiment4();
    ~Experiment4();

    bool Initialize();
    void Run();
    void RunComparePaths();
    void RunE2SweepAudit(bool reversed);
    void Shutdown();

    // Validation results (max error across 30, 90, 150 deg)
    struct ValidationResult {
        std::string variant;
        std::string shaderPath;
        double maxError30;
        double maxError90;
        double maxError150;
        double overallMaxError;
        bool passed;
    };
    const std::vector<ValidationResult>& GetValidationResults() const { return mValidations; }

private:
    enum E4Variant {
        VAR_LBS_NAIVE_BASE = 0,
        VAR_LBS_NAIVE_OPT  = 1,
        VAR_LBS_PRE_4X4    = 2,
        VAR_LBS_PRE_3X4    = 3,
        VAR_LBS_BLEND_ONCE = 4,
        VAR_DQS_NAIVE_BASE = 5,
        VAR_DQS_PRE_BASE   = 6,
        NUM_E4_VARIANTS    = 7
    };

    enum E4Path {
        PATH_UNIFORM = 0,
        PATH_UBO     = 1,
        PATH_TBO     = 2,
        NUM_E4_PATHS = 3
    };

    // Shaders for main timing
    Shader* mShaders[NUM_E4_VARIANTS];
    Shader* mShader_3x4_UBO;
    Shader* mShader_3x4_TBO;

    // Transform feedback validation shaders
    Shader* mTF_LBS_pre_3x4;
    Shader* mTF_LBS_pre_3x4_ubo;
    Shader* mTF_LBS_pre_3x4_tbo;
    Shader* mTF_LBS_naive_opt;
    Shader* mTF_LBS_blend_once;

    // GPU buffers for UBO and TBO
    GLuint mUBO_3x4;
    GLuint mTBO_3x4_buf;
    GLuint mTBO_3x4_tex;

    // GL limits
    GLint mMaxUniformComponents;
    GLint mMaxUBOSize;
    GLint mMaxTBOSize;

    std::vector<ValidationResult> mValidations;

    struct TimingResult {
        std::string variant;
        std::string path;
        int vertices;
        int bones;
        int influences;
        int run;
        double gpuMsMedian;
        double gpuMsP5;
        double gpuMsP95;
        double cpuPaletteMs;
        double uploadMs;
        std::string status;
    };
    std::vector<TimingResult> mResults;

    void RunValidation();

    void RunConfig(int variantIdx, int pathIdx, BenchMesh& mesh, int bones, int influences,
                   int numWarmup, int numFrames,
                   std::vector<double>& gpuTimes, double& cpuMs, double& uploadMs);

    // Palette helpers
    std::vector<mat4>           MakeSyntheticLBSPalette(int bones, float t);
    std::vector<DualQuaternion> MakeSyntheticDQSPalette(int bones, float t);
    std::vector<mat4>           MakeSyntheticInvBind(int bones);
    std::vector<DualQuaternion> MakeSyntheticInvBindDQ(int bones);
    void BuildAnimatedLBS(const std::vector<mat4>& pose, const std::vector<mat4>& invBind, std::vector<mat4>& out);
    void BuildAnimated3x4(const std::vector<mat4>& animated4x4, std::vector<vec4>& out3x4Rows);
    void BuildAnimatedDQS(const std::vector<DualQuaternion>& pose, const std::vector<DualQuaternion>& invBind, std::vector<DualQuaternion>& out);

    // Limit check
    int GetMaxBonesUniform(int componentsPerBone) const;
};

#endif

