#pragma once
#ifndef _H_EXPERIMENT6_
#define _H_EXPERIMENT6_

#include "Mesh.h"
#include "Skeleton.h"
#include "Pose.h"
#include "DualQuaternion.h"
#include "mat4.h"
#include <vector>
#include <string>

// Experiment 6: Demanding Quality Tests (Three-Joint Chain, Steep Weights, Local Bulging)
// Evaluates:
//   - Three-joint chain (joints at -2.5, 0, +2.5)
//   - Extreme bend angles: 60+60, 90+90, 120+120, 150+150 degrees
//   - Steep weight ramps: W in {0.5, 1.0, 2.0}
//   - Metrics: Volume ratio, min/max slice thickness, per-vertex radial stretch ratio (p99, max, argmax x)
//   - Joint ring cross-section perimeter ratio
//   - OBJ/PLY mesh export and vertex-color bulge maps (150+150 bend, W=0.5)

class Experiment6 {
public:
    Experiment6();
    ~Experiment6();

    bool Initialize();
    void Run();
    void Shutdown();

private:
    Mesh mBaseMesh;
    double mRestVolume;

    struct ThreeJointMesh {
        Mesh mesh;
        double restVolume;
    };
    ThreeJointMesh CreateThreeJointBar(float W) const;

    // CPU Skinning
    std::vector<vec3> SkinLBS(const std::vector<vec3>& restPos,
                              const std::vector<ivec4>& joints,
                              const std::vector<vec4>& weights,
                              const std::vector<mat4>& posePalette) const;

    std::vector<vec3> SkinDQS(const std::vector<vec3>& restPos,
                              const std::vector<ivec4>& joints,
                              const std::vector<vec4>& weights,
                              const std::vector<DualQuaternion>& poseDQ) const;

    double ComputeVolume(const std::vector<vec3>& pos,
                          const std::vector<unsigned int>& idx) const;

    // Kinematics for three-joint chain (bends at -2.5 and +2.5, center segment at 0)
    void MakeThreeJointPoses(float bend1Deg, float bend2Deg,
                             std::vector<mat4>& outLBS,
                             std::vector<DualQuaternion>& outDQS) const;

    // Per-vertex bulge metric
    struct VertexMetricResult {
        double p99Ratio;
        double maxRatio;
        float  argmaxX;
        double ringPerimeterRatio;
        std::vector<double> vertexRatios;
    };

    VertexMetricResult ComputeVertexBulgeMetric(
        const std::vector<vec3>& skinnedPos,
        const std::vector<vec3>& restPos,
        const std::vector<vec4>& weights,
        const std::vector<mat4>& poseLBS,
        const std::vector<DualQuaternion>& poseDQS,
        bool isDQS) const;

    // Slice thickness computation
    struct SliceResult {
        float sliceX;
        float thicknessRatio;
        bool isBlended;
    };
    std::vector<SliceResult> ComputeSlices(
        const std::vector<vec3>& skinnedPos,
        const std::vector<vec3>& restPos,
        const std::vector<vec4>& weights,
        int numSlices = 32) const;

    // Export OBJ and PLY with per-vertex color
    void ExportOBJ(const std::string& path, const std::vector<vec3>& pos,
                   const std::vector<unsigned int>& indices) const;

    void ExportPLYWithColors(const std::string& path, const std::vector<vec3>& pos,
                             const std::vector<unsigned int>& indices,
                             const std::vector<double>& ratios) const;
};

#endif

