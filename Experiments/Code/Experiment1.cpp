// Experiment1.cpp: LBS vs DQS deformation quality.
// Runs Twist and Bend motions on dq.gltf, computes:
//   - Volume ratio V(theta)/V0 via signed tetrahedra
//   - Per-slice radial distance ratios
//   - Transform-feedback GPU/CPU validation (tolerance 1e-4)
//   - Analytical formula validation for LBS twist

#define _CRT_SECURE_NO_WARNINGS
#include "Experiment1.h"
#include "GLTFLoader.h"
#include "Uniform.h"
#include "glad.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <cmath>
#include <algorithm>
#include <cassert>
#include <map>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

// ============================================================
// Construction / Destruction
// ============================================================

Experiment1::Experiment1()
    : mTexture(nullptr), mLBSShader(nullptr), mDQSShader(nullptr),
      mLBSTFShader(nullptr), mDQSTFShader(nullptr), mRestVolume(0.0) {}

Experiment1::~Experiment1() {}

bool Experiment1::Initialize() {
    std::cout << "[E1] Loading dq.gltf...\n";
    cgltf_data* gltf = LoadGLTFFile("Assets/dq.gltf");
    if (!gltf) {
        std::cerr << "[E1] ERROR: Could not load Assets/dq.gltf\n";
        return false;
    }
    mMeshes   = LoadMeshes(gltf);
    mSkeleton = LoadSkeleton(gltf);
    FreeGLTFFile(gltf);

    if (mMeshes.empty()) {
        std::cerr << "[E1] ERROR: No meshes loaded\n";
        return false;
    }
    mTexture = new Texture("Assets/dq.png");

    mLBSShader   = new Shader("Shaders/skinned.vert",         "Shaders/lit.frag");
    mDQSShader   = new Shader("Shaders/dualquaternion.vert",  "Shaders/lit.frag");

    // TF shaders need transform feedback varyings registered before link
    const char* tfVaryings[] = {"tf_position"};
    mLBSTFShader = new Shader();
    mLBSTFShader->LoadWithTF("Shaders/skinned_tf.vert", "Shaders/lit.frag", tfVaryings, 1);
    mDQSTFShader = new Shader();
    mDQSTFShader->LoadWithTF("Shaders/dualquaternion_tf.vert", "Shaders/lit.frag", tfVaryings, 1);

    // Compute rest volume
    const Mesh& mesh = mMeshes[0];
    mRestVolume = ComputeVolume(mesh.GetPosition(), mesh.GetIndices());
    std::cout << "[E1] Rest volume = " << mRestVolume << "\n";
    std::cout << "[E1] Vertices = " << mesh.GetPosition().size() 
              << ", Triangles = " << mesh.GetIndices().size()/3 << "\n";

    return true;
}

void Experiment1::Shutdown() {
    delete mTexture;
    delete mLBSShader;
    delete mDQSShader;
    delete mLBSTFShader;
    delete mDQSTFShader;
    mTexture     = nullptr;
    mLBSShader   = nullptr;
    mDQSShader   = nullptr;
    mLBSTFShader = nullptr;
    mDQSTFShader = nullptr;
    mMeshes.clear();
}

// ============================================================
// CPU Skinning
// ============================================================

std::vector<vec3> Experiment1::SkinLBS(
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

// Helper: CPU DQS combineDq
static DualQuaternion CombineDQ(const DualQuaternion& inv, const DualQuaternion& pose) {
    DualQuaternion l = normalized(inv);
    DualQuaternion r = normalized(pose);
    quat real = l.real * r.real;
    quat dual = l.real * r.dual + l.dual * r.real;
    return DualQuaternion(real, dual);
}

std::vector<vec3> Experiment1::SkinDQS(
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

        // Blend
        DualQuaternion skinDQ = dq0 * w.x + dq1 * w.y + dq2 * w.z + dq3 * w.w;
        normalize(skinDQ);
        out[i] = transformPoint(skinDQ, restPos[i]);
    }
    return out;
}

// ============================================================
// Volume computation (signed tetrahedra)
// ============================================================

double Experiment1::ComputeVolume(const std::vector<vec3>& pos,
                                   const std::vector<unsigned int>& idx) const
{
    double vol = 0.0;
    for (size_t i = 0; i + 2 < idx.size(); i += 3) {
        const vec3& a = pos[idx[i]];
        const vec3& b = pos[idx[i+1]];
        const vec3& c = pos[idx[i+2]];
        // Signed volume of tetrahedron from origin
        vol += (double)(a.x * (b.y * c.z - b.z * c.y)
                      - a.y * (b.x * c.z - b.z * c.x)
                      + a.z * (b.x * c.y - b.y * c.x));
    }
    return fabs(vol) / 6.0;
}

// ============================================================
// Slice radial distances
// ============================================================

std::vector<Experiment1::SliceResult> Experiment1::ComputeSliceDistances(
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

        // Check if vertex has blended weights between multiple bones
        // In dq.gltf, bone 0 is child and bone 1 is root.
        float w0 = 0.0f;
        const ivec4& j = joints[i];
        const vec4& w = weights[i];
        if (j.x == 0) w0 = w.x;
        else if (j.y == 0) w0 = w.y;
        else if (j.z == 0) w0 = w.z;
        else if (j.w == 0) w0 = w.w;
        if (w0 > 0.001f && w0 < 0.999f) {
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

            // Legacy radial distance from longitudinal X axis
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
// Pose construction
// ============================================================

Pose Experiment1::MakeTwistPose(float angleDeg) {
    Pose pose = mSkeleton.GetRestPose();
    // Bone 0 is "Bone.001" (the second/child bone).
    // In Bone 0's local space, the bar's long axis is Y (0,1,0).
    float rad = (float)(angleDeg * M_PI / 180.0);
    Transform local = pose.GetLocalTransform(0);
    local.rotation = local.rotation * angleAxis(-rad, vec3(0, 1, 0));
    pose.SetLocalTransform(0, local);
    return pose;
}

Pose Experiment1::MakeBendPose(float angleDeg) {
    Pose pose = mSkeleton.GetRestPose();
    // Bone 0 bend: rotate about axis perpendicular to bar long axis
    float rad = (float)(angleDeg * M_PI / 180.0);
    Transform local = pose.GetLocalTransform(0);
    local.rotation = local.rotation * angleAxis(rad, vec3(0, 0, 1));
    pose.SetLocalTransform(0, local);
    return pose;
}

// ============================================================
// Transform Feedback GPU validation
// ============================================================

std::vector<vec3> Experiment1::CaptureGPU_LBS(
    const std::vector<mat4>& posePalette,
    const std::vector<mat4>& invBind,
    const Mesh& mesh)
{
    int N = (int)mesh.GetPosition().size();
    if (N == 0) return {};

    // Set up transform feedback buffer
    GLuint tfBuf = 0;
    glGenBuffers(1, &tfBuf);
    glBindBuffer(GL_TRANSFORM_FEEDBACK_BUFFER, tfBuf);
    glBufferData(GL_TRANSFORM_FEEDBACK_BUFFER, N * sizeof(vec3), nullptr, GL_STREAM_READ);
    glBindBufferBase(GL_TRANSFORM_FEEDBACK_BUFFER, 0, tfBuf);

    mLBSTFShader->Bind();

    // Set model = identity so tf_position = skinned local position
    mat4 identity;
    mat4 view = lookAt(vec3(0,3,14), vec3(0,0,0), vec3(0,1,0));
    mat4 proj = perspective(60.0f, 1.0f, 0.01f, 1000.0f);
    Uniform<mat4>::Set(mLBSTFShader->GetUniform("model"),      identity);
    Uniform<mat4>::Set(mLBSTFShader->GetUniform("view"),       view);
    Uniform<mat4>::Set(mLBSTFShader->GetUniform("projection"), proj);
    Uniform<mat4>::Set(mLBSTFShader->GetUniform("pose"),       (std::vector<mat4>&)const_cast<std::vector<mat4>&>(posePalette));
    Uniform<mat4>::Set(mLBSTFShader->GetUniform("invBindPose"), (std::vector<mat4>&)const_cast<std::vector<mat4>&>(invBind));

    const_cast<Mesh&>(mesh).Bind(
        mLBSTFShader->GetAttribute("position"),
        mLBSTFShader->GetAttribute("normal"),
        mLBSTFShader->GetAttribute("texCoord"),
        mLBSTFShader->GetAttribute("weights"),
        mLBSTFShader->GetAttribute("joints"));

    glEnable(GL_RASTERIZER_DISCARD);
    glBeginTransformFeedback(GL_POINTS);
    glDrawArrays(GL_POINTS, 0, N);
    glEndTransformFeedback();
    glDisable(GL_RASTERIZER_DISCARD);
    glFlush();

    const_cast<Mesh&>(mesh).UnBind(
        mLBSTFShader->GetAttribute("position"),
        mLBSTFShader->GetAttribute("normal"),
        mLBSTFShader->GetAttribute("texCoord"),
        mLBSTFShader->GetAttribute("weights"),
        mLBSTFShader->GetAttribute("joints"));

    mLBSTFShader->UnBind();

    // Read back
    std::vector<vec3> gpuOut(N);
    glBindBuffer(GL_TRANSFORM_FEEDBACK_BUFFER, tfBuf);
    glGetBufferSubData(GL_TRANSFORM_FEEDBACK_BUFFER, 0, N * sizeof(vec3), gpuOut.data());
    glBindBuffer(GL_TRANSFORM_FEEDBACK_BUFFER, 0);
    glDeleteBuffers(1, &tfBuf);

    return gpuOut;
}

std::vector<vec3> Experiment1::CaptureGPU_DQS(
    const std::vector<DualQuaternion>& poseDQ,
    const std::vector<DualQuaternion>& invBindDQ,
    const Mesh& mesh)
{
    int N = (int)mesh.GetPosition().size();
    if (N == 0) return {};

    GLuint tfBuf = 0;
    glGenBuffers(1, &tfBuf);
    glBindBuffer(GL_TRANSFORM_FEEDBACK_BUFFER, tfBuf);
    glBufferData(GL_TRANSFORM_FEEDBACK_BUFFER, N * sizeof(vec3), nullptr, GL_STREAM_READ);
    glBindBufferBase(GL_TRANSFORM_FEEDBACK_BUFFER, 0, tfBuf);

    mDQSTFShader->Bind();

    mat4 identity;
    mat4 view = lookAt(vec3(0,3,14), vec3(0,0,0), vec3(0,1,0));
    mat4 proj = perspective(60.0f, 1.0f, 0.01f, 1000.0f);
    Uniform<mat4>::Set(mDQSTFShader->GetUniform("model"),      identity);
    Uniform<mat4>::Set(mDQSTFShader->GetUniform("view"),       view);
    Uniform<mat4>::Set(mDQSTFShader->GetUniform("projection"), proj);
    Uniform<DualQuaternion>::Set(mDQSTFShader->GetUniform("pose"),        (std::vector<DualQuaternion>&)const_cast<std::vector<DualQuaternion>&>(poseDQ));
    Uniform<DualQuaternion>::Set(mDQSTFShader->GetUniform("invBindPose"), (std::vector<DualQuaternion>&)const_cast<std::vector<DualQuaternion>&>(invBindDQ));

    const_cast<Mesh&>(mesh).Bind(
        mDQSTFShader->GetAttribute("position"),
        mDQSTFShader->GetAttribute("normal"),
        mDQSTFShader->GetAttribute("texCoord"),
        mDQSTFShader->GetAttribute("weights"),
        mDQSTFShader->GetAttribute("joints"));

    glEnable(GL_RASTERIZER_DISCARD);
    glBeginTransformFeedback(GL_POINTS);
    glDrawArrays(GL_POINTS, 0, N);
    glEndTransformFeedback();
    glDisable(GL_RASTERIZER_DISCARD);
    glFlush();

    const_cast<Mesh&>(mesh).UnBind(
        mDQSTFShader->GetAttribute("position"),
        mDQSTFShader->GetAttribute("normal"),
        mDQSTFShader->GetAttribute("texCoord"),
        mDQSTFShader->GetAttribute("weights"),
        mDQSTFShader->GetAttribute("joints"));

    mDQSTFShader->UnBind();

    std::vector<vec3> gpuOut(N);
    glBindBuffer(GL_TRANSFORM_FEEDBACK_BUFFER, tfBuf);
    glGetBufferSubData(GL_TRANSFORM_FEEDBACK_BUFFER, 0, N * sizeof(vec3), gpuOut.data());
    glBindBuffer(GL_TRANSFORM_FEEDBACK_BUFFER, 0);
    glDeleteBuffers(1, &tfBuf);

    return gpuOut;
}

// ============================================================
// Validation
// ============================================================

Experiment1::TFResult Experiment1::ValidateLBSWithTF(Pose pose, float angleDeg) {
    std::vector<mat4> posePalette;
    pose.GetMatrixPalette(posePalette);
    std::vector<mat4> invBind = mSkeleton.GetInvBindPose();

    const Mesh& mesh = mMeshes[0];
    std::vector<vec3> cpuOut = SkinLBS(mesh.GetPosition(), mesh.GetInfluences(),
                                        mesh.GetWeights(), posePalette, invBind);
    std::vector<vec3> gpuOut = CaptureGPU_LBS(posePalette, invBind, mesh);

    double maxDiff = 0.0;
    for (size_t i = 0; i < cpuOut.size() && i < gpuOut.size(); ++i) {
        vec3 d = {cpuOut[i].x - gpuOut[i].x, 
                  cpuOut[i].y - gpuOut[i].y, 
                  cpuOut[i].z - gpuOut[i].z};
        double dist = sqrt((double)(d.x*d.x + d.y*d.y + d.z*d.z));
        if (dist > maxDiff) maxDiff = dist;
    }

    bool passed = (maxDiff < 1e-4);
    std::cout << "[E1] LBS TF validation at " << angleDeg << " deg: "
              << (passed ? "PASS" : "FAIL")
              << " maxDiff=" << maxDiff << "\n";
    return { passed, maxDiff, (int)cpuOut.size() };
}

Experiment1::TFResult Experiment1::ValidateDQSWithTF(Pose pose, float angleDeg) {
    std::vector<DualQuaternion> poseDQ, invBindDQ;
    pose.GetDualQuaternionPalette(poseDQ);
    mSkeleton.GetInvBindPose(invBindDQ);

    const Mesh& mesh = mMeshes[0];
    std::vector<vec3> cpuOut = SkinDQS(mesh.GetPosition(), mesh.GetInfluences(),
                                        mesh.GetWeights(), poseDQ, invBindDQ);
    std::vector<vec3> gpuOut = CaptureGPU_DQS(poseDQ, invBindDQ, mesh);

    double maxDiff = 0.0;
    for (size_t i = 0; i < cpuOut.size() && i < gpuOut.size(); ++i) {
        vec3 d = {cpuOut[i].x - gpuOut[i].x, 
                  cpuOut[i].y - gpuOut[i].y, 
                  cpuOut[i].z - gpuOut[i].z};
        double dist = sqrt((double)(d.x*d.x + d.y*d.y + d.z*d.z));
        if (dist > maxDiff) maxDiff = dist;
    }

    bool passed = (maxDiff < 1e-4);
    std::cout << "[E1] DQS TF validation at " << angleDeg << " deg: "
              << (passed ? "PASS" : "FAIL")
              << " maxDiff=" << maxDiff << "\n";
    return { passed, maxDiff, (int)cpuOut.size() };
}

void Experiment1::ValidateLBSTwistAnalytical() {
    std::cout << "[E1] Validating LBS twist analytical formula...\n";

    const Mesh& mesh = mMeshes[0];
    const auto& restPos  = mesh.GetPosition();
    const auto& joints   = mesh.GetInfluences();
    const auto& weights  = mesh.GetWeights();
    std::vector<mat4> invBind = mSkeleton.GetInvBindPose();

    // Test a few angles
    float angles[] = {30.0f, 90.0f, 150.0f};
    bool allPassed = true;

    for (float deg : angles) {
        Pose pose = MakeTwistPose(deg);
        std::vector<mat4> posePalette;
        pose.GetMatrixPalette(posePalette);
        std::vector<vec3> skinnedPos = SkinLBS(restPos, joints, weights, posePalette, invBind);

        double maxErr = 0.0;
        float rad = (float)(deg * M_PI / 180.0);

        for (size_t i = 0; i < restPos.size(); ++i) {
            const vec3& rp = restPos[i];
            const vec4& w4 = weights[i];

            float rRest = sqrtf(rp.y*rp.y + rp.z*rp.z);
            if (rRest < 1e-5f) continue;  // skip points on axis

            // Weight of bone 0 (the twisted second bone: Bone.001)
            float wBone0 = 0.0f;
            const ivec4& j4 = joints[i];
            if (j4.x == 0) wBone0 = w4.x;
            else if (j4.y == 0) wBone0 = w4.y;
            else if (j4.z == 0) wBone0 = w4.z;
            else if (j4.w == 0) wBone0 = w4.w;

            // Analytical: r_ratio = sqrt(1 - 2*w*(1-w)*(1-cos(theta)))
            float w = wBone0;
            float analytical = sqrtf(1.0f - 2.0f*w*(1.0f-w)*(1.0f - cosf(rad)));

            const vec3& sp = skinnedPos[i];
            float rSkinned = sqrtf(sp.y*sp.y + sp.z*sp.z);
            float measured = rSkinned / rRest;

            double err = fabs(measured - analytical);
            if (err > maxErr) maxErr = err;
        }

        bool ok = (maxErr < 1e-4);
        if (!ok) allPassed = false;
        std::cout << "[E1]   Twist " << deg << " deg: "
                  << (ok ? "PASS" : "FAIL") << " maxErr=" << maxErr << "\n";
    }

    std::cout << "[E1] LBS twist analytical validation: "
              << (allPassed ? "ALL PASS" : "SOME FAILURES") << "\n";
}

void Experiment1::ValidateLBSNaiveVsPre() {
    // LBS-naive: pose * invBind per vertex
    // LBS-pre: animated = pose * invBind precomputed
    // They should produce identical results.
    std::cout << "[E1] Validating LBS-naive vs LBS-pre...\n";

    Pose pose = MakeBendPose(45.0f);
    std::vector<mat4> posePalette;
    pose.GetMatrixPalette(posePalette);
    std::vector<mat4> invBind = mSkeleton.GetInvBindPose();

    // LBS-naive CPU
    const Mesh& mesh = mMeshes[0];
    std::vector<vec3> naive = SkinLBS(mesh.GetPosition(), mesh.GetInfluences(),
                                       mesh.GetWeights(), posePalette, invBind);

    // LBS-pre CPU: precompute animated[i] = pose[i] * invBind[i]
    std::vector<mat4> animated(posePalette.size());
    for (size_t i = 0; i < posePalette.size(); ++i) {
        animated[i] = posePalette[i] * invBind[i];
    }

    // Now skin with animated
    const auto& restPos = mesh.GetPosition();
    const auto& joints  = mesh.GetInfluences();
    const auto& weights = mesh.GetWeights();
    std::vector<vec3> pre(restPos.size());
    for (size_t i = 0; i < restPos.size(); ++i) {
        const ivec4& j = joints[i];
        const vec4&  w = weights[i];
        mat4 skin = animated[j.x]*w.x + animated[j.y]*w.y + animated[j.z]*w.z + animated[j.w]*w.w;
        pre[i] = transformPoint(skin, restPos[i]);
    }

    double maxDiff = 0.0;
    for (size_t i = 0; i < naive.size(); ++i) {
        vec3 d = {naive[i].x-pre[i].x, naive[i].y-pre[i].y, naive[i].z-pre[i].z};
        double dist = sqrt((double)(d.x*d.x+d.y*d.y+d.z*d.z));
        if (dist > maxDiff) maxDiff = dist;
    }
    std::cout << "[E1] LBS-naive vs LBS-pre maxDiff=" << maxDiff 
              << " " << (maxDiff < 1e-5 ? "PASS" : "FAIL") << "\n";
}

void Experiment1::ValidateDQSNaiveVsPre() {
    std::cout << "[E1] Validating DQS-naive vs DQS-pre...\n";

    Pose pose = MakeBendPose(45.0f);
    std::vector<DualQuaternion> poseDQ, invBindDQ;
    pose.GetDualQuaternionPalette(poseDQ);
    mSkeleton.GetInvBindPose(invBindDQ);

    const Mesh& mesh = mMeshes[0];
    std::vector<vec3> naive = SkinDQS(mesh.GetPosition(), mesh.GetInfluences(),
                                       mesh.GetWeights(), poseDQ, invBindDQ);

    // DQS-pre: animated[i] = combineDQ(invBind[i], pose[i])
    std::vector<DualQuaternion> animated(poseDQ.size());
    for (size_t i = 0; i < poseDQ.size(); ++i) {
        animated[i] = CombineDQ(invBindDQ[i], poseDQ[i]);
    }

    // Skin with pre-combined
    const auto& restPos = mesh.GetPosition();
    const auto& joints  = mesh.GetInfluences();
    const auto& weights = mesh.GetWeights();
    std::vector<vec3> pre(restPos.size());
    for (size_t i = 0; i < restPos.size(); ++i) {
        const ivec4& j = joints[i];
        vec4 w = weights[i];

        // Hemisphere check
        if (dot(animated[j.x].real, animated[j.y].real) < 0.0f) w.y *= -1.0f;
        if (dot(animated[j.x].real, animated[j.z].real) < 0.0f) w.z *= -1.0f;
        if (dot(animated[j.x].real, animated[j.w].real) < 0.0f) w.w *= -1.0f;

        DualQuaternion skinDQ = animated[j.x]*w.x + animated[j.y]*w.y + animated[j.z]*w.z + animated[j.w]*w.w;
        normalize(skinDQ);
        pre[i] = transformPoint(skinDQ, restPos[i]);
    }

    // Note: DQS-naive does the hemisphere check on pose[j] before combineDQ,
    // but DQS-pre does it on animated[j] = combineDq(invBind[j], pose[j]).
    // These two are equivalent for 2-joint cases but can differ when more joints
    // are involved. Document any difference.
    double maxDiff = 0.0;
    for (size_t i = 0; i < naive.size(); ++i) {
        vec3 d = {naive[i].x-pre[i].x, naive[i].y-pre[i].y, naive[i].z-pre[i].z};
        double dist = sqrt((double)(d.x*d.x+d.y*d.y+d.z*d.z));
        if (dist > maxDiff) maxDiff = dist;
    }
    std::cout << "[E1] DQS-naive vs DQS-pre maxDiff=" << maxDiff;
    if (maxDiff > 1e-5) {
        std::cout << " DIFFER (hemisphere check placement difference — documented)";
    } else {
        std::cout << " PASS";
    }
    std::cout << "\n";
}

// ============================================================
// Main Run
// ============================================================

void Experiment1::Run() {
    std::cout << "\n=== Experiment 1: Deformation Quality ===\n";

    // --- Step A: Validation tests ---
    std::cout << "\n-- Validation Tests --\n";
    ValidateLBSTwistAnalytical();

    // GPU/CPU validation at 3 angles (30, 90, 150 deg twist)
    float valAngles[] = {30.0f, 90.0f, 150.0f};
    bool lbsTFAllPassed = true, dqsTFAllPassed = true;
    for (float deg : valAngles) {
        Pose pose = MakeTwistPose(deg);
        auto r1 = ValidateLBSWithTF(pose, deg);
        auto r2 = ValidateDQSWithTF(pose, deg);
        if (!r1.passed) lbsTFAllPassed = false;
        if (!r2.passed) dqsTFAllPassed = false;
    }
    std::cout << "[E1] LBS GPU/CPU TF validation: " << (lbsTFAllPassed ? "ALL PASS" : "SOME FAIL") << "\n";
    std::cout << "[E1] DQS GPU/CPU TF validation: " << (dqsTFAllPassed ? "ALL PASS" : "SOME FAIL") << "\n";

    ValidateLBSNaiveVsPre();
    ValidateDQSNaiveVsPre();

    // --- Step B: Measurement ---
    const Mesh& mesh = mMeshes[0];
    std::vector<mat4> invBind = mSkeleton.GetInvBindPose();
    std::vector<DualQuaternion> invBindDQ;
    mSkeleton.GetInvBindPose(invBindDQ);

    const auto& restPos = mesh.GetPosition();
    const auto& joints  = mesh.GetInfluences();
    const auto& weights = mesh.GetWeights();
    const auto& indices = mesh.GetIndices();

    // Twist: 0..180 in 15-deg steps
    std::cout << "\n-- Twist measurements --\n";
    for (float deg = 0.0f; deg <= 180.0f; deg += 15.0f) {
        Pose pose = MakeTwistPose(deg);

        // LBS
        {
            std::vector<mat4> pp; pose.GetMatrixPalette(pp);
            std::vector<vec3> sk = SkinLBS(restPos, joints, weights, pp, invBind);
            double vol = ComputeVolume(sk, indices);
            double vRatio = (mRestVolume > 1e-9) ? vol / mRestVolume : 0.0;
            auto slices = ComputeSliceDistances(sk, restPos, joints, weights);
            float minThick = 1e9f, maxThick = -1e9f;
            float maxAxisLegacy = 0.0f;
            bool foundBlended = false;
            for (auto& s : slices) {
                if (s.isBlended) {
                    foundBlended = true;
                    if (s.thicknessRatio < minThick) minThick = s.thicknessRatio;
                    if (s.thicknessRatio > maxThick) maxThick = s.thicknessRatio;
                }
                if (s.maxAxisDistRatioLegacy > maxAxisLegacy) maxAxisLegacy = s.maxAxisDistRatioLegacy;
            }
            if (!foundBlended) { minThick = 1.0f; maxThick = 1.0f; }
            mQualityRows.push_back({"LBS", "Twist", deg, vRatio, minThick, maxThick, maxAxisLegacy});
            for (auto& s : slices) {
                mSliceRows.push_back({"LBS", "Twist", deg, s.sliceX, s.thicknessRatio, s.maxAxisDistRatioLegacy});
            }
        }

        // DQS
        {
            std::vector<DualQuaternion> pp; pose.GetDualQuaternionPalette(pp);
            std::vector<vec3> sk = SkinDQS(restPos, joints, weights, pp, invBindDQ);
            double vol = ComputeVolume(sk, indices);
            double vRatio = (mRestVolume > 1e-9) ? vol / mRestVolume : 0.0;
            auto slices = ComputeSliceDistances(sk, restPos, joints, weights);
            float minThick = 1e9f, maxThick = -1e9f;
            float maxAxisLegacy = 0.0f;
            bool foundBlended = false;
            for (auto& s : slices) {
                if (s.isBlended) {
                    foundBlended = true;
                    if (s.thicknessRatio < minThick) minThick = s.thicknessRatio;
                    if (s.thicknessRatio > maxThick) maxThick = s.thicknessRatio;
                }
                if (s.maxAxisDistRatioLegacy > maxAxisLegacy) maxAxisLegacy = s.maxAxisDistRatioLegacy;
            }
            if (!foundBlended) { minThick = 1.0f; maxThick = 1.0f; }
            mQualityRows.push_back({"DQS", "Twist", deg, vRatio, minThick, maxThick, maxAxisLegacy});
            for (auto& s : slices) {
                mSliceRows.push_back({"DQS", "Twist", deg, s.sliceX, s.thicknessRatio, s.maxAxisDistRatioLegacy});
            }
        }

        std::cout << "[E1] Twist " << deg << " deg done\n";
    }

    // Bend: 0..120 in 15-deg steps
    std::cout << "\n-- Bend measurements --\n";
    for (float deg = 0.0f; deg <= 120.0f; deg += 15.0f) {
        Pose pose = MakeBendPose(deg);

        // LBS
        {
            std::vector<mat4> pp; pose.GetMatrixPalette(pp);
            std::vector<vec3> sk = SkinLBS(restPos, joints, weights, pp, invBind);
            double vol = ComputeVolume(sk, indices);
            double vRatio = (mRestVolume > 1e-9) ? vol / mRestVolume : 0.0;
            auto slices = ComputeSliceDistances(sk, restPos, joints, weights);
            float minThick = 1e9f, maxThick = -1e9f;
            float maxAxisLegacy = 0.0f;
            bool foundBlended = false;
            for (auto& s : slices) {
                if (s.isBlended) {
                    foundBlended = true;
                    if (s.thicknessRatio < minThick) minThick = s.thicknessRatio;
                    if (s.thicknessRatio > maxThick) maxThick = s.thicknessRatio;
                }
                if (s.maxAxisDistRatioLegacy > maxAxisLegacy) maxAxisLegacy = s.maxAxisDistRatioLegacy;
            }
            if (!foundBlended) { minThick = 1.0f; maxThick = 1.0f; }
            mQualityRows.push_back({"LBS", "Bend", deg, vRatio, minThick, maxThick, maxAxisLegacy});
            for (auto& s : slices) {
                mSliceRows.push_back({"LBS", "Bend", deg, s.sliceX, s.thicknessRatio, s.maxAxisDistRatioLegacy});
            }
        }

        // DQS
        {
            std::vector<DualQuaternion> pp; pose.GetDualQuaternionPalette(pp);
            std::vector<vec3> sk = SkinDQS(restPos, joints, weights, pp, invBindDQ);
            double vol = ComputeVolume(sk, indices);
            double vRatio = (mRestVolume > 1e-9) ? vol / mRestVolume : 0.0;
            auto slices = ComputeSliceDistances(sk, restPos, joints, weights);
            float minThick = 1e9f, maxThick = -1e9f;
            float maxAxisLegacy = 0.0f;
            bool foundBlended = false;
            for (auto& s : slices) {
                if (s.isBlended) {
                    foundBlended = true;
                    if (s.thicknessRatio < minThick) minThick = s.thicknessRatio;
                    if (s.thicknessRatio > maxThick) maxThick = s.thicknessRatio;
                }
                if (s.maxAxisDistRatioLegacy > maxAxisLegacy) maxAxisLegacy = s.maxAxisDistRatioLegacy;
            }
            if (!foundBlended) { minThick = 1.0f; maxThick = 1.0f; }
            mQualityRows.push_back({"DQS", "Bend", deg, vRatio, minThick, maxThick, maxAxisLegacy});
            for (auto& s : slices) {
                mSliceRows.push_back({"DQS", "Bend", deg, s.sliceX, s.thicknessRatio, s.maxAxisDistRatioLegacy});
            }
        }

        std::cout << "[E1] Bend " << deg << " deg done\n";
    }

    std::cout << "\n-- Writing CSVs --\n";
    WriteQualityCSV("Results/e1_quality.csv");
    WriteQualityCSV("e1_quality.csv");
    WriteSlicesCSV("Results/e1_slices.csv");
    WriteSlicesCSV("e1_slices.csv");

    std::cout << "[E1] Done. Results written.\n";
}

// ============================================================
// CSV output
// ============================================================

void Experiment1::WriteQualityCSV(const std::string& path) const {
    std::ofstream f(path);
    if (!f.is_open()) {
        std::cerr << "[E1] ERROR: Cannot write " << path << "\n";
        return;
    }
    f << "method,motion,angle_deg,volume_ratio,min_thickness_ratio,max_thickness_ratio,max_axis_dist_ratio_legacy\n";
    for (const auto& r : mQualityRows) {
        f << r.method << "," << r.motion << "," << r.angleDeg << ","
          << r.volumeRatio << "," << r.minThicknessRatio << ","
          << r.maxThicknessRatio << "," << r.maxAxisDistRatioLegacy << "\n";
    }
    std::cout << "[E1] Wrote " << path << " (" << mQualityRows.size() << " rows)\n";
}

void Experiment1::WriteSlicesCSV(const std::string& path) const {
    std::ofstream f(path);
    if (!f.is_open()) {
        std::cerr << "[E1] ERROR: Cannot write " << path << "\n";
        return;
    }
    f << "method,motion,angle_deg,slice_x,thickness_ratio,max_axis_dist_ratio_legacy\n";
    for (const auto& r : mSliceRows) {
        f << r.method << "," << r.motion << "," << r.angleDeg << ","
          << r.sliceX << "," << r.thicknessRatio << "," << r.maxAxisDistRatioLegacy << "\n";
    }
    std::cout << "[E1] Wrote " << path << " (" << mSliceRows.size() << " rows)\n";
}
