#pragma once
#ifndef _H_EXPERIMENT1B_
#define _H_EXPERIMENT1B_

#include "Mesh.h"
#include "Skeleton.h"
#include "Pose.h"
#include "DualQuaternion.h"
#include <vector>
#include <string>

// Experiment 1b: Sensitivity of deformation quality to weight smoothness (W)
// CPU only, no GPU timing.
// Evaluates LBS volume loss and DQS bulging across linear ramps W in {0.25, 0.5, 1, 2, 4, 8}
// and tests a three-joint kinematic chain.
class Experiment1b {
public:
    Experiment1b();
    ~Experiment1b();

    bool Initialize();
    void Run();
    void Shutdown();

private:
    // Models & Skeletons
    Mesh     mBaseMesh;       // 10 x 2 x 2, closed, 32 slices along x
    Skeleton mSkeleton2;      // Two-joint skeleton
    double   mRestVolume2;

    // Procedural bar construction / reweighting
    Mesh CreateTwoJointBar(float W, bool useOriginalGLTFWeights = false) const;

    // Three-joint bar & kinematics
    struct ThreeJointBar {
        Mesh mesh;
        double restVolume;
    };
    ThreeJointBar CreateThreeJointBar(float W) const;

    // CPU skinning functions (exact match to Experiment1.cpp)
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

    double ComputeVolume(const std::vector<vec3>& pos,
                          const std::vector<unsigned int>& idx) const;

    struct SliceResult {
        float sliceX;
        float thicknessRatio;
        float maxAxisDistRatioLegacy;
        bool isBlended;
    };
    std::vector<SliceResult> ComputeSliceDistances(
        const std::vector<vec3>& skinnedPos,
        const std::vector<vec3>& restPos,
        const std::vector<ivec4>& joints,
        const std::vector<vec4>& weights,
        int numSlices = 32) const;

    // Two-joint pose construction
    Pose MakeTwistPose(float angleDeg);
    Pose MakeBendPose(float angleDeg);

    // Three-joint pose construction
    void MakeThreeJointPoses(float twist1Deg, float twist2Deg,
                             float bend1Deg, float bend2Deg,
                             std::vector<mat4>& outPosePalette,
                             std::vector<DualQuaternion>& outPoseDQ) const;

    // Validations
    void ValidateLBSTwistAnalytical(float W);
    void VerifyOriginalBarE1();

    // Results storage
    struct QualityRow {
        std::string method;
        std::string motion;
        float angleDeg;
        float W;
        double volumeRatio;
        float minThickness;
        float maxThickness;
        float argmaxX;
    };
    std::vector<QualityRow> mQualityRows;

    struct ThreeJointRow {
        std::string method;
        std::string motion;
        float W;
        double volumeRatio;
        float minThickness;
        float maxThickness;
        float argmaxX;
    };
    std::vector<ThreeJointRow> mThreeJointRows;

    void WriteQualityCSV(const std::string& path) const;
    void WriteThreeJointCSV(const std::string& path) const;
};

#endif
