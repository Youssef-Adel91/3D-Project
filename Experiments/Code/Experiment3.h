#pragma once
#ifndef _H_EXPERIMENT3_
#define _H_EXPERIMENT3_

#include "Shader.h"
#include "BenchMesh.h"
#include "GLTimer.h"
#include "DualQuaternion.h"
#include "mat4.h"
#include <vector>
#include <string>

// Experiment 3: Joint-data upload path comparison.
// LBS-pre and DQS-pre only, with 100k vertices and 4 influences.
// Upload paths: uniform array, UBO (std140), texture buffer (samplerBuffer).
// Bone counts: 60, 120, 256, 512.
// 5 runs, each measuring gpu_ms and upload_ms.

class Experiment3 {
public:
    Experiment3();
    ~Experiment3();

    bool Initialize();
    void Run();
    void Shutdown();

private:
    enum UploadPath { PATH_UNIFORM=0, PATH_UBO=1, PATH_TBO=2, NUM_PATHS=3 };
    enum Variant    { VARIANT_LBS_PRE=0, VARIANT_DQS_PRE=1, NUM_VARIANTS=2 };

    // Shaders: one per (variant, path) combination.
    // For uniform: use existing Shaders/preskinned.vert and Shaders/dqs_pre.vert
    // For UBO:     use new Shaders/preskinned_ubo.vert and Shaders/dqs_pre_ubo.vert
    // For TBO:     use new Shaders/preskinned_tbo.vert and Shaders/dqs_pre_tbo.vert
    Shader* mShaders[NUM_VARIANTS][NUM_PATHS];

    // GPU resources for UBO and TBO paths
    GLuint mUBO_LBS = 0, mUBO_DQS = 0;
    GLuint mTBO_LBS_tex = 0, mTBO_LBS_buf = 0;
    GLuint mTBO_DQS_tex = 0, mTBO_DQS_buf = 0;

    // GL limits
    GLint mMaxUniformComponents = 0;
    GLint mMaxUBOSize = 0;
    GLint mMaxTBOSize = 0;

    struct TimingResult {
        std::string variant;
        std::string path;
        int bones;
        int run;
        double gpuMsMedian;
        double uploadMs;
        std::string status;  // "ok" or "exceeds_limit"
    };
    std::vector<TimingResult> mResults;

    // Run one (variant, path, bones) combination.
    void RunConfig(Variant var, UploadPath path, int bones, int run,
                   BenchMesh& mesh);

    // Upload methods
    void UploadUniform_LBS(const std::vector<mat4>& animated, Shader* shader);
    void UploadUniform_DQS(const std::vector<DualQuaternion>& animated, Shader* shader);
    void UploadUBO_LBS(const std::vector<mat4>& animated, GLuint ubo);
    void UploadUBO_DQS(const std::vector<DualQuaternion>& animated, GLuint ubo);
    void UploadTBO_LBS(const std::vector<mat4>& animated);
    void UploadTBO_DQS(const std::vector<DualQuaternion>& animated);

    // Build palette
    std::vector<mat4>          BuildLBSPalette(int bones, float t);
    std::vector<DualQuaternion> BuildDQSPalette(int bones, float t);

    static double Median(std::vector<double>& v);

    void WriteCSV(const std::string& path) const;

    // GL limit checks
    bool CheckUniformLimit(int bones, bool isDQS) const;
    bool CheckUBOLimit(int bones, bool isDQS) const;
};

#endif
