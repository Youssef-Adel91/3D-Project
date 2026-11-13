#pragma once
#ifndef _H_EXPERIMENT8_
#define _H_EXPERIMENT8_

#include "Shader.h"
#include "Mesh.h"
#include "Skeleton.h"
#include "Pose.h"
#include "GLTimer.h"
#include "DualQuaternion.h"
#include "mat4.h"
#include <vector>
#include <string>

// Experiment 8 (Optional): Woman.gltf Instanced at Measurable Resolution
// Instances Woman.gltf N times (N = 100, 1000) so that vertex shader invocations
// reach 1.5M and 15.3M vertices (runtimes well above 20 microseconds).
// Evaluates: LBS-pre, LBS-pre-3x4, LBS-blend-once, DQS-pre.
// Note: DQS-naive is marked INVALID due to static root scale failure (proven in E7 Case A).

class Experiment8 {
public:
    Experiment8();
    ~Experiment8();

    bool Initialize();
    void Run();
    void Shutdown();

private:
    std::vector<Mesh> mWomanMeshes;
    Skeleton          mWomanSkeleton;
    Pose              mWomanPose;

    Shader* mShader_LBS_pre;
    Shader* mShader_LBS_pre_3x4;
    Shader* mShader_LBS_blend_once;
    Shader* mShader_DQS_pre;

    struct WomanTimingResult {
        std::string variant;
        int instances;
        int totalVertices;
        int run;
        double gpuMsMedian;
        double p5;
        double p95;
        std::string status;
    };
    std::vector<WomanTimingResult> mResults;
};

#endif

