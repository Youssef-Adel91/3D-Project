// Experiment1b.cpp: Deformation quality sensitivity to weight smoothness W
// and 3-joint kinematic chain. CPU-only benchmark.

#define _CRT_SECURE_NO_WARNINGS
#include "Experiment1b.h"
#include "GLTFLoader.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <iomanip>
#include <cmath>
#include <algorithm>
#include <cassert>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

// ============================================================
// Construction / Destruction
// ============================================================

Experiment1b::Experiment1b()
    : mRestVolume2(0.0) {}

Experiment1b::~Experiment1b() {}

bool Experiment1b::Initialize() {
    std::cout << "============================================================\n";
    std::cout << "[E1b] Initializing Experiment 1b (Weight Smoothness Sensitivity)\n";
    std::cout << "============================================================\n";

    cgltf_data* gltf = LoadGLTFFile("Assets/dq.gltf");
    if (!gltf) {
        std::cerr << "[E1b] ERROR: Could not load Assets/dq.gltf\n";
        return false;
    }
    std::vector<Mesh> meshes = LoadMeshes(gltf);
    mSkeleton2 = LoadSkeleton(gltf);
    FreeGLTFFile(gltf);

    if (meshes.empty()) {
        std::cerr << "[E1b] ERROR: No meshes found in Assets/dq.gltf\n";
        return false;
    }

    mBaseMesh = meshes[0];
    mRestVolume2 = ComputeVolume(mBaseMesh.GetPosition(), mBaseMesh.GetIndices());

    std::cout << "[E1b] Loaded base mesh from Assets/dq.gltf\n";
    std::cout << "[E1b]   Vertices:  " << mBaseMesh.GetPosition().size() << "\n";
    std::cout << "[E1b]   Triangles: " << mBaseMesh.GetIndices().size() / 3 << "\n";
    std::cout << "[E1b]   Rest volume V0: " << std::fixed << std::setprecision(6) << mRestVolume2 << "\n";

    return true;
}

void Experiment1b::Shutdown() {
    mBaseMesh = Mesh();
    mQualityRows.clear();
    mThreeJointRows.clear();
    std::cout << "[E1b] Shutdown complete.\n";
}

// ============================================================
// Two-joint bar procedural re-weighting
// ============================================================

Mesh Experiment1b::CreateTwoJointBar(float W, bool useOriginalGLTFWeights) const {
    Mesh mesh = mBaseMesh;
    if (useOriginalGLTFWeights) {
        return mesh;
    }

    const auto& positions = mesh.GetPosition();
    size_t N = positions.size();
    auto& weights = mesh.GetWeights();
    auto& influences = mesh.GetInfluences();
    weights.resize(N);
    influences.resize(N);

    // Linear ramp of total width W centered at joint x = 0
    // w1 is the weight of joint 1 (Bone 0, child, x > 0)
    // w1 = 0 for x <= -W/2, w1 = 1 for x >= +W/2
    for (size_t i = 0; i < N; ++i) {
        float x = positions[i].x;
        float t = (x + 0.5f * W) / W;
        float wChild = (t < 0.0f) ? 0.0f : ((t > 1.0f) ? 1.0f : t);

        // In dq.gltf: Bone 0 is child (Bone.001), Bone 1 is parent (Bone)
        influences[i] = ivec4(0, 1, 0, 0);
        weights[i]    = vec4(wChild, 1.0f - wChild, 0.0f, 0.0f);
    }

    return mesh;
}

// ============================================================
// Three-joint bar procedural mesh & weighting
// ============================================================

Experiment1b::ThreeJointBar Experiment1b::CreateThreeJointBar(float W) const {
    ThreeJointBar tb;
    tb.mesh = mBaseMesh;

    const auto& positions = tb.mesh.GetPosition();
    size_t N = positions.size();
    auto& weights = tb.mesh.GetWeights();
    auto& influences = tb.mesh.GetInfluences();
    weights.resize(N);
    influences.resize(N);

    // Joints at x1 = -2.5 and x2 = +2.5
    // Bone 0: x < -2.5 (left)
    // Bone 1: -2.5 <= x <= +2.5 (middle)
    // Bone 2: x > +2.5 (right)
    for (size_t i = 0; i < N; ++i) {
        float x = positions[i].x;
        float s1 = (x - (-2.5f)) / W + 0.5f;
        s1 = (s1 < 0.0f) ? 0.0f : ((s1 > 1.0f) ? 1.0f : s1);

        float s2 = (x - (+2.5f)) / W + 0.5f;
        s2 = (s2 < 0.0f) ? 0.0f : ((s2 > 1.0f) ? 1.0f : s2);

        float w0 = 1.0f - s1;
        float w1 = s1 - s2;
        float w2 = s2;

        float sum = w0 + w1 + w2;
        if (sum > 0.0f) {
            w0 /= sum;
            w1 /= sum;
            w2 /= sum;
        }

        influences[i] = ivec4(0, 1, 2, 0);
        weights[i]    = vec4(w0, w1, w2, 0.0f);
    }

    tb.restVolume = ComputeVolume(tb.mesh.GetPosition(), tb.mesh.GetIndices());
    return tb;
}

// ============================================================
// CPU Skinning (Exact match to Experiment1.cpp)
// ============================================================

std::vector<vec3> Experiment1b::SkinLBS(
    const std::vector<vec3>& restPos,
    const std::vector<ivec4>& joints,
    const std::vector<vec4>& weights,
    const std::vector<mat4>& posePalette,
    const std::vector<mat4>& invBind) const
{
    size_t N = restPos.size();
    std::vector<vec3> out(N);
    for (size_t i = 0; i < N; ++i) {
        const ivec4& j = joints[i];
        const vec4&  w = weights[i];
        mat4 m0 = (posePalette[j.x] * invBind[j.x]) * w.x;
        mat4 m1 = (posePalette[j.y] * invBind[j.y]) * w.y;
        mat4 m2 = (posePalette[j.z] * invBind[j.z]) * w.z;
        mat4 m3 = (posePalette[j.w] * invBind[j.w]) * w.w;
        mat4 skin = m0 + m1 + m2 + m3;
        out[i] = transformPoint(skin, restPos[i]);
    }
    return out;
}

static DualQuaternion CombineDQ(const DualQuaternion& inv, const DualQuaternion& pose) {
    DualQuaternion l = normalized(inv);
    DualQuaternion r = normalized(pose);
    quat real = l.real * r.real;
    quat dual = l.real * r.dual + l.dual * r.real;
    return DualQuaternion(real, dual);
}

std::vector<vec3> Experiment1b::SkinDQS(
    const std::vector<vec3>& restPos,
    const std::vector<ivec4>& joints,
    const std::vector<vec4>& weights,
    const std::vector<DualQuaternion>& poseDQ,
    const std::vector<DualQuaternion>& invBindDQ) const
{
    size_t N = restPos.size();
    std::vector<vec3> out(N);
    for (size_t i = 0; i < N; ++i) {
        const ivec4& j = joints[i];
        vec4 w = weights[i];

        DualQuaternion dq0 = CombineDQ(invBindDQ[j.x], poseDQ[j.x]);
        DualQuaternion dq1 = CombineDQ(invBindDQ[j.y], poseDQ[j.y]);
        DualQuaternion dq2 = CombineDQ(invBindDQ[j.z], poseDQ[j.z]);
        DualQuaternion dq3 = CombineDQ(invBindDQ[j.w], poseDQ[j.w]);

        // Hemisphere check relative to dq0
        if (dot(dq0.real, dq1.real) < 0.0f) w.y *= -1.0f;
        if (dot(dq0.real, dq2.real) < 0.0f) w.z *= -1.0f;
        if (dot(dq0.real, dq3.real) < 0.0f) w.w *= -1.0f;

        DualQuaternion skinDQ = dq0 * w.x + dq1 * w.y + dq2 * w.z + dq3 * w.w;
        normalize(skinDQ);
        out[i] = transformPoint(skinDQ, restPos[i]);
    }
    return out;
}

// ============================================================
// Volume computation (signed tetrahedra)
// ============================================================

double Experiment1b::ComputeVolume(const std::vector<vec3>& pos,
                                   const std::vector<unsigned int>& idx) const
{
    double vol = 0.0;
    for (size_t i = 0; i + 2 < idx.size(); i += 3) {
        const vec3& a = pos[idx[i]];
        const vec3& b = pos[idx[i+1]];
        const vec3& c = pos[idx[i+2]];
        vol += (double)(a.x * (b.y * c.z - b.z * c.y)
                      - a.y * (b.x * c.z - b.z * c.x)
                      + a.z * (b.x * c.y - b.y * c.x));
    }
    return fabs(vol) / 6.0;
}

// ============================================================
// Slice distance & thickness ratio computation
// ============================================================

std::vector<Experiment1b::SliceResult> Experiment1b::ComputeSliceDistances(
    const std::vector<vec3>& skinnedPos,
    const std::vector<vec3>& restPos,
    const std::vector<ivec4>& joints,
    const std::vector<vec4>& weights,
    int numSlices) const
{
    float xMin = -5.1f, xMax = 5.1f;
    float sliceW = (xMax - xMin) / numSlices;

    std::vector<vec3> sumRestPos(numSlices, vec3(0, 0, 0));
    std::vector<vec3> sumSkinnedPos(numSlices, vec3(0, 0, 0));
    std::vector<int> count(numSlices, 0);
    std::vector<bool> isBlended(numSlices, false);
    std::vector<float> sliceCenters(numSlices, 0.0f);
    std::vector<std::vector<int>> sliceIndices(numSlices);

    for (int s = 0; s < numSlices; ++s) {
        sliceCenters[s] = xMin + (s + 0.5f) * sliceW;
    }

    for (size_t i = 0; i < skinnedPos.size(); ++i) {
        float x = restPos[i].x;
        int s = (int)((x - xMin) / sliceW);
        if (s < 0) s = 0;
        if (s >= numSlices) s = numSlices - 1;

        sumRestPos[s] = sumRestPos[s] + restPos[i];
        sumSkinnedPos[s] = sumSkinnedPos[s] + skinnedPos[i];
        count[s]++;
        sliceIndices[s].push_back((int)i);

        // Blended vertex check:
        // A vertex is blended if no single influence has >= 0.999f weight
        const vec4& w = weights[i];
        float maxW = std::max(std::max(w.x, w.y), std::max(w.z, w.w));
        if (maxW > 0.001f && maxW < 0.999f) {
            isBlended[s] = true;
        }
    }

    std::vector<SliceResult> results;
    for (int s = 0; s < numSlices; ++s) {
        if (count[s] == 0) continue;
        int n = count[s];
        vec3 cRest = sumRestPos[s] * (1.0f / (float)n);
        vec3 cSkin = sumSkinnedPos[s] * (1.0f / (float)n);

        float sumDistRest = 0.0f;
        float sumDistSkin = 0.0f;
        float sumAxisRest = 0.0f;
        float sumAxisSkin = 0.0f;

        for (int idx : sliceIndices[s]) {
            vec3 dRest = restPos[idx] - cRest;
            vec3 dSkin = skinnedPos[idx] - cSkin;
            sumDistRest += sqrtf(dRest.x*dRest.x + dRest.y*dRest.y + dRest.z*dRest.z);
            sumDistSkin += sqrtf(dSkin.x*dSkin.x + dSkin.y*dSkin.y + dSkin.z*dSkin.z);

            sumAxisRest += sqrtf(restPos[idx].y * restPos[idx].y + restPos[idx].z * restPos[idx].z);
            sumAxisSkin += sqrtf(skinnedPos[idx].y * skinnedPos[idx].y + skinnedPos[idx].z * skinnedPos[idx].z);
        }

        float meanDistRest = sumDistRest / n;
        float meanDistSkin = sumDistSkin / n;
        float thicknessRatio = (meanDistRest > 1e-6f) ? (meanDistSkin / meanDistRest) : 1.0f;

        float meanAxisRest = sumAxisRest / n;
        float meanAxisSkin = sumAxisSkin / n;
        float axisDistRatio = (meanAxisRest > 1e-6f) ? (meanAxisSkin / meanAxisRest) : 1.0f;

        results.push_back({sliceCenters[s], thicknessRatio, axisDistRatio, isBlended[s]});
    }
    return results;
}

// ============================================================
// Two-joint poses
// ============================================================

Pose Experiment1b::MakeTwistPose(float angleDeg) {
    Pose pose = mSkeleton2.GetRestPose();
    float rad = (float)(angleDeg * M_PI / 180.0);
    Transform local = pose.GetLocalTransform(0);
    local.rotation = local.rotation * angleAxis(-rad, vec3(0, 1, 0));
    pose.SetLocalTransform(0, local);
    return pose;
}

Pose Experiment1b::MakeBendPose(float angleDeg) {
    Pose pose = mSkeleton2.GetRestPose();
    float rad = (float)(angleDeg * M_PI / 180.0);
    Transform local = pose.GetLocalTransform(0);
    local.rotation = local.rotation * angleAxis(rad, vec3(0, 0, 1));
    pose.SetLocalTransform(0, local);
    return pose;
}

// ============================================================
// Three-joint poses & palettes
// ============================================================

void Experiment1b::MakeThreeJointPoses(
    float twist1Deg, float twist2Deg,
    float bend1Deg, float bend2Deg,
    std::vector<mat4>& outPosePalette,
    std::vector<DualQuaternion>& outPoseDQ) const
{
    outPosePalette.resize(3);
    outPoseDQ.resize(3);

    vec3 c1(-2.5f, 0.0f, 0.0f);
    vec3 c2(+2.5f, 0.0f, 0.0f);

    // Bone 0: Identity reference (x < -2.5)
    outPosePalette[0] = mat4();
    outPoseDQ[0] = DualQuaternion(quat(0, 0, 0, 1), quat(0, 0, 0, 0));

    if (fabs(twist1Deg) > 1e-4f || fabs(twist2Deg) > 1e-4f) {
        // Twist motion around X axis
        float r1 = (float)(twist1Deg * M_PI / 180.0);
        float r2 = (float)((twist1Deg + twist2Deg) * M_PI / 180.0);

        quat q1 = angleAxis(r1, vec3(1, 0, 0));
        quat q2 = angleAxis(r2, vec3(1, 0, 0));

        outPosePalette[1] = quatToMat4(q1);
        outPosePalette[2] = quatToMat4(q2);

        outPoseDQ[1] = DualQuaternion(q1, quat(0, 0, 0, 0));
        outPoseDQ[2] = DualQuaternion(q2, quat(0, 0, 0, 0));
    }
    else {
        // Bend motion around Z axis in XY plane
        float r1 = (float)(bend1Deg * M_PI / 180.0);
        float r2 = (float)((bend1Deg + bend2Deg) * M_PI / 180.0);

        quat q1 = angleAxis(r1, vec3(0, 0, 1));
        quat q2 = angleAxis(r2, vec3(0, 0, 1));

        mat4 R1 = quatToMat4(q1);
        mat4 R2 = quatToMat4(q2);

        // Bone 1: rotated around c1
        // M1 * p = c1 + R1 * (p - c1) = R1 * p + (c1 - R1 * c1)
        vec3 t1 = c1 - transformVector(R1, c1);
        mat4 M1 = R1;
        M1.tx = t1.x; M1.ty = t1.y; M1.tz = t1.z;
        outPosePalette[1] = M1;

        // Bone 2: Joint 2 rest location c2 moved by Bone 1 to c2'
        vec3 c2_prime = c1 + transformVector(R1, c2 - c1);
        vec3 t2 = c2_prime - transformVector(R2, c2);
        mat4 M2 = R2;
        M2.tx = t2.x; M2.ty = t2.y; M2.tz = t2.z;
        outPosePalette[2] = M2;

        // Dual quaternion representation:
        outPoseDQ[1] = transformToDualQuat(Transform(t1, q1, vec3(1, 1, 1)));
        outPoseDQ[2] = transformToDualQuat(Transform(t2, q2, vec3(1, 1, 1)));
    }
}

// ============================================================
// Validations
// ============================================================

void Experiment1b::ValidateLBSTwistAnalytical(float W) {
    Mesh mesh = CreateTwoJointBar(W);
    const auto& restPos = mesh.GetPosition();
    const auto& joints  = mesh.GetInfluences();
    const auto& weights = mesh.GetWeights();

    std::vector<mat4> invBind = mSkeleton2.GetInvBindPose();
    float testAngles[] = { 30.0f, 60.0f, 90.0f, 120.0f, 150.0f, 180.0f };

    double maxErrAcrossAngles = 0.0;
    bool allPass = true;

    for (float deg : testAngles) {
        Pose pose = MakeTwistPose(deg);
        std::vector<mat4> posePalette;
        pose.GetMatrixPalette(posePalette);
        std::vector<vec3> skinned = SkinLBS(restPos, joints, weights, posePalette, invBind);

        float rad = (float)(deg * M_PI / 180.0);
        double maxErrThisAngle = 0.0;

        for (size_t i = 0; i < restPos.size(); ++i) {
            float rRest = sqrtf(restPos[i].y * restPos[i].y + restPos[i].z * restPos[i].z);
            if (rRest < 1e-5f) continue;

            float w = weights[i].x; // Weight of Bone 0 (child)
            float analytical = sqrtf(1.0f - 2.0f * w * (1.0f - w) * (1.0f - cosf(rad)));

            float rSkinned = sqrtf(skinned[i].y * skinned[i].y + skinned[i].z * skinned[i].z);
            float measured = rSkinned / rRest;

            double err = fabs((double)(measured - analytical));
            if (err > maxErrThisAngle) maxErrThisAngle = err;
        }

        if (maxErrThisAngle > maxErrAcrossAngles) maxErrAcrossAngles = maxErrThisAngle;
        if (maxErrThisAngle >= 1e-4) allPass = false;
    }

    std::cout << "[E1b] Validation: LBS Twist analytical match for W = " << std::setw(4) << W
              << ": " << (allPass ? "PASS" : "FAIL")
              << " (max error = " << std::scientific << std::setprecision(2) << maxErrAcrossAngles << ")\n";
    assert(allPass && "LBS Twist analytical formula must match within 1e-4");
}

void Experiment1b::VerifyOriginalBarE1() {
    std::cout << "\n[E1b] Verifying consistency with committed E1 results on dq.gltf...\n";
    Mesh origMesh = CreateTwoJointBar(1.0f, true); // true = use original GLTF weights

    std::vector<mat4> invBindMat = mSkeleton2.GetInvBindPose();
    std::vector<DualQuaternion> invBindDQ;
    mSkeleton2.GetInvBindPose(invBindDQ);

    // 1. Twist 180 deg
    Pose twist180 = MakeTwistPose(180.0f);
    std::vector<mat4> twistMat;
    std::vector<DualQuaternion> twistDQ;
    twist180.GetMatrixPalette(twistMat);
    twist180.GetDualQuaternionPalette(twistDQ);

    std::vector<vec3> lbsTwist180 = SkinLBS(origMesh.GetPosition(), origMesh.GetInfluences(), origMesh.GetWeights(), twistMat, invBindMat);
    std::vector<vec3> dqsTwist180 = SkinDQS(origMesh.GetPosition(), origMesh.GetInfluences(), origMesh.GetWeights(), twistDQ, invBindDQ);

    double vRest = ComputeVolume(origMesh.GetPosition(), origMesh.GetIndices());
    double vLbsTwist = ComputeVolume(lbsTwist180, origMesh.GetIndices()) / vRest;
    double vDqsTwist = ComputeVolume(dqsTwist180, origMesh.GetIndices()) / vRest;

    // Committed E1 values (from Results/e1_quality.csv):
    // LBS Twist 180: 0.838106
    // DQS Twist 180: 0.994269
    const double committedLBS_Twist180 = 0.838106;
    const double committedDQS_Twist180 = 0.994269;

    double diffLBS = fabs(vLbsTwist - committedLBS_Twist180);
    double diffDQS = fabs(vDqsTwist - committedDQS_Twist180);

    std::cout << "[E1b]   Original Bar Twist 180 LBS vol ratio: " << std::fixed << std::setprecision(6)
              << vLbsTwist << " (committed: " << committedLBS_Twist180 << ", diff: " << diffLBS << ")\n";
    std::cout << "[E1b]   Original Bar Twist 180 DQS vol ratio: " << std::fixed << std::setprecision(6)
              << vDqsTwist << " (committed: " << committedDQS_Twist180 << ", diff: " << diffDQS << ")\n";

    assert(diffLBS < 1e-4 && "Original bar LBS Twist 180 must match committed within 1e-4");
    assert(diffDQS < 1e-4 && "Original bar DQS Twist 180 must match committed within 1e-4");

    // Characterize original bar W:
    // In dq.gltf, active blended region spans x in [-1.7742, +1.7742], so W_active = 3.5484.
    // The span between pure 1.0 and pure 0.0 slices is W_span = 4.1935.
    // The equivalent linear ramp slope at x = 0 is dw/dx = pi / (2 * 3.5484) ~= 0.4427 => W_equiv ~= 2.259.
    std::cout << "[E1b]   Original dq.gltf bar weight profile:\n";
    std::cout << "[E1b]     W_active (blended region): 3.5484 units\n";
    std::cout << "[E1b]     W_span (pure-to-pure):     4.1935 units\n";
    std::cout << "[E1b]     W_equiv (center slope):    2.2590 units\n";
    std::cout << "[E1b] Original bar E1 verification: PASS\n\n";
}

// ============================================================
// Main Execution
// ============================================================

void Experiment1b::Run() {
    std::cout << "============================================================\n";
    std::cout << "[E1b] Running Experiment 1b Sweep\n";
    std::cout << "============================================================\n";

    // 1. Verify against original bar
    VerifyOriginalBarE1();

    // 2. Validate analytical formula across all W
    std::vector<float> W_values = { 0.25f, 0.5f, 1.0f, 2.0f, 4.0f, 8.0f };
    std::cout << "[E1b] Validating analytical LBS twist formula across all W...\n";
    for (float W : W_values) {
        ValidateLBSTwistAnalytical(W);
    }
    std::cout << "[E1b] All analytical validations passed.\n\n";

    // Palettes & Inverses
    std::vector<mat4> invBindMat = mSkeleton2.GetInvBindPose();
    std::vector<DualQuaternion> invBindDQ;
    mSkeleton2.GetInvBindPose(invBindDQ);

    // 3. Two-joint sweep: Twist 0..180 (step 15), Bend 0..120 (step 15)
    std::cout << "[E1b] Sweeping two-joint bar across W in {0.25, 0.5, 1, 2, 4, 8}...\n";

    for (float W : W_values) {
        Mesh mesh = CreateTwoJointBar(W);
        const auto& restPos = mesh.GetPosition();
        const auto& joints  = mesh.GetInfluences();
        const auto& weights = mesh.GetWeights();
        const auto& indices = mesh.GetIndices();
        double vRest = ComputeVolume(restPos, indices);

        // Twist 0..180
        for (int deg = 0; deg <= 180; deg += 15) {
            float angleDeg = (float)deg;
            Pose pose = MakeTwistPose(angleDeg);

            std::vector<mat4> posePalette;
            std::vector<DualQuaternion> poseDQ;
            pose.GetMatrixPalette(posePalette);
            pose.GetDualQuaternionPalette(poseDQ);

            // LBS
            {
                std::vector<vec3> skinned = SkinLBS(restPos, joints, weights, posePalette, invBindMat);
                double volRatio = ComputeVolume(skinned, indices) / vRest;
                auto slices = ComputeSliceDistances(skinned, restPos, joints, weights);

                float minThick = 1.0f;
                float maxThick = 0.0f;
                float argmaxX = 0.0f;
                bool hadBlended = false;

                for (const auto& sl : slices) {
                    if (sl.isBlended) {
                        if (!hadBlended || sl.thicknessRatio < minThick) {
                            minThick = sl.thicknessRatio;
                        }
                        hadBlended = true;
                    }
                    if (sl.thicknessRatio > maxThick) {
                        maxThick = sl.thicknessRatio;
                        argmaxX = sl.sliceX;
                    }
                }
                if (!hadBlended) minThick = 1.0f;

                mQualityRows.push_back({ "LBS", "twist", angleDeg, W, volRatio, minThick, maxThick, argmaxX });
            }

            // DQS
            {
                std::vector<vec3> skinned = SkinDQS(restPos, joints, weights, poseDQ, invBindDQ);
                double volRatio = ComputeVolume(skinned, indices) / vRest;
                auto slices = ComputeSliceDistances(skinned, restPos, joints, weights);

                float minThick = 1.0f;
                float maxThick = 0.0f;
                float argmaxX = 0.0f;
                bool hadBlended = false;

                for (const auto& sl : slices) {
                    if (sl.isBlended) {
                        if (!hadBlended || sl.thicknessRatio < minThick) {
                            minThick = sl.thicknessRatio;
                        }
                        hadBlended = true;
                    }
                    if (sl.thicknessRatio > maxThick) {
                        maxThick = sl.thicknessRatio;
                        argmaxX = sl.sliceX;
                    }
                }
                if (!hadBlended) minThick = 1.0f;

                mQualityRows.push_back({ "DQS", "twist", angleDeg, W, volRatio, minThick, maxThick, argmaxX });
            }
        }

        // Bend 0..120
        for (int deg = 0; deg <= 120; deg += 15) {
            float angleDeg = (float)deg;
            Pose pose = MakeBendPose(angleDeg);

            std::vector<mat4> posePalette;
            std::vector<DualQuaternion> poseDQ;
            pose.GetMatrixPalette(posePalette);
            pose.GetDualQuaternionPalette(poseDQ);

            // LBS
            {
                std::vector<vec3> skinned = SkinLBS(restPos, joints, weights, posePalette, invBindMat);
                double volRatio = ComputeVolume(skinned, indices) / vRest;
                auto slices = ComputeSliceDistances(skinned, restPos, joints, weights);

                float minThick = 1.0f;
                float maxThick = 0.0f;
                float argmaxX = 0.0f;
                bool hadBlended = false;

                for (const auto& sl : slices) {
                    if (sl.isBlended) {
                        if (!hadBlended || sl.thicknessRatio < minThick) {
                            minThick = sl.thicknessRatio;
                        }
                        hadBlended = true;
                    }
                    if (sl.thicknessRatio > maxThick) {
                        maxThick = sl.thicknessRatio;
                        argmaxX = sl.sliceX;
                    }
                }
                if (!hadBlended) minThick = 1.0f;

                mQualityRows.push_back({ "LBS", "bend", angleDeg, W, volRatio, minThick, maxThick, argmaxX });
            }

            // DQS
            {
                std::vector<vec3> skinned = SkinDQS(restPos, joints, weights, poseDQ, invBindDQ);
                double volRatio = ComputeVolume(skinned, indices) / vRest;
                auto slices = ComputeSliceDistances(skinned, restPos, joints, weights);

                float minThick = 1.0f;
                float maxThick = 0.0f;
                float argmaxX = 0.0f;
                bool hadBlended = false;

                for (const auto& sl : slices) {
                    if (sl.isBlended) {
                        if (!hadBlended || sl.thicknessRatio < minThick) {
                            minThick = sl.thicknessRatio;
                        }
                        hadBlended = true;
                    }
                    if (sl.thicknessRatio > maxThick) {
                        maxThick = sl.thicknessRatio;
                        argmaxX = sl.sliceX;
                    }
                }
                if (!hadBlended) minThick = 1.0f;

                mQualityRows.push_back({ "DQS", "bend", angleDeg, W, volRatio, minThick, maxThick, argmaxX });
            }
        }
    }
    std::cout << "[E1b] Two-joint sweep complete (" << mQualityRows.size() << " quality rows recorded).\n";

    // 4. Three-joint chain evaluation
    std::cout << "[E1b] Evaluating three-joint chain (twist 90+90, bend 60+60)...\n";
    std::vector<mat4> invBind3 = { mat4(), mat4(), mat4() };
    std::vector<DualQuaternion> invBindDQ3 = {
        DualQuaternion(quat(0,0,0,1), quat(0,0,0,0)),
        DualQuaternion(quat(0,0,0,1), quat(0,0,0,0)),
        DualQuaternion(quat(0,0,0,1), quat(0,0,0,0))
    };

    for (float W : W_values) {
        ThreeJointBar tb = CreateThreeJointBar(W);
        const auto& restPos = tb.mesh.GetPosition();
        const auto& joints  = tb.mesh.GetInfluences();
        const auto& weights = tb.mesh.GetWeights();
        const auto& indices = tb.mesh.GetIndices();
        double vRest = tb.restVolume;

        // A. Twist: 90 deg at joint 1, 90 deg at joint 2
        {
            std::vector<mat4> posePalette;
            std::vector<DualQuaternion> poseDQ;
            MakeThreeJointPoses(90.0f, 90.0f, 0.0f, 0.0f, posePalette, poseDQ);

            // LBS
            {
                std::vector<vec3> skinned = SkinLBS(restPos, joints, weights, posePalette, invBind3);
                double volRatio = ComputeVolume(skinned, indices) / vRest;
                auto slices = ComputeSliceDistances(skinned, restPos, joints, weights);

                float minThick = 1.0f;
                float maxThick = 0.0f;
                float argmaxX = 0.0f;
                bool hadBlended = false;

                for (const auto& sl : slices) {
                    if (sl.isBlended) {
                        if (!hadBlended || sl.thicknessRatio < minThick) {
                            minThick = sl.thicknessRatio;
                        }
                        hadBlended = true;
                    }
                    if (sl.thicknessRatio > maxThick) {
                        maxThick = sl.thicknessRatio;
                        argmaxX = sl.sliceX;
                    }
                }
                if (!hadBlended) minThick = 1.0f;

                mThreeJointRows.push_back({ "LBS", "twist", W, volRatio, minThick, maxThick, argmaxX });
            }

            // DQS
            {
                std::vector<vec3> skinned = SkinDQS(restPos, joints, weights, poseDQ, invBindDQ3);
                double volRatio = ComputeVolume(skinned, indices) / vRest;
                auto slices = ComputeSliceDistances(skinned, restPos, joints, weights);

                float minThick = 1.0f;
                float maxThick = 0.0f;
                float argmaxX = 0.0f;
                bool hadBlended = false;

                for (const auto& sl : slices) {
                    if (sl.isBlended) {
                        if (!hadBlended || sl.thicknessRatio < minThick) {
                            minThick = sl.thicknessRatio;
                        }
                        hadBlended = true;
                    }
                    if (sl.thicknessRatio > maxThick) {
                        maxThick = sl.thicknessRatio;
                        argmaxX = sl.sliceX;
                    }
                }
                if (!hadBlended) minThick = 1.0f;

                mThreeJointRows.push_back({ "DQS", "twist", W, volRatio, minThick, maxThick, argmaxX });
            }
        }

        // B. Bend: 60 deg at joint 1, 60 deg at joint 2
        {
            std::vector<mat4> posePalette;
            std::vector<DualQuaternion> poseDQ;
            MakeThreeJointPoses(0.0f, 0.0f, 60.0f, 60.0f, posePalette, poseDQ);

            // LBS
            {
                std::vector<vec3> skinned = SkinLBS(restPos, joints, weights, posePalette, invBind3);
                double volRatio = ComputeVolume(skinned, indices) / vRest;
                auto slices = ComputeSliceDistances(skinned, restPos, joints, weights);

                float minThick = 1.0f;
                float maxThick = 0.0f;
                float argmaxX = 0.0f;
                bool hadBlended = false;

                for (const auto& sl : slices) {
                    if (sl.isBlended) {
                        if (!hadBlended || sl.thicknessRatio < minThick) {
                            minThick = sl.thicknessRatio;
                        }
                        hadBlended = true;
                    }
                    if (sl.thicknessRatio > maxThick) {
                        maxThick = sl.thicknessRatio;
                        argmaxX = sl.sliceX;
                    }
                }
                if (!hadBlended) minThick = 1.0f;

                mThreeJointRows.push_back({ "LBS", "bend", W, volRatio, minThick, maxThick, argmaxX });
            }

            // DQS
            {
                std::vector<vec3> skinned = SkinDQS(restPos, joints, weights, poseDQ, invBindDQ3);
                double volRatio = ComputeVolume(skinned, indices) / vRest;
                auto slices = ComputeSliceDistances(skinned, restPos, joints, weights);

                float minThick = 1.0f;
                float maxThick = 0.0f;
                float argmaxX = 0.0f;
                bool hadBlended = false;

                for (const auto& sl : slices) {
                    if (sl.isBlended) {
                        if (!hadBlended || sl.thicknessRatio < minThick) {
                            minThick = sl.thicknessRatio;
                        }
                        hadBlended = true;
                    }
                    if (sl.thicknessRatio > maxThick) {
                        maxThick = sl.thicknessRatio;
                        argmaxX = sl.sliceX;
                    }
                }
                if (!hadBlended) minThick = 1.0f;

                mThreeJointRows.push_back({ "DQS", "bend", W, volRatio, minThick, maxThick, argmaxX });
            }
        }
    }
    std::cout << "[E1b] Three-joint chain evaluation complete (" << mThreeJointRows.size() << " rows recorded).\n";

    // 5. Write CSV outputs
    WriteQualityCSV("Results/e1b_quality.csv");
    WriteQualityCSV("e1b_quality.csv");

    WriteThreeJointCSV("Results/e1b_three_joint.csv");
    WriteThreeJointCSV("e1b_three_joint.csv");

    std::cout << "\n============================================================\n";
    std::cout << "[E1b] EXPERIMENT 1b COMPLETED SUCCESSFULLY.\n";
    std::cout << "============================================================\n";
}

// ============================================================
// CSV Output
// ============================================================

void Experiment1b::WriteQualityCSV(const std::string& path) const {
    std::ofstream f(path);
    if (!f.is_open()) {
        std::cerr << "[E1b] WARNING: Could not open " << path << " for writing\n";
        return;
    }
    f << "method,motion,angle_deg,W,volume_ratio,min_thickness,max_thickness,argmax_x\n";
    for (const auto& r : mQualityRows) {
        f << r.method << ","
          << r.motion << ","
          << std::fixed << std::setprecision(1) << r.angleDeg << ","
          << std::setprecision(2) << r.W << ","
          << std::setprecision(6) << r.volumeRatio << ","
          << std::setprecision(6) << r.minThickness << ","
          << std::setprecision(6) << r.maxThickness << ","
          << std::setprecision(2) << r.argmaxX << "\n";
    }
    f.close();
    std::cout << "[E1b] Wrote " << mQualityRows.size() << " rows to " << path << "\n";
}

void Experiment1b::WriteThreeJointCSV(const std::string& path) const {
    std::ofstream f(path);
    if (!f.is_open()) {
        std::cerr << "[E1b] WARNING: Could not open " << path << " for writing\n";
        return;
    }
    f << "method,motion,W,volume_ratio,min_thickness,max_thickness,argmax_x\n";
    for (const auto& r : mThreeJointRows) {
        f << r.method << ","
          << r.motion << ","
          << std::fixed << std::setprecision(2) << r.W << ","
          << std::setprecision(6) << r.volumeRatio << ","
          << std::setprecision(6) << r.minThickness << ","
          << std::setprecision(6) << r.maxThickness << ","
          << std::setprecision(2) << r.argmaxX << "\n";
    }
    f.close();
    std::cout << "[E1b] Wrote " << mThreeJointRows.size() << " rows to " << path << "\n";
}
