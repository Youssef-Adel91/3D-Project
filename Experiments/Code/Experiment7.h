#pragma once
#ifndef _H_EXPERIMENT7_
#define _H_EXPERIMENT7_

#include "Mesh.h"
#include "mat4.h"
#include "DualQuaternion.h"
#include <vector>
#include <string>

// Experiment 7: Scale Cases Evaluation
// Evaluates:
//   - Case A: Static uniform scale (0.01 on root in bind & pose, Woman.gltf scenario)
//   - Case B: Uniform scale difference between bind (1.0) and pose (1.2)
//   - Case C: Non-uniform scale (1.0, 1.3, 1.0) on animated bone
// Compares: LBS (Reference), DQS-naive (per-node DQ), DQS-pre (pre-multiplied matrix -> DQ).
// Reports: Max and mean position error relative to bar length (L = 10.0 units).
// Output: e7_scale.csv, E7_REPORT.md

class Experiment7 {
public:
    Experiment7();
    ~Experiment7();

    bool Initialize();
    void Run();
    void Shutdown();

    struct ScaleResult {
        std::string testCase;
        std::string method;
        double maxErrorAbs;
        double meanErrorAbs;
        double maxErrorRel;  // relative to bar length (10.0)
        double meanErrorRel;
        bool   reproducesWithin1e4;
    };

    const std::vector<ScaleResult>& GetResults() const { return mResults; }

private:
    Mesh mMesh;
    double mBarLength;
    std::vector<ScaleResult> mResults;

    // CPU Skinning
    std::vector<vec3> SkinLBS(const std::vector<vec3>& restPos,
                              const std::vector<ivec4>& joints,
                              const std::vector<vec4>& weights,
                              const std::vector<mat4>& posePalette,
                              const std::vector<mat4>& invBind) const;

    std::vector<vec3> SkinDQS(const std::vector<vec3>& restPos,
                              const std::vector<ivec4>& joints,
                              const std::vector<vec4>& weights,
                              const std::vector<DualQuaternion>& poseDQ,
                              const std::vector<DualQuaternion>& invBindDQ) const;

    std::vector<vec3> SkinDQSPre(const std::vector<vec3>& restPos,
                                 const std::vector<ivec4>& joints,
                                 const std::vector<vec4>& weights,
                                 const std::vector<DualQuaternion>& animatedDQ) const;
};

#endif

