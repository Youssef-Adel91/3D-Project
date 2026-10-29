#define _CRT_SECURE_NO_WARNINGS
#include "Sample.h"
#include "GLTFLoader.h"
#include "Uniform.h"
#include "glad.h"
#include <iostream>
#include <sstream>
#include <iomanip>
#include <cmath>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

Sample::Sample()
    : mTextureBar(nullptr),
      mTextureWoman(nullptr),
      mLBSShader(nullptr),
      mDQSShader(nullptr),
      mModelMode(VISUAL_BAR),
      mMotionMode(MOTION_TWIST),
      mIsPlaying(true),
      mAngleDeg(0.0f),
      mAngleSpeed(45.0f),
      mAngleDir(1.0f),
      mWomanPlaybackTime(0.0f),
      mWomanCenter(vec3(0, 2.65f, 0)),
      mWomanSize(vec3(1, 5.3f, 5)),
      mWomanCamDist(6.5f)
{
}

Sample::~Sample() {
    Shutdown();
}

void Sample::ComputeWomanBounds() {
    if (mMeshesWoman.empty()) return;
    vec3 minP(1e9f, 1e9f, 1e9f);
    vec3 maxP(-1e9f, -1e9f, -1e9f);

    for (const auto& mesh : mMeshesWoman) {
        const auto& pos = mesh.GetPosition();
        for (const auto& p : pos) {
            if (p.x < minP.x) minP.x = p.x;
            if (p.y < minP.y) minP.y = p.y;
            if (p.z < minP.z) minP.z = p.z;
            if (p.x > maxP.x) maxP.x = p.x;
            if (p.y > maxP.y) maxP.y = p.y;
            if (p.z > maxP.z) maxP.z = p.z;
        }
    }
    mWomanCenter = (minP + maxP) * 0.5f;
    mWomanSize = maxP - minP;

    // Fit vertical field of view (60 degrees) with 20% margin
    float halfFovRad = 30.0f * (float)M_PI / 180.0f;
    float maxDim = mWomanSize.y * 0.5f;
    mWomanCamDist = (maxDim / tanf(halfFovRad)) * 1.25f;
}

void Sample::Initialize() {
    // 1. Load Bar Asset
    cgltf_data* gltfBar = LoadGLTFFile("Assets/dq.gltf");
    if (gltfBar) {
        mMeshesBar = LoadMeshes(gltfBar);
        mSkeletonBar = LoadSkeleton(gltfBar);
        FreeGLTFFile(gltfBar);
    }
    mTextureBar = new Texture("Assets/dq.png");

    // 2. Load Woman Asset
    cgltf_data* gltfWoman = LoadGLTFFile("Assets/Woman.gltf");
    if (gltfWoman) {
        mMeshesWoman = LoadMeshes(gltfWoman);
        mSkeletonWoman = LoadSkeleton(gltfWoman);
        mClipsWoman = LoadAnimationClips(gltfWoman);
        FreeGLTFFile(gltfWoman);
    }
    mTextureWoman = new Texture("Assets/Woman.png");

    ComputeWomanBounds();

    // 3. Shaders
    mLBSShader = new Shader("Shaders/skinned.vert", "Shaders/lit.frag");
    mDQSShader = new Shader("Shaders/dualquaternion.vert", "Shaders/lit.frag");
}

Pose Sample::BuildBarPose(float deg, VisualMotion motion) {
    if (deg > 179.99f && motion == MOTION_TWIST) {
        deg = 179.99f;
    }
    Pose pose = mSkeletonBar.GetRestPose();
    float rad = (float)(deg * M_PI / 180.0);
    Transform local = pose.GetLocalTransform(0);

    if (motion == MOTION_TWIST) {
        // Twist rotates Bone 0 around its local Y axis (bar cylinder axis)
        local.rotation = local.rotation * angleAxis(-rad, vec3(0, 1, 0));
    } else {
        // Bend rotates Bone 0 around its local Z axis
        local.rotation = local.rotation * angleAxis(rad, vec3(0, 0, 1));
    }

    pose.SetLocalTransform(0, local);
    return pose;
}

void Sample::Update(float deltaTime) {
    if (mIsPlaying) {
        if (mModelMode == VISUAL_BAR) {
            float maxAngle = (mMotionMode == MOTION_TWIST) ? 180.0f : 120.0f;
            mAngleDeg += mAngleDir * mAngleSpeed * deltaTime;
            if (mAngleDeg >= maxAngle) {
                mAngleDeg = maxAngle;
                mAngleDir = -1.0f;
            } else if (mAngleDeg <= 0.0f) {
                mAngleDeg = 0.0f;
                mAngleDir = 1.0f;
            }
        } else {
            if (!mClipsWoman.empty()) {
                mWomanPlaybackTime += deltaTime;
                float duration = mClipsWoman[0].GetDuration();
                if (duration > 0.0f && mWomanPlaybackTime >= duration) {
                    mWomanPlaybackTime = fmodf(mWomanPlaybackTime, duration);
                }
            }
        }
    }
}

void Sample::SetBarTwist(float deg) {
    mModelMode = VISUAL_BAR;
    mMotionMode = MOTION_TWIST;
    mAngleDeg = deg;
    mIsPlaying = false;
}

void Sample::SetBarBend(float deg) {
    mModelMode = VISUAL_BAR;
    mMotionMode = MOTION_BEND;
    mAngleDeg = deg;
    mIsPlaying = false;
}

void Sample::SetWomanTime(float t) {
    mModelMode = VISUAL_WOMAN;
    mWomanPlaybackTime = t;
    mIsPlaying = false;
}

void Sample::SetModel(VisualModel model) {
    mModelMode = model;
}

void Sample::OnKeyDown(int key) {
    switch (key) {
    case 0x20: // Space
        mIsPlaying = !mIsPlaying;
        break;
    case 0x25: // Left arrow
        if (mModelMode == VISUAL_BAR) {
            mAngleDeg = fmaxf(0.0f, mAngleDeg - 5.0f);
        } else {
            mWomanPlaybackTime = fmaxf(0.0f, mWomanPlaybackTime - 0.05f);
        }
        break;
    case 0x27: // Right arrow
        if (mModelMode == VISUAL_BAR) {
            float maxAngle = (mMotionMode == MOTION_TWIST) ? 180.0f : 120.0f;
            mAngleDeg = fminf(maxAngle, mAngleDeg + 5.0f);
        } else {
            mWomanPlaybackTime += 0.05f;
        }
        break;
    case 'B':
    case 'b':
        mModelMode = (mModelMode == VISUAL_BAR) ? VISUAL_WOMAN : VISUAL_BAR;
        break;
    case 'T':
    case 't':
        if (mModelMode == VISUAL_BAR) {
            mMotionMode = (mMotionMode == MOTION_TWIST) ? MOTION_BEND : MOTION_TWIST;
            if (mAngleDeg > 120.0f && mMotionMode == MOTION_BEND) {
                mAngleDeg = 120.0f;
            }
        }
        break;
    }
}

std::string Sample::GetWindowTitle(float fps) const {
    std::ostringstream ss;
    ss << std::fixed << std::setprecision(1);
    ss << "LBS (Left) vs DQS (Right) | ";
    if (mModelMode == VISUAL_BAR) {
        ss << "Bar " << (mMotionMode == MOTION_TWIST ? "Twist: " : "Bend: ")
           << mAngleDeg << " deg";
    } else {
        ss << "Woman Walk: " << mWomanPlaybackTime << "s";
    }
    ss << (mIsPlaying ? " [Playing]" : " [Paused]");
    ss << " | FPS: " << fps;
    return ss.str();
}

void Sample::RenderSplit(int width, int height) {
    int halfW = width / 2;
    float aspect = (float)halfW / (float)height;

    mat4 projection = perspective(60.0f, aspect, 0.01f, 1000.0f);
    mat4 view;
    Transform model(vec3(0, 0, 0), quat(), vec3(1, 1, 1));

    std::vector<mat4> posePaletteLBS;
    std::vector<mat4> invBindLBS;
    std::vector<DualQuaternion> posePaletteDQS;
    std::vector<DualQuaternion> invBindDQS;

    std::vector<Mesh>* pMeshes = nullptr;
    Texture* pTexture = nullptr;

    if (mModelMode == VISUAL_BAR) {
        if (mMotionMode == MOTION_BEND) {
            // Perpendicular side-view along Z axis: directly displays X-Y bending profile & joint pinching
            view = lookAt(vec3(0.0f, 1.8f, 11.0f), vec3(0.0f, 1.2f, 0.0f), vec3(0.0f, 1.0f, 0.0f));
        } else {
            // 3/4 angle view for bar twist
            view = lookAt(vec3(7.5f, 4.5f, 9.0f), vec3(0.0f, 0.0f, 0.0f), vec3(0.0f, 1.0f, 0.0f));
        }
        pMeshes = &mMeshesBar;
        pTexture = mTextureBar;

        Pose pose = BuildBarPose(mAngleDeg, mMotionMode);
        pose.GetMatrixPalette(posePaletteLBS);
        invBindLBS = mSkeletonBar.GetInvBindPose();

        pose.GetDualQuaternionPalette(posePaletteDQS);
        mSkeletonBar.GetInvBindPose(invBindDQS);
    } else {
        // Full-body view for Woman
        vec3 eye = mWomanCenter + vec3(0.5f, 0.0f, mWomanCamDist);
        view = lookAt(eye, mWomanCenter, vec3(0.0f, 1.0f, 0.0f));
        pMeshes = &mMeshesWoman;
        pTexture = mTextureWoman;

        Pose pose = mSkeletonWoman.GetRestPose();
        if (!mClipsWoman.empty()) {
            mClipsWoman[0].Sample(pose, mWomanPlaybackTime);
        }

        pose.GetMatrixPalette(posePaletteLBS);
        invBindLBS = mSkeletonWoman.GetInvBindPose();

        // Woman DQS: precompute combined dual quaternions from the combined skinning matrix (pose * invBind).
        // In Woman.gltf, the Armature root node has scale [0.01, 0.01, 0.01] and invBind has scale 100.
        // Multiplying the matrices cancels the scale (0.01 * 100 = 1.0) into a pure rigid transform.
        posePaletteDQS.resize(posePaletteLBS.size());
        invBindDQS.resize(posePaletteLBS.size());
        for (size_t i = 0; i < posePaletteLBS.size(); ++i) {
            mat4 comb = posePaletteLBS[i] * invBindLBS[i];
            posePaletteDQS[i] = transformToDualQuat(mat4ToTransform(comb));
            invBindDQS[i] = DualQuaternion(); // Identity dual quaternion
        }
    }

    if (!pMeshes || pMeshes->empty()) return;

    // Enable depth test so polygons are properly z-buffered
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS);

    // Disable face culling so twisted/sheared triangles render cleanly without holes
    glDisable(GL_CULL_FACE);

    vec3 lightDir = normalized(vec3(0.5f, 0.8f, 0.7f));

    // Clear whole screen with dark background
    glViewport(0, 0, width, height);
    glClearColor(0.12f, 0.14f, 0.18f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // ==========================================
    // Left Viewport: Linear Blend Skinning (LBS)
    // ==========================================
    glViewport(0, 0, halfW - 1, height);
    mLBSShader->Bind();
    Uniform<mat4>::Set(mLBSShader->GetUniform("model"), transformToMat4(model));
    Uniform<mat4>::Set(mLBSShader->GetUniform("view"), view);
    Uniform<mat4>::Set(mLBSShader->GetUniform("projection"), projection);
    Uniform<vec3>::Set(mLBSShader->GetUniform("light"), lightDir);
    Uniform<mat4>::Set(mLBSShader->GetUniform("pose"), posePaletteLBS);
    Uniform<mat4>::Set(mLBSShader->GetUniform("invBindPose"), invBindLBS);

    if (pTexture) pTexture->Set(mLBSShader->GetUniform("tex0"), 0);
    for (auto& mesh : *pMeshes) {
        mesh.Bind(mLBSShader->GetAttribute("position"), mLBSShader->GetAttribute("normal"),
                  mLBSShader->GetAttribute("texCoord"), mLBSShader->GetAttribute("weights"),
                  mLBSShader->GetAttribute("joints"));
        mesh.Draw();
        mesh.UnBind(mLBSShader->GetAttribute("position"), mLBSShader->GetAttribute("normal"),
                    mLBSShader->GetAttribute("texCoord"), mLBSShader->GetAttribute("weights"),
                    mLBSShader->GetAttribute("joints"));
    }
    if (pTexture) pTexture->UnSet(0);
    mLBSShader->UnBind();

    // ==========================================
    // Right Viewport: Dual Quaternion Skinning (DQS)
    // ==========================================
    glViewport(halfW + 1, 0, width - halfW - 1, height);
    mDQSShader->Bind();
    Uniform<mat4>::Set(mDQSShader->GetUniform("model"), transformToMat4(model));
    Uniform<mat4>::Set(mDQSShader->GetUniform("view"), view);
    Uniform<mat4>::Set(mDQSShader->GetUniform("projection"), projection);
    Uniform<vec3>::Set(mDQSShader->GetUniform("light"), lightDir);
    Uniform<DualQuaternion>::Set(mDQSShader->GetUniform("pose"), posePaletteDQS);
    Uniform<DualQuaternion>::Set(mDQSShader->GetUniform("invBindPose"), invBindDQS);

    if (pTexture) pTexture->Set(mDQSShader->GetUniform("tex0"), 0);
    for (auto& mesh : *pMeshes) {
        mesh.Bind(mDQSShader->GetAttribute("position"), mDQSShader->GetAttribute("normal"),
                  mDQSShader->GetAttribute("texCoord"), mDQSShader->GetAttribute("weights"),
                  mDQSShader->GetAttribute("joints"));
        mesh.Draw();
        mesh.UnBind(mDQSShader->GetAttribute("position"), mDQSShader->GetAttribute("normal"),
                    mDQSShader->GetAttribute("texCoord"), mDQSShader->GetAttribute("weights"),
                    mDQSShader->GetAttribute("joints"));
    }
    if (pTexture) pTexture->UnSet(0);
    mDQSShader->UnBind();

    // Restore full viewport
    glViewport(0, 0, width, height);
}

void Sample::Render(float inAspectRatio) {
    // Standard interface fallback (calls split with default 1024x768 aspect)
    int h = 768;
    int w = (int)(h * inAspectRatio);
    RenderSplit(w, h);
}

void Sample::Shutdown() {
    delete mTextureBar;
    mTextureBar = nullptr;
    delete mTextureWoman;
    mTextureWoman = nullptr;

    delete mLBSShader;
    mLBSShader = nullptr;
    delete mDQSShader;
    mDQSShader = nullptr;

    mMeshesBar.clear();
    mMeshesWoman.clear();
    mClipsWoman.clear();
}

void Sample::DiagnoseWomanDQS() {
    std::cout << "\n=======================================================\n";
    std::cout << "           DIAGNOSTIC: WOMAN DQS vs LBS SKINNING        \n";
    std::cout << "=======================================================\n";

    if (mMeshesWoman.empty()) {
        std::cout << "[Diagnose] ERROR: Woman meshes empty!\n";
        return;
    }

    int numNodes = (int)mSkeletonWoman.GetRestPose().Size();
    std::cout << "[Diagnose] Skeleton nodes count: " << numNodes << "\n";

    // 1. Joint index range check in mesh influences
    int minJoint = 999999, maxJoint = -999999;
    size_t totalVerts = 0;
    float yMin = 1e9f, yMax = -1e9f;

    for (const auto& mesh : mMeshesWoman) {
        const auto& pos = mesh.GetPosition();
        const auto& j   = mesh.GetInfluences();
        const auto& w   = mesh.GetWeights();
        for (size_t i = 0; i < pos.size(); ++i) {
            totalVerts++;
            if (pos[i].y < yMin) yMin = pos[i].y;
            if (pos[i].y > yMax) yMax = pos[i].y;

            if (j[i].x < minJoint) minJoint = j[i].x;
            if (j[i].x > maxJoint) maxJoint = j[i].x;
            if (j[i].y < minJoint) minJoint = j[i].y;
            if (j[i].y > maxJoint) maxJoint = j[i].y;
            if (j[i].z < minJoint) minJoint = j[i].z;
            if (j[i].z > maxJoint) maxJoint = j[i].z;
            if (j[i].w < minJoint) minJoint = j[i].w;
            if (j[i].w > maxJoint) maxJoint = j[i].w;
        }
    }
    float charHeight = yMax - yMin;
    std::cout << "[Diagnose] Total vertices: " << totalVerts << "\n";
    std::cout << "[Diagnose] Rest character height: " << charHeight << " units (y: " << yMin << " to " << yMax << ")\n";
    std::cout << "[Diagnose] Joint index range in mesh: [" << minJoint << ", " << maxJoint << "]\n";

    // Walking pose at t = 0.5s
    Pose pose = mSkeletonWoman.GetRestPose();
    if (!mClipsWoman.empty()) {
        mClipsWoman[0].Sample(pose, 0.5f);
    }

    // LBS Palette
    std::vector<mat4> lbsPosePalette;
    pose.GetMatrixPalette(lbsPosePalette);
    std::vector<mat4> lbsInvBind = mSkeletonWoman.GetInvBindPose();

    // DQS Palette
    std::vector<DualQuaternion> dqsPosePalette;
    pose.GetDualQuaternionPalette(dqsPosePalette);
    std::vector<DualQuaternion> dqsInvBind;
    mSkeletonWoman.GetInvBindPose(dqsInvBind);

    std::cout << "[Diagnose] LBS posePalette size: " << lbsPosePalette.size()
              << ", invBind size: " << lbsInvBind.size() << "\n";
    std::cout << "[Diagnose] DQS posePalette size: " << dqsPosePalette.size()
              << ", invBind size: " << dqsInvBind.size() << "\n";

    // Helper: combineDQ
    auto combineDQ = [](const DualQuaternion& inv, const DualQuaternion& p) -> DualQuaternion {
        DualQuaternion l = normalized(inv);
        DualQuaternion r = normalized(p);
        quat real = l.real * r.real;
        quat dual = l.real * r.dual + l.dual * r.real;
        return DualQuaternion(real, dual);
    };

    // Test Method A: hemisphere on pose (naive dualquaternion.vert style)
    // Test Method B: hemisphere on combined DQ (dqs_pre style)
    double maxDistA = 0.0, sumDistA = 0.0;
    int nanCountA = 0, infCountA = 0, hugeCountA = 0;

    double maxDistB = 0.0, sumDistB = 0.0;
    int nanCountB = 0, infCountB = 0, hugeCountB = 0;

    double maxDistC = 0.0, sumDistC = 0.0;

    for (const auto& mesh : mMeshesWoman) {
        const auto& positions  = mesh.GetPosition();
        const auto& joints     = mesh.GetInfluences();
        const auto& weights    = mesh.GetWeights();

        for (size_t i = 0; i < positions.size(); ++i) {
            const vec3& p = positions[i];
            const ivec4& j = joints[i];
            const vec4& w = weights[i];

            // CPU LBS
            mat4 skinLBS = (lbsPosePalette[j.x] * lbsInvBind[j.x]) * w.x
                         + (lbsPosePalette[j.y] * lbsInvBind[j.y]) * w.y
                         + (lbsPosePalette[j.z] * lbsInvBind[j.z]) * w.z
                         + (lbsPosePalette[j.w] * lbsInvBind[j.w]) * w.w;
            vec3 pLBS = transformPoint(skinLBS, p);

            // CPU DQS Method A (hemisphere check on pose - naive dualquaternion.vert)
            {
                vec4 wA = w;
                if (dot(dqsPosePalette[j.x].real, dqsPosePalette[j.y].real) < 0.0f) wA.y *= -1.0f;
                if (dot(dqsPosePalette[j.x].real, dqsPosePalette[j.z].real) < 0.0f) wA.z *= -1.0f;
                if (dot(dqsPosePalette[j.x].real, dqsPosePalette[j.w].real) < 0.0f) wA.w *= -1.0f;

                DualQuaternion dq0 = combineDQ(dqsInvBind[j.x], dqsPosePalette[j.x]);
                DualQuaternion dq1 = combineDQ(dqsInvBind[j.y], dqsPosePalette[j.y]);
                DualQuaternion dq2 = combineDQ(dqsInvBind[j.z], dqsPosePalette[j.z]);
                DualQuaternion dq3 = combineDQ(dqsInvBind[j.w], dqsPosePalette[j.w]);

                DualQuaternion skinA = dq0 * wA.x + dq1 * wA.y + dq2 * wA.z + dq3 * wA.w;
                float magA = len(skinA.real);
                if (std::isnan(magA)) nanCountA++;
                else if (magA < 1e-6f) infCountA++;
                else {
                    normalize(skinA);
                    vec3 pA = transformPoint(skinA, p);
                    double d = len(pA - pLBS);
                    if (d > 100.0) hugeCountA++;
                    if (d > maxDistA) maxDistA = d;
                    sumDistA += d;
                }
            }

            // CPU DQS Method B (hemisphere check on combined DQ - dqs_pre style)
            {
                DualQuaternion dq0 = combineDQ(dqsInvBind[j.x], dqsPosePalette[j.x]);
                DualQuaternion dq1 = combineDQ(dqsInvBind[j.y], dqsPosePalette[j.y]);
                DualQuaternion dq2 = combineDQ(dqsInvBind[j.z], dqsPosePalette[j.z]);
                DualQuaternion dq3 = combineDQ(dqsInvBind[j.w], dqsPosePalette[j.w]);

                vec4 wB = w;
                if (dot(dq0.real, dq1.real) < 0.0f) wB.y *= -1.0f;
                if (dot(dq0.real, dq2.real) < 0.0f) wB.z *= -1.0f;
                if (dot(dq0.real, dq3.real) < 0.0f) wB.w *= -1.0f;

                DualQuaternion skinB = dq0 * wB.x + dq1 * wB.y + dq2 * wB.z + dq3 * wB.w;
                float magB = len(skinB.real);
                if (std::isnan(magB)) nanCountB++;
                else if (magB < 1e-6f) infCountB++;
                else {
                    normalize(skinB);
                    vec3 pB = transformPoint(skinB, p);
                    double d = len(pB - pLBS);
                    if (d > 100.0) hugeCountB++;
                    if (d > maxDistB) maxDistB = d;
                    sumDistB += d;
                }
            }

            // CPU DQS Method C: convert combined matrix (pose * invBind) to DualQuaternion
            // Since pose * invBind has net scale 1.0 (0.01 * 100 cancels out)
            {
                mat4 m0 = lbsPosePalette[j.x] * lbsInvBind[j.x];
                mat4 m1 = lbsPosePalette[j.y] * lbsInvBind[j.y];
                mat4 m2 = lbsPosePalette[j.z] * lbsInvBind[j.z];
                mat4 m3 = lbsPosePalette[j.w] * lbsInvBind[j.w];

                DualQuaternion dq0 = transformToDualQuat(mat4ToTransform(m0));
                DualQuaternion dq1 = transformToDualQuat(mat4ToTransform(m1));
                DualQuaternion dq2 = transformToDualQuat(mat4ToTransform(m2));
                DualQuaternion dq3 = transformToDualQuat(mat4ToTransform(m3));

                vec4 wC = w;
                if (dot(dq0.real, dq1.real) < 0.0f) wC.y *= -1.0f;
                if (dot(dq0.real, dq2.real) < 0.0f) wC.z *= -1.0f;
                if (dot(dq0.real, dq3.real) < 0.0f) wC.w *= -1.0f;

                DualQuaternion skinC = dq0 * wC.x + dq1 * wC.y + dq2 * wC.z + dq3 * wC.w;
                normalize(skinC);
                vec3 pC = transformPoint(skinC, p);
                double d = len(pC - pLBS);
                if (d > maxDistC) maxDistC = d;
                sumDistC += d;
            }
        }
    }

    std::cout << std::fixed << std::setprecision(5);
    std::cout << "[Diagnose] Method A (hemisphere on pose): NaN=" << nanCountA
              << ", Inf=" << infCountA << ", Huge(>100u)=" << hugeCountA
              << ", maxDist=" << maxDistA << " (" << (maxDistA / charHeight)*100.0 << "% of height)"
              << ", meanDist=" << (sumDistA / totalVerts) << " (" << (sumDistA / totalVerts / charHeight)*100.0 << "% of height)\n";

    std::cout << "[Diagnose] Method B (hemisphere on combined DQ): NaN=" << nanCountB
              << ", Inf=" << infCountB << ", Huge(>100u)=" << hugeCountB
              << ", maxDist=" << maxDistB << " (" << (maxDistB / charHeight)*100.0 << "% of height)"
              << ", meanDist=" << (sumDistB / totalVerts) << " (" << (sumDistB / totalVerts / charHeight)*100.0 << "% of height)\n";

    std::cout << "[Diagnose] Method C (combined animated M_i -> DQ): "
              << "maxDist=" << maxDistC << " (" << (maxDistC / charHeight)*100.0 << "% of height)"
              << ", meanDist=" << (sumDistC / totalVerts) << " (" << (sumDistC / totalVerts / charHeight)*100.0 << "% of height)\n";
    std::cout << "=======================================================\n\n";
}
