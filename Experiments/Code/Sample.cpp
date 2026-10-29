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
        // 3/4 angle view for bar
        view = lookAt(vec3(7.5f, 4.5f, 9.0f), vec3(0.0f, 0.0f, 0.0f), vec3(0.0f, 1.0f, 0.0f));
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

        pose.GetDualQuaternionPalette(posePaletteDQS);
        mSkeletonWoman.GetInvBindPose(invBindDQS);
    }

    if (!pMeshes || pMeshes->empty()) return;

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
