#pragma once
#ifndef _H_EXPERIMENT1_
#define _H_EXPERIMENT1_

#include "Mesh.h"
#include "Skeleton.h"
#include "Pose.h"
#include "DualQuaternion.h"
#include "Shader.h"
#include "Texture.h"
#include <vector>
#include <string>

// Experiment 1: Deformation quality — LBS vs DQS on dq.gltf
// Measures volume preservation and radial distance ratios.
// Includes transform-feedback GPU/CPU validation.
class Experiment1 {
public:
    Experiment1();
    ~Experiment1();

    bool Initialize();
    void Run();
    void Shutdown();

private:
    // --- Model data ---
    std::vector<Mesh>  mMeshes;
    Skeleton           mSkeleton;
    Texture*           mTexture;

    // --- Shaders ---
    Shader* mLBSShader;      // skinned.vert (LBS-naive, for TF validation)
    Shader* mDQSShader;      // dualquaternion.vert (DQS-naive, for TF validation)
    Shader* mLBSTFShader;    // skinned_tf.vert (with tf_position out)
    Shader* mDQSTFShader;    // dualquaternion_tf.vert (with tf_position out)

    // --- CPU skinning ---
    // LBS: skinned position = sum_i w_i * pose[i] * invBind[i] * restPos
    std::vector<vec3> SkinLBS(const std::vector<vec3>& restPos,
                               const std::vector<ivec4>& joints,
                               const std::vector<vec4>& weights,
                               const std::vector<mat4>& posePalette,
                               const std::vector<mat4>& invBind) const;

    // DQS: skinned position using dual quaternion blending
    std::vector<vec3> SkinDQS(const std::vector<vec3>& restPos,
                               const std::vector<ivec4>& joints,
                               const std::vector<vec4>& weights,
                               const std::vector<DualQuaternion>& poseDQ,
                               const std::vector<DualQuaternion>& invBindDQ) const;

    // --- Geometry metrics ---
    // Signed volume of closed mesh via signed tetrahedra.
    double ComputeVolume(const std::vector<vec3>& pos,
                          const std::vector<unsigned int>& idx) const;

    // Per-slice metrics: rotation-invariant thickness and legacy axis distance.
    struct SliceResult {
        float sliceX;
        float thicknessRatio;         // rotation-invariant thickness: mean dist from slice centroid / rest mean dist
        float maxAxisDistRatioLegacy; // legacy radial distance from longitudinal X axis
        bool isBlended;               // true if slice contains blended (skinned) vertices
    };
    std::vector<SliceResult> ComputeSliceDistances(
        const std::vector<vec3>& skinnedPos,
        const std::vector<vec3>& restPos,
        const std::vector<ivec4>& joints,
        const std::vector<vec4>& weights,
        int numSlices = 32) const;

    // --- Pose construction ---
    // Rotate bone[1] about the bar long axis (Twist) or perpendicular (Bend).
    Pose MakeTwistPose(float angleDeg);
    Pose MakeBendPose(float angleDeg);

    // --- Transform feedback validation ---
    struct TFResult {
        bool passed;
        double maxDiff;
        int numVertices;
    };
    TFResult ValidateLBSWithTF(Pose pose, float angleDeg);
    TFResult ValidateDQSWithTF(Pose pose, float angleDeg);

    // --- TF GPU capture helper ---
    std::vector<vec3> CaptureGPU_LBS(const std::vector<mat4>& posePalette,
                                      const std::vector<mat4>& invBind,
                                      const Mesh& mesh);
    std::vector<vec3> CaptureGPU_DQS(const std::vector<DualQuaternion>& poseDQ,
                                      const std::vector<DualQuaternion>& invBindDQ,
                                      const Mesh& mesh);

    // Validation: compare LBS-naive vs LBS-pre outputs
    void ValidateLBSNaiveVsPre();
    void ValidateDQSNaiveVsPre();

    // Validation: LBS twist analytical formula
    // |r(theta,w)| = sqrt(1 - 2w(1-w)(1-cos(theta)))
    // where w is weight of bone1, original r = 1 (normalized radial dist).
    void ValidateLBSTwistAnalytical();

    // CSV output
    void WriteQualityCSV(const std::string& path) const;
    void WriteSlicesCSV(const std::string& path) const;

    // Results storage
    struct QualityRow {
        std::string method;
        std::string motion;
        float angleDeg;
        double volumeRatio;
        float minThicknessRatio;      // min thickness ratio over blended slices
        float maxThicknessRatio;      // max thickness ratio over blended slices
        float maxAxisDistRatioLegacy; // legacy max_dist_ratio
    };
    struct SliceRow {
        std::string method;
        std::string motion;
        float angleDeg;
        float sliceX;
        float thicknessRatio;
        float maxAxisDistRatioLegacy;
    };
    std::vector<QualityRow> mQualityRows;
    std::vector<SliceRow>   mSliceRows;

    double mRestVolume;
};

#endif
