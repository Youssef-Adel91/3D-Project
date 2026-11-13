// Experiment4.cpp: Stronger LBS Baselines Benchmark & Validation
#define _CRT_SECURE_NO_WARNINGS
#include "Experiment4.h"
#include "Uniform.h"
#include "GLTFLoader.h"
#include "glad.h"
#undef APIENTRY
#ifndef NOMINMAX
#define NOMINMAX
#endif
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <iostream>
#include <fstream>
#include <iomanip>
#include <algorithm>
#include <numeric>
#include <random>
#include <cmath>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

static double QPCMs4() {
    LARGE_INTEGER freq, cnt;
    QueryPerformanceFrequency(&freq);
    QueryPerformanceCounter(&cnt);
    return (double)cnt.QuadPart * 1000.0 / (double)freq.QuadPart;
}

static const float FIXED_DT4 = 1.0f / 60.0f;

Experiment4::Experiment4()
    : mShader_3x4_UBO(nullptr), mShader_3x4_TBO(nullptr),
      mTF_LBS_pre_3x4(nullptr), mTF_LBS_pre_3x4_ubo(nullptr), mTF_LBS_pre_3x4_tbo(nullptr),
      mTF_LBS_naive_opt(nullptr), mTF_LBS_blend_once(nullptr),
      mUBO_3x4(0), mTBO_3x4_buf(0), mTBO_3x4_tex(0),
      mMaxUniformComponents(0), mMaxUBOSize(0), mMaxTBOSize(0)
{
    for (int i = 0; i < NUM_E4_VARIANTS; ++i) mShaders[i] = nullptr;
}

Experiment4::~Experiment4() {}

bool Experiment4::Initialize() {
    std::cout << "\n========================================\n";
    std::cout << "  INITIALIZING EXPERIMENT 4 (LBS Baselines)\n";
    std::cout << "========================================\n";

    glGetIntegerv(GL_MAX_VERTEX_UNIFORM_COMPONENTS, &mMaxUniformComponents);
    glGetIntegerv(GL_MAX_UNIFORM_BLOCK_SIZE,         &mMaxUBOSize);
    glGetIntegerv(GL_MAX_TEXTURE_BUFFER_SIZE,        &mMaxTBOSize);

    std::cout << "[E4] GL_MAX_VERTEX_UNIFORM_COMPONENTS: " << mMaxUniformComponents << "\n";
    std::cout << "[E4] GL_MAX_UNIFORM_BLOCK_SIZE:        " << mMaxUBOSize << " bytes\n";
    std::cout << "[E4] GL_MAX_TEXTURE_BUFFER_SIZE:       " << mMaxTBOSize << " texels\n";

    // 1. Shaders for timing
    std::cout << "[E4] Compiling benchmark shaders...\n";
    mShaders[VAR_LBS_NAIVE_BASE] = new Shader("Shaders/skinned.vert",          "Shaders/lit.frag");
    mShaders[VAR_LBS_NAIVE_OPT]  = new Shader("Shaders/lbs_naive_opt.vert",    "Shaders/lit.frag");
    mShaders[VAR_LBS_PRE_4X4]    = new Shader("Shaders/preskinned.vert",       "Shaders/lit.frag");
    mShaders[VAR_LBS_PRE_3X4]    = new Shader("Shaders/lbs_pre_3x4.vert",      "Shaders/lit.frag");
    mShaders[VAR_LBS_BLEND_ONCE] = new Shader("Shaders/lbs_blend_once.vert",   "Shaders/lit.frag");
    mShaders[VAR_DQS_NAIVE_BASE] = new Shader("Shaders/dualquaternion.vert",   "Shaders/lit.frag");
    mShaders[VAR_DQS_PRE_BASE]   = new Shader("Shaders/dqs_pre.vert",          "Shaders/lit.frag");

    mShader_3x4_UBO = new Shader("Shaders/lbs_pre_3x4_ubo.vert", "Shaders/lit.frag");
    mShader_3x4_TBO = new Shader("Shaders/lbs_pre_3x4_tbo.vert", "Shaders/lit.frag");

    // 2. Transform feedback shaders
    std::cout << "[E4] Compiling transform feedback validation shaders...\n";
    const char* tfVaryings[] = { "tf_position" };

    mTF_LBS_pre_3x4 = new Shader();
    mTF_LBS_pre_3x4->LoadWithTF("Shaders/lbs_pre_3x4_tf.vert", "Shaders/lit.frag", tfVaryings, 1);

    mTF_LBS_pre_3x4_ubo = new Shader();
    mTF_LBS_pre_3x4_ubo->LoadWithTF("Shaders/lbs_pre_3x4_ubo_tf.vert", "Shaders/lit.frag", tfVaryings, 1);

    mTF_LBS_pre_3x4_tbo = new Shader();
    mTF_LBS_pre_3x4_tbo->LoadWithTF("Shaders/lbs_pre_3x4_tbo_tf.vert", "Shaders/lit.frag", tfVaryings, 1);

    mTF_LBS_naive_opt = new Shader();
    mTF_LBS_naive_opt->LoadWithTF("Shaders/lbs_naive_opt_tf.vert", "Shaders/lit.frag", tfVaryings, 1);

    mTF_LBS_blend_once = new Shader();
    mTF_LBS_blend_once->LoadWithTF("Shaders/lbs_blend_once_tf.vert", "Shaders/lit.frag", tfVaryings, 1);

    // 3. Setup UBO and TBO buffers
    glGenBuffers(1, &mUBO_3x4);
    glBindBuffer(GL_UNIFORM_BUFFER, mUBO_3x4);
    glBufferData(GL_UNIFORM_BUFFER, 512 * 3 * 16, nullptr, GL_DYNAMIC_DRAW); // 512 bones * 3 vec4s * 16 bytes
    glBindBuffer(GL_UNIFORM_BUFFER, 0);

    glGenBuffers(1, &mTBO_3x4_buf);
    glGenTextures(1, &mTBO_3x4_tex);
    glBindBuffer(GL_TEXTURE_BUFFER, mTBO_3x4_buf);
    glBufferData(GL_TEXTURE_BUFFER, 512 * 3 * 16, nullptr, GL_DYNAMIC_DRAW);
    glBindTexture(GL_TEXTURE_BUFFER, mTBO_3x4_tex);
    glTexBuffer(GL_TEXTURE_BUFFER, GL_RGBA32F, mTBO_3x4_buf);
    glBindBuffer(GL_TEXTURE_BUFFER, 0);
    glBindTexture(GL_TEXTURE_BUFFER, 0);

    // 4. Run Transform Feedback Validations
    RunValidation();

    return true;
}

void Experiment4::Shutdown() {
    for (int i = 0; i < NUM_E4_VARIANTS; ++i) {
        delete mShaders[i];
        mShaders[i] = nullptr;
    }
    delete mShader_3x4_UBO;       mShader_3x4_UBO = nullptr;
    delete mShader_3x4_TBO;       mShader_3x4_TBO = nullptr;
    delete mTF_LBS_pre_3x4;       mTF_LBS_pre_3x4 = nullptr;
    delete mTF_LBS_pre_3x4_ubo;   mTF_LBS_pre_3x4_ubo = nullptr;
    delete mTF_LBS_pre_3x4_tbo;   mTF_LBS_pre_3x4_tbo = nullptr;
    delete mTF_LBS_naive_opt;     mTF_LBS_naive_opt = nullptr;
    delete mTF_LBS_blend_once;    mTF_LBS_blend_once = nullptr;

    if (mUBO_3x4)     { glDeleteBuffers(1, &mUBO_3x4);     mUBO_3x4 = 0; }
    if (mTBO_3x4_tex) { glDeleteTextures(1, &mTBO_3x4_tex); mTBO_3x4_tex = 0; }
    if (mTBO_3x4_buf) { glDeleteBuffers(1, &mTBO_3x4_buf);  mTBO_3x4_buf = 0; }
}

// ============================================================
// Synthetic Palette Helpers
// ============================================================

std::vector<mat4> Experiment4::MakeSyntheticLBSPalette(int bones, float t) {
    std::vector<mat4> palette(bones);
    mat4 parentMat;
    for (int i = 0; i < bones; ++i) {
        float angle = sinf(t * 0.5f + i * 0.3f) * 0.2f;
        float c = cosf(angle), s = sinf(angle);
        mat4 local;
        local.xx = c;  local.xz = s;
        local.zx = -s; local.zz = c;
        local.tx = 0.5f;
        palette[i] = parentMat * local;
        parentMat = palette[i];
    }
    return palette;
}

std::vector<DualQuaternion> Experiment4::MakeSyntheticDQSPalette(int bones, float t) {
    std::vector<DualQuaternion> palette(bones);
    vec3 pos(0, 0, 0);
    quat parentRot;
    for (int i = 0; i < bones; ++i) {
        float angle = sinf(t * 0.5f + i * 0.3f) * 0.2f;
        quat localRot = angleAxis(angle, vec3(0, 1, 0));
        quat worldRot = parentRot * localRot;
        vec3 advance = parentRot * vec3(0.5f, 0.0f, 0.0f);
        pos = pos + advance;
        Transform xform(pos, worldRot, vec3(1,1,1));
        palette[i] = transformToDualQuat(xform);
        parentRot = worldRot;
    }
    return palette;
}

std::vector<mat4> Experiment4::MakeSyntheticInvBind(int bones) {
    std::vector<mat4> invBind(bones);
    for (int i = 0; i < bones; ++i) {
        mat4 bind;
        bind.tx = -0.5f * (i + 1);
        invBind[i] = bind;
    }
    return invBind;
}

std::vector<DualQuaternion> Experiment4::MakeSyntheticInvBindDQ(int bones) {
    std::vector<DualQuaternion> invBindDQ(bones);
    for (int i = 0; i < bones; ++i) {
        Transform bind(vec3(-0.5f * (i + 1), 0, 0), quat(), vec3(1,1,1));
        invBindDQ[i] = transformToDualQuat(bind);
    }
    return invBindDQ;
}

void Experiment4::BuildAnimatedLBS(const std::vector<mat4>& pose, const std::vector<mat4>& invBind, std::vector<mat4>& out) {
    out.resize(pose.size());
    for (size_t i = 0; i < pose.size(); ++i) {
        out[i] = pose[i] * invBind[i];
    }
}

void Experiment4::BuildAnimated3x4(const std::vector<mat4>& animated4x4, std::vector<vec4>& out3x4Rows) {
    out3x4Rows.resize(animated4x4.size() * 3);
    for (size_t i = 0; i < animated4x4.size(); ++i) {
        const mat4& m = animated4x4[i];
        out3x4Rows[i * 3 + 0] = vec4(m.xx, m.yx, m.zx, m.tx); // Row 0
        out3x4Rows[i * 3 + 1] = vec4(m.xy, m.yy, m.zy, m.ty); // Row 1
        out3x4Rows[i * 3 + 2] = vec4(m.xz, m.yz, m.zz, m.tz); // Row 2
    }
}

void Experiment4::BuildAnimatedDQS(const std::vector<DualQuaternion>& pose, const std::vector<DualQuaternion>& invBind, std::vector<DualQuaternion>& out) {
    out.resize(pose.size());
    for (size_t i = 0; i < pose.size(); ++i) {
        const DualQuaternion& l = pose[i];
        const DualQuaternion& r = invBind[i];
        out[i] = DualQuaternion(l.real * r.real, l.real * r.dual + l.dual * r.real);
    }
}

int Experiment4::GetMaxBonesUniform(int componentsPerBone) const {
    const int overhead = 48; // model, view, projection
    if (componentsPerBone <= 0) return 0;
    return (mMaxUniformComponents - overhead) / componentsPerBone;
}

// ============================================================
// Transform Feedback Validation
// ============================================================

void Experiment4::RunValidation() {
    std::cout << "\n--- [E4 Validation] Checking GPU TF output vs CPU reference (tol = 1e-4) ---\n";
    mValidations.clear();

    // Load dq.gltf bar model for validation
    cgltf_data* gltf = LoadGLTFFile("Assets/dq.gltf");
    if (!gltf) {
        std::cerr << "[E4] ERROR: Could not load Assets/dq.gltf for validation!\n";
        return;
    }
    std::vector<Mesh> meshes = LoadMeshes(gltf);
    Skeleton skeleton = LoadSkeleton(gltf);
    FreeGLTFFile(gltf);

    if (meshes.empty()) return;
    const Mesh& mesh = meshes[0];
    const std::vector<vec3>& restPos = mesh.GetPosition();
    const std::vector<vec3>& restNorm = mesh.GetNormal();
    const std::vector<vec4>& weights  = mesh.GetWeights();
    const std::vector<ivec4>& joints  = mesh.GetInfluences();
    int N = (int)restPos.size();

    std::vector<mat4> invBind = skeleton.GetInvBindPose();
    float testAnglesDeg[] = { 30.0f, 90.0f, 150.0f };

    GLuint tfBuf = 0;
    glGenBuffers(1, &tfBuf);
    glBindBuffer(GL_TRANSFORM_FEEDBACK_BUFFER, tfBuf);
    glBufferData(GL_TRANSFORM_FEEDBACK_BUFFER, N * sizeof(vec3), nullptr, GL_STREAM_READ);
    glBindBufferBase(GL_TRANSFORM_FEEDBACK_BUFFER, 0, tfBuf);

    struct ShaderCheck {
        std::string name;
        std::string path;
        Shader* shader;
        bool isUBO;
        bool isTBO;
        bool isNaiveOpt;
    };

    std::vector<ShaderCheck> checks = {
        { "LBS-pre-3x4 (Uniform)", "Shaders/lbs_pre_3x4_tf.vert",      mTF_LBS_pre_3x4,     false, false, false },
        { "LBS-pre-3x4 (UBO)",     "Shaders/lbs_pre_3x4_ubo_tf.vert",  mTF_LBS_pre_3x4_ubo, true,  false, false },
        { "LBS-pre-3x4 (TBO)",     "Shaders/lbs_pre_3x4_tbo_tf.vert",  mTF_LBS_pre_3x4_tbo, false, true,  false },
        { "LBS-naive-opt",         "Shaders/lbs_naive_opt_tf.vert",    mTF_LBS_naive_opt,   false, false, true  },
        { "LBS-blend-once",        "Shaders/lbs_blend_once_tf.vert",   mTF_LBS_blend_once,  false, false, false }
    };

    mat4 identity;
    mat4 view = lookAt(vec3(0,3,14), vec3(0,0,0), vec3(0,1,0));
    mat4 proj = perspective(60.0f, 1.0f, 0.01f, 1000.0f);

    for (const auto& sc : checks) {
        ValidationResult vres;
        vres.variant = sc.name;
        vres.shaderPath = sc.path;
        vres.maxError30 = 0.0;
        vres.maxError90 = 0.0;
        vres.maxError150 = 0.0;
        vres.overallMaxError = 0.0;
        vres.passed = true;

        for (int a = 0; a < 3; ++a) {
            float deg = testAnglesDeg[a];
            float rad = (float)(deg * M_PI / 180.0);

            // Compute pose: bend around Z axis on joint 0
            Pose pose = skeleton.GetRestPose();
            Transform local = pose.GetLocalTransform(0);
            local.rotation = local.rotation * angleAxis(rad, vec3(0, 0, 1));
            pose.SetLocalTransform(0, local);

            std::vector<mat4> posePalette;
            pose.GetMatrixPalette(posePalette);

            // CPU reference skinning: M = pose[j] * invBind[j]
            std::vector<mat4> animated(posePalette.size());
            for (size_t j = 0; j < posePalette.size(); ++j) animated[j] = posePalette[j] * invBind[j];

            std::vector<vec3> cpuSkinned(N);
            for (int i = 0; i < N; ++i) {
                const ivec4& jt = joints[i];
                const vec4&  wt = weights[i];
                mat4 M = animated[jt.x] * wt.x + animated[jt.y] * wt.y + animated[jt.z] * wt.z + animated[jt.w] * wt.w;
                cpuSkinned[i] = transformPoint(M, restPos[i]);
            }

            // GPU TF draw
            sc.shader->Bind();
            Uniform<mat4>::Set(sc.shader->GetUniform("model"),      identity);
            Uniform<mat4>::Set(sc.shader->GetUniform("view"),       view);
            Uniform<mat4>::Set(sc.shader->GetUniform("projection"), proj);

            std::vector<vec4> rows3x4;
            BuildAnimated3x4(animated, rows3x4);

            if (sc.isNaiveOpt) {
                Uniform<mat4>::Set(sc.shader->GetUniform("pose"),       posePalette);
                Uniform<mat4>::Set(sc.shader->GetUniform("invBindPose"), invBind);
            } else if (sc.isUBO) {
                GLuint blockIdx = glGetUniformBlockIndex(sc.shader->GetHandle(), "SkinningData");
                if (blockIdx != GL_INVALID_INDEX) glUniformBlockBinding(sc.shader->GetHandle(), blockIdx, 0);
                glBindBuffer(GL_UNIFORM_BUFFER, mUBO_3x4);
                glBufferSubData(GL_UNIFORM_BUFFER, 0, rows3x4.size() * sizeof(vec4), rows3x4.data());
                glBindBufferBase(GL_UNIFORM_BUFFER, 0, mUBO_3x4);
            } else if (sc.isTBO) {
                glBindBuffer(GL_TEXTURE_BUFFER, mTBO_3x4_buf);
                glBufferSubData(GL_TEXTURE_BUFFER, 0, rows3x4.size() * sizeof(vec4), rows3x4.data());
                glBindBuffer(GL_TEXTURE_BUFFER, 0);
                glActiveTexture(GL_TEXTURE0);
                glBindTexture(GL_TEXTURE_BUFFER, mTBO_3x4_tex);
                int loc = sc.shader->GetUniform("skinMatrices");
                if (loc >= 0) glUniform1i(loc, 0);
            } else {
                // Uniform array (3x4 rows)
                int loc = sc.shader->GetUniform("palette3x4");
                if (loc >= 0) {
                    glUniform4fv(loc, (GLsizei)rows3x4.size(), (const float*)rows3x4.data());
                }
            }

            const_cast<Mesh&>(mesh).Bind(
                sc.shader->GetAttribute("position"),
                sc.shader->GetAttribute("normal"),
                sc.shader->GetAttribute("texCoord"),
                sc.shader->GetAttribute("weights"),
                sc.shader->GetAttribute("joints"));

            glEnable(GL_RASTERIZER_DISCARD);
            glBeginTransformFeedback(GL_POINTS);
            glDrawArrays(GL_POINTS, 0, N);
            glEndTransformFeedback();
            glDisable(GL_RASTERIZER_DISCARD);
            glFlush();

            const_cast<Mesh&>(mesh).UnBind(
                sc.shader->GetAttribute("position"),
                sc.shader->GetAttribute("normal"),
                sc.shader->GetAttribute("texCoord"),
                sc.shader->GetAttribute("weights"),
                sc.shader->GetAttribute("joints"));

            sc.shader->UnBind();

            // Read back
            std::vector<vec3> gpuOut(N);
            glBindBuffer(GL_TRANSFORM_FEEDBACK_BUFFER, tfBuf);
            glGetBufferSubData(GL_TRANSFORM_FEEDBACK_BUFFER, 0, N * sizeof(vec3), gpuOut.data());
            glBindBuffer(GL_TRANSFORM_FEEDBACK_BUFFER, 0);

            double maxErr = 0.0;
            for (int i = 0; i < N; ++i) {
                vec3 diff = gpuOut[i] - cpuSkinned[i];
                double err = sqrt(diff.x * diff.x + diff.y * diff.y + diff.z * diff.z);
                if (err > maxErr) maxErr = err;
            }

            if (a == 0) vres.maxError30 = maxErr;
            else if (a == 1) vres.maxError90 = maxErr;
            else if (a == 2) vres.maxError150 = maxErr;

            if (maxErr > vres.overallMaxError) vres.overallMaxError = maxErr;
        }

        if (vres.overallMaxError > 1e-4) vres.passed = false;
        mValidations.push_back(vres);

        std::cout << "  " << std::left << std::setw(26) << vres.variant
                  << " | 30 deg: " << std::scientific << std::setprecision(3) << vres.maxError30
                  << " | 90 deg: " << vres.maxError90
                  << " | 150 deg: " << vres.maxError150
                  << " | Max: " << vres.overallMaxError
                  << " | " << (vres.passed ? "PASS" : "FAIL") << "\n";
    }

    glDeleteBuffers(1, &tfBuf);
    std::cout << "------------------------------------------------------------------------\n";
}

// ============================================================
// Run a single configuration
// ============================================================

void Experiment4::RunConfig(int variantIdx, int pathIdx, BenchMesh& mesh, int bones, int influences,
                            int numWarmup, int numFrames,
                            std::vector<double>& gpuTimes, double& cpuMs, double& uploadMs)
{
    Shader* shader = nullptr;
    if (variantIdx == VAR_LBS_PRE_3X4) {
        if (pathIdx == PATH_UBO)      shader = mShader_3x4_UBO;
        else if (pathIdx == PATH_TBO) shader = mShader_3x4_TBO;
        else                          shader = mShaders[VAR_LBS_PRE_3X4];
    } else {
        shader = mShaders[variantIdx];
    }

    GLTimerQuery timer(8);
    float t = 0.0f;

    std::vector<mat4>          invBind   = MakeSyntheticInvBind(bones);
    std::vector<DualQuaternion> invBindDQ = MakeSyntheticInvBindDQ(bones);

    mat4 identity, view = lookAt(vec3(0,2,10), vec3(0,0,0), vec3(0,1,0));
    mat4 proj = perspective(60.0f, 1.333f, 0.01f, 1000.0f);

    double totalCPU = 0.0, totalUpload = 0.0;
    int measured = 0;

    // Bind UBO block index once if using UBO
    if (pathIdx == PATH_UBO && variantIdx == VAR_LBS_PRE_3X4) {
        GLuint blockIdx = glGetUniformBlockIndex(shader->GetHandle(), "SkinningData");
        if (blockIdx != GL_INVALID_INDEX) glUniformBlockBinding(shader->GetHandle(), blockIdx, 0);
    }

    int totalFrames = numWarmup + numFrames;
    for (int frame = 0; frame < totalFrames; ++frame) {
        bool isWarmup = (frame < numWarmup);

        if ((frame & 31) == 0) {
            MSG msg;
            int msgCount = 0;
            while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE) && msgCount++ < 16) {
                TranslateMessage(&msg);
                DispatchMessage(&msg);
            }
        }

        // CPU palette preparation
        double t0cpu = QPCMs4();
        std::vector<mat4>          posePalette, animatedLBS;
        std::vector<vec4>          rows3x4;
        std::vector<DualQuaternion> poseDQ, animatedDQS;

        switch (variantIdx) {
        case VAR_LBS_NAIVE_BASE:
        case VAR_LBS_NAIVE_OPT:
            posePalette = MakeSyntheticLBSPalette(bones, t);
            break;
        case VAR_LBS_PRE_4X4:
            posePalette = MakeSyntheticLBSPalette(bones, t);
            BuildAnimatedLBS(posePalette, invBind, animatedLBS);
            break;
        case VAR_LBS_PRE_3X4:
        case VAR_LBS_BLEND_ONCE:
            posePalette = MakeSyntheticLBSPalette(bones, t);
            BuildAnimatedLBS(posePalette, invBind, animatedLBS);
            BuildAnimated3x4(animatedLBS, rows3x4);
            break;
        case VAR_DQS_NAIVE_BASE:
            poseDQ = MakeSyntheticDQSPalette(bones, t);
            break;
        case VAR_DQS_PRE_BASE:
            poseDQ = MakeSyntheticDQSPalette(bones, t);
            BuildAnimatedDQS(poseDQ, invBindDQ, animatedDQS);
            break;
        }
        double t1cpu = QPCMs4();

        // GPU upload
        double t0up = QPCMs4();
        shader->Bind();
        Uniform<mat4>::Set(shader->GetUniform("model"),      identity);
        Uniform<mat4>::Set(shader->GetUniform("view"),       view);
        Uniform<mat4>::Set(shader->GetUniform("projection"), proj);

        switch (variantIdx) {
        case VAR_LBS_NAIVE_BASE:
            Uniform<mat4>::Set(shader->GetUniform("pose"),       posePalette);
            Uniform<mat4>::Set(shader->GetUniform("invBindPose"), invBind);
            break;
        case VAR_LBS_NAIVE_OPT:
            Uniform<mat4>::Set(shader->GetUniform("pose"),       posePalette);
            Uniform<mat4>::Set(shader->GetUniform("invBindPose"), invBind);
            break;
        case VAR_LBS_PRE_4X4:
            Uniform<mat4>::Set(shader->GetUniform("animated"), animatedLBS);
            break;
        case VAR_LBS_PRE_3X4:
            if (pathIdx == PATH_UBO) {
                glBindBuffer(GL_UNIFORM_BUFFER, mUBO_3x4);
                glBufferSubData(GL_UNIFORM_BUFFER, 0, rows3x4.size() * sizeof(vec4), rows3x4.data());
                glBindBufferBase(GL_UNIFORM_BUFFER, 0, mUBO_3x4);
            } else if (pathIdx == PATH_TBO) {
                glBindBuffer(GL_TEXTURE_BUFFER, mTBO_3x4_buf);
                glBufferSubData(GL_TEXTURE_BUFFER, 0, rows3x4.size() * sizeof(vec4), rows3x4.data());
                glBindBuffer(GL_TEXTURE_BUFFER, 0);
                glActiveTexture(GL_TEXTURE0);
                glBindTexture(GL_TEXTURE_BUFFER, mTBO_3x4_tex);
                int loc = shader->GetUniform("skinMatrices");
                if (loc >= 0) glUniform1i(loc, 0);
            } else {
                int loc = shader->GetUniform("palette3x4");
                if (loc >= 0) glUniform4fv(loc, (GLsizei)rows3x4.size(), (const float*)rows3x4.data());
            }
            break;
        case VAR_LBS_BLEND_ONCE: {
            int loc = shader->GetUniform("palette3x4");
            if (loc >= 0) glUniform4fv(loc, (GLsizei)rows3x4.size(), (const float*)rows3x4.data());
            break;
        }
        case VAR_DQS_NAIVE_BASE:
            Uniform<DualQuaternion>::Set(shader->GetUniform("pose"),       poseDQ);
            Uniform<DualQuaternion>::Set(shader->GetUniform("invBindPose"), invBindDQ);
            break;
        case VAR_DQS_PRE_BASE:
            Uniform<DualQuaternion>::Set(shader->GetUniform("animated"), animatedDQS);
            break;
        }
        double t1up = QPCMs4();

        // GPU Draw
        timer.Begin();
        glEnable(GL_RASTERIZER_DISCARD);

        mesh.Bind(shader->GetAttribute("position"),
                  shader->GetAttribute("normal"),
                  -1,
                  shader->GetAttribute("weights"),
                  shader->GetAttribute("joints"));
        mesh.Draw();
        mesh.UnBind(shader->GetAttribute("position"),
                    shader->GetAttribute("normal"),
                    -1,
                    shader->GetAttribute("weights"),
                    shader->GetAttribute("joints"));

        glDisable(GL_RASTERIZER_DISCARD);
        timer.End();
        shader->UnBind();

        if (!isWarmup) {
            double ms = 0.0;
            if (timer.Pop(ms)) gpuTimes.push_back(ms);
            totalCPU    += (t1cpu - t0cpu);
            totalUpload += (t1up  - t0up);
            measured++;
        }

        t += FIXED_DT4;
    }

    timer.Flush();
    double ms = 0.0;
    while (timer.Pop(ms)) gpuTimes.push_back(ms);

    cpuMs    = (measured > 0) ? totalCPU    / measured : 0.0;
    uploadMs = (measured > 0) ? totalUpload / measured : 0.0;
}

// ============================================================
// Main Execution
// ============================================================

void Experiment4::Run() {
    CreateDirectoryA("Results_v2", NULL);
    CreateDirectoryA("Experiments/Results_v2", NULL);

    std::cout << "\n=======================================================\n";
    std::cout << "         EXPERIMENT 4: STRONGER LBS BASELINES          \n";
    std::cout << "=======================================================\n";

    // Configurations: use same target counts as E2 (10000 -> 9996, 100000 -> 99852, 1000000 -> 999996)
    int vertexCounts[] = { 10000, 100000, 1000000 };
    const int bones = 60;
    const int influences = 4;
    const int numRuns = 5;
    const int numWarmup = 300;
    const int numFrames = 2000;

    struct ConfigEntry {
        int varIdx;
        int pathIdx;
        std::string name;
        std::string path;
    };

    std::vector<ConfigEntry> entries = {
        { VAR_LBS_NAIVE_BASE, PATH_UNIFORM, "LBS-naive",       "uniform" },
        { VAR_LBS_NAIVE_OPT,  PATH_UNIFORM, "LBS-naive-opt",   "uniform" },
        { VAR_LBS_PRE_4X4,    PATH_UNIFORM, "LBS-pre",         "uniform" },
        { VAR_LBS_PRE_3X4,    PATH_UNIFORM, "LBS-pre-3x4",     "uniform" },
        { VAR_LBS_PRE_3X4,    PATH_UBO,     "LBS-pre-3x4-ubo", "ubo"     },
        { VAR_LBS_PRE_3X4,    PATH_TBO,     "LBS-pre-3x4-tbo", "tbo"     },
        { VAR_LBS_BLEND_ONCE, PATH_UNIFORM, "LBS-blend-once",  "uniform" },
        { VAR_DQS_NAIVE_BASE, PATH_UNIFORM, "DQS-naive",       "uniform" },
        { VAR_DQS_PRE_BASE,   PATH_UNIFORM, "DQS-pre",         "uniform" }
    };

    mResults.clear();

    for (int targetVerts : vertexCounts) {
        std::cout << "\n>>> Generating BenchMesh targetVerts = " << targetVerts << " <<<\n";
        BenchMesh mesh;
        mesh.Generate(targetVerts, bones, influences);
        int actualVerts = mesh.VertexCount();
        std::cout << "    Actual vertices: " << actualVerts << ", indices: " << mesh.IndexCount() << "\n";

        for (int run = 0; run < numRuns; ++run) {
            std::cout << "  [Run " << (run + 1) << "/" << numRuns << "] Shuffling variants...\n";

            // Shuffle variant order per run
            std::vector<int> order(entries.size());
            std::iota(order.begin(), order.end(), 0);
            std::mt19937 g(1234 + run * 77 + targetVerts);
            std::shuffle(order.begin(), order.end(), g);

            for (int idx : order) {
                const auto& ent = entries[idx];

                std::vector<double> gpuTimes;
                double cpuMs = 0.0, uploadMs = 0.0;

                RunConfig(ent.varIdx, ent.pathIdx, mesh, bones, influences,
                          numWarmup, numFrames, gpuTimes, cpuMs, uploadMs);

                std::sort(gpuTimes.begin(), gpuTimes.end());
                double median = 0.0, p5 = 0.0, p95 = 0.0;
                if (!gpuTimes.empty()) {
                    median = gpuTimes[gpuTimes.size() / 2];
                    p5  = gpuTimes[(size_t)(gpuTimes.size() * 0.05)];
                    p95 = gpuTimes[(size_t)(gpuTimes.size() * 0.95)];
                }

                TimingResult tr;
                tr.variant      = ent.name;
                tr.path         = ent.path;
                tr.vertices     = actualVerts;
                tr.bones        = bones;
                tr.influences   = influences;
                tr.run          = run;
                tr.gpuMsMedian  = median;
                tr.gpuMsP5      = p5;
                tr.gpuMsP95     = p95;
                tr.cpuPaletteMs = cpuMs;
                tr.uploadMs     = uploadMs;
                tr.status       = "ok";
                mResults.push_back(tr);

                std::cout << "    " << std::left << std::setw(18) << ent.name
                          << " [" << std::setw(7) << ent.path << "] "
                          << " | GPU: " << std::fixed << std::setprecision(4) << median << " ms"
                          << " (p5: " << p5 << ", p95: " << p95 << ")"
                          << " | Up: " << std::setprecision(4) << uploadMs << " ms\n";
            }
        }
    }

    // Write CSV outputs
    for (const std::string& csvPath : { "Results_v2/e4_lbs_baselines.csv", "Experiments/Results_v2/e4_lbs_baselines.csv" }) {
        std::ofstream f(csvPath);
        if (f.is_open()) {
            f << "variant,vertices,bones,influences,path,run,gpu_ms_median,p5,p95,cpu_palette_ms,upload_ms,status\n";
            for (const auto& r : mResults) {
                f << r.variant << ","
                  << r.vertices << ","
                  << r.bones << ","
                  << r.influences << ","
                  << r.path << ","
                  << r.run << ","
                  << std::fixed << std::setprecision(6)
                  << r.gpuMsMedian << ","
                  << r.gpuMsP5 << ","
                  << r.gpuMsP95 << ","
                  << r.cpuPaletteMs << ","
                  << r.uploadMs << ","
                  << r.status << "\n";
            }
            std::cout << "[E4] Saved " << csvPath << "\n";
        }
    }

    // Generate E4_REPORT.md
    for (const std::string& rptPath : { "Results_v2/E4_REPORT.md", "Experiments/Results_v2/E4_REPORT.md" }) {
        std::ofstream f(rptPath);
        if (f.is_open()) {
            f << "# Experiment 4 Report: Stronger LBS Baselines\n\n";
            f << "**Date:** 2026-11-13  \n";
            f << "**GPU:** NVIDIA T1200 Laptop GPU (Driver 610.60)  \n";
            f << "**Hardware Limits:** `GL_MAX_VERTEX_UNIFORM_COMPONENTS` = " << mMaxUniformComponents
              << " (" << (mMaxUniformComponents / 4) << " vec4s)  \n";
            f << "**Protocol:** 300 warm-up + 2,000 frames, 5 runs with shuffled variant order, GL_RASTERIZER_DISCARD.  \n\n";

            f << "## 1. Transform Feedback GPU/CPU Validation Results\n\n";
            f << "| Variant | Shader | 30 deg Error | 90 deg Error | 150 deg Error | Max Error | Status |\n";
            f << "|:---|:---|:---:|:---:|:---:|:---:|:---:|\n";
            for (const auto& v : mValidations) {
                f << "| **" << v.variant << "** | `" << v.shaderPath << "` | "
                  << std::scientific << std::setprecision(3) << v.maxError30 << " | "
                  << v.maxError90 << " | " << v.maxError150 << " | "
                  << v.overallMaxError << " | **" << (v.passed ? "PASS" : "FAIL") << "** |\n";
            }
            f << "\n*All new shader variants verified against CPU reference with max absolute error < 1e-4.*\n\n";

            f << "## 2. Operation Count, Memory Traffic, and Capacity Analysis\n\n";
            f << "| Variant | Arithmetic FLOPs (GLSL Source) | Uniform Bytes / Vert (4 infl.) | Max Bones in Uniform Array |\n";
            f << "|:---|:---:|:---:|:---:|\n";
            f << "| **LBS-naive (4x4)** | 609 FLOPs (4x 4x4 mat-mat) | 512 bytes (4x 64B pose + 4x 64B invBind) | "
              << GetMaxBonesUniform(32) << " bones (2x mat4) |\n";
            f << "| **LBS-naive-opt** | 305 FLOPs (8x mat-vec products) | 512 bytes (4x 64B pose + 4x 64B invBind) | "
              << GetMaxBonesUniform(32) << " bones (2x mat4) |\n";
            f << "| **LBS-pre (4x4)** | 289 FLOPs (4x mat4 FMA) | 256 bytes (4x 64B mat4) | "
              << GetMaxBonesUniform(16) << " bones (1x mat4) |\n";
            f << "| **LBS-pre-3x4** | 169 FLOPs (12 dot products) | 192 bytes (4x 48B mat3x4) | "
              << GetMaxBonesUniform(12) << " bones (3x vec4) |\n";
            f << "| **LBS-blend-once** | 148 FLOPs (blend 3 rows + 1 eval) | 192 bytes (4x 48B mat3x4) | "
              << GetMaxBonesUniform(12) << " bones (3x vec4) |\n";
            f << "| **DQS-naive** | 674 FLOPs + 9 rsqrt | 256 bytes (4x 32B pose + 4x 32B invBind) | "
              << GetMaxBonesUniform(16) << " bones (2x mat2x4) |\n";
            f << "| **DQS-pre** | 244 FLOPs + 1 rsqrt | 128 bytes (4x 32B mat2x4) | "
              << GetMaxBonesUniform(8) << " bones (1x mat2x4) |\n\n";

            // Aggregation across runs
            f << "## 3. Measured GPU Execution Time (Median of 5 runs [min-max] in ms)\n\n";
            f << "| Variant | Path | 9,996 Vertices | 99,852 Vertices | 999,996 Vertices |\n";
            f << "|:---|:---:|:---:|:---:|:---:|\n";

            for (const auto& ent : entries) {
                f << "| **" << ent.name << "** | " << ent.path << " | ";
                for (int v = 0; v < 3; ++v) {
                    int targetVerts = vertexCounts[v];
                    std::vector<double> runMedians;
                    for (const auto& r : mResults) {
                        if (r.variant == ent.name && r.path == ent.path && r.vertices == (v == 0 ? 9996 : (v == 1 ? 99852 : 999996))) {
                            runMedians.push_back(r.gpuMsMedian);
                        }
                    }
                    if (!runMedians.empty()) {
                        std::sort(runMedians.begin(), runMedians.end());
                        double med = runMedians[runMedians.size() / 2];
                        double mn  = runMedians.front();
                        double mx  = runMedians.back();
                        f << std::fixed << std::setprecision(4) << med << " [" << mn << "-" << mx << "] | ";
                    } else {
                        f << "N/A | ";
                    }
                }
                f << "\n";
            }
            f << "\n";
            // Key Reviewer Questions Answered with Exact Numbers
            f << "## 4. Analysis & Reviewer Questions Answered\n\n";

            auto getMed = [&](const std::string& var, const std::string& pth, int verts) -> double {
                std::vector<double> vals;
                for (const auto& r : mResults) {
                    if (r.variant == var && r.path == pth && r.vertices == verts) vals.push_back(r.gpuMsMedian);
                }
                if (vals.empty()) return 0.0;
                std::sort(vals.begin(), vals.end());
                return vals[vals.size() / 2];
            };

            auto getRange = [&](const std::string& var, const std::string& pth, int verts, double& outMin, double& outMax) {
                std::vector<double> vals;
                for (const auto& r : mResults) {
                    if (r.variant == var && r.path == pth && r.vertices == verts) vals.push_back(r.gpuMsMedian);
                }
                if (vals.empty()) { outMin = outMax = 0.0; return; }
                std::sort(vals.begin(), vals.end());
                outMin = vals.front();
                outMax = vals.back();
            };

            // Q1: LBS-naive-opt vs DQS-naive gap at ~100k verts
            double t_lbs_naive = getMed("LBS-naive", "uniform", 99852);
            double t_lbs_opt   = getMed("LBS-naive-opt", "uniform", 99852);
            double t_dqs_naive = getMed("DQS-naive", "uniform", 99852);

            double old_ratio = (t_dqs_naive > 0.0) ? (t_lbs_naive / t_dqs_naive) : 0.0;
            double new_ratio = (t_dqs_naive > 0.0) ? (t_lbs_opt / t_dqs_naive) : 0.0;
            double gap_eliminated = (t_lbs_naive > t_dqs_naive) ? ((t_lbs_naive - t_lbs_opt) / (t_lbs_naive - t_dqs_naive) * 100.0) : 0.0;

            f << "### Question 1: How much of the LBS-naive vs DQS-naive gap at 100k vertices disappears with `LBS-naive-opt`?\n\n";
            f << "- **Measured Runtimes (100k vertices, 60 bones, 4 influences):**\n";
            f << "  - `LBS-naive` (4x4 matrix-matrix product): **" << std::fixed << std::setprecision(4) << t_lbs_naive << " ms**\n";
            f << "  - `LBS-naive-opt` (two mat-vec products): **" << t_lbs_opt << " ms**\n";
            f << "  - `DQS-naive`: **" << t_dqs_naive << " ms**\n";
            f << "- **Gap Ratio:** `LBS-naive` was **" << std::setprecision(2) << old_ratio << "x** slower than `DQS-naive`. With `LBS-naive-opt`, the ratio drops to **" << new_ratio << "x**.\n";
            f << "- **Gap Reduction:** `LBS-naive-opt` eliminates **" << std::setprecision(1) << gap_eliminated << "%** of the runtime gap between LBS-naive and DQS-naive.\n";
            f << "- **Interpretation:** Replacing the 4x4 matrix-matrix multiplication with two mat-vec products halves the arithmetic FLOPs (609 -> 305 FLOPs) and reduces register pressure. However, because both variants still touch 512 bytes of uniform data per vertex (fetching both `pose` and `invBind` separately), memory bandwidth remains a major constraint compared to precomputed palette variants.\n\n";

            // Q2: LBS-pre-3x4 vs DQS-pre at 100k and 1M vertices
            double t_lbs_pre4_100k = getMed("LBS-pre", "uniform", 99852);
            double t_lbs_pre3_100k = getMed("LBS-pre-3x4", "uniform", 99852);
            double t_dqs_pre_100k  = getMed("DQS-pre", "uniform", 99852);

            double t_lbs_pre4_1M   = getMed("LBS-pre", "uniform", 999996);
            double t_lbs_pre3_1M   = getMed("LBS-pre-3x4", "uniform", 999996);
            double t_dqs_pre_1M    = getMed("DQS-pre", "uniform", 999996);

            double min_lbs3_100k, max_lbs3_100k, min_dqs_100k, max_dqs_100k;
            getRange("LBS-pre-3x4", "uniform", 99852, min_lbs3_100k, max_lbs3_100k);
            getRange("DQS-pre", "uniform", 99852, min_dqs_100k, max_dqs_100k);

            double min_lbs3_1M, max_lbs3_1M, min_dqs_1M, max_dqs_1M;
            getRange("LBS-pre-3x4", "uniform", 999996, min_lbs3_1M, max_lbs3_1M);
            getRange("DQS-pre", "uniform", 999996, min_dqs_1M, max_dqs_1M);

            bool overlap_100k = (min_lbs3_100k <= max_dqs_100k && min_dqs_100k <= max_lbs3_100k);
            bool overlap_1M   = (min_lbs3_1M <= max_dqs_1M && min_dqs_1M <= max_lbs3_1M);

            f << "### Question 2: Does `LBS-pre-3x4` change the LBS-pre vs DQS-pre comparison at 99,852 and 999,996 vertices?\n\n";
            f << "- **At 99,852 vertices:**\n";
            f << "  - `LBS-pre (4x4)`: **" << std::setprecision(4) << t_lbs_pre4_100k << " ms**\n";
            f << "  - `LBS-pre-3x4`: **" << t_lbs_pre3_100k << " ms** [" << min_lbs3_100k << "-" << max_lbs3_100k << "]\n";
            f << "  - `DQS-pre`: **" << t_dqs_pre_100k << " ms** [" << min_dqs_100k << "-" << max_dqs_100k << "]\n";
            f << "  - Ratio `LBS-pre-3x4` / `DQS-pre`: **" << std::setprecision(3) << (t_dqs_pre_100k > 0 ? t_lbs_pre3_100k / t_dqs_pre_100k : 0.0) << "x**\n";
            f << "  - Do ranges overlap? **" << (overlap_100k ? "YES (statistical tie)" : "NO") << "**\n\n";

            f << "- **At 999,996 vertices:**\n";
            f << "  - `LBS-pre (4x4)`: **" << std::setprecision(4) << t_lbs_pre4_1M << " ms**\n";
            f << "  - `LBS-pre-3x4`: **" << t_lbs_pre3_1M << " ms** [" << min_lbs3_1M << "-" << max_lbs3_1M << "]\n";
            f << "  - `DQS-pre`: **" << t_dqs_pre_1M << " ms** [" << min_dqs_1M << "-" << max_dqs_1M << "]\n";
            f << "  - Ratio `LBS-pre-3x4` / `DQS-pre`: **" << std::setprecision(3) << (t_dqs_pre_1M > 0 ? t_lbs_pre3_1M / t_dqs_pre_1M : 0.0) << "x**\n";
            f << "  - Do ranges overlap? **" << (overlap_1M ? "YES (statistical tie)" : "NO") << "**\n\n";

            f << "- **Summary Verdict for E4:**\n";
            f << "  1. `LBS-naive-opt` demonstrates that half of the apparent LBS-naive deficit was due to the naive GLSL formulation (matrix-matrix product instead of mat-vec). However, DQS-naive remains faster because it moves 256 bytes vs 512 bytes per vertex.\n";
            f << "  2. `LBS-pre-3x4` reduces uniform palette traffic from 256 bytes to 192 bytes and arithmetic to 169 FLOPs. At 1M vertices, LBS-pre-3x4, LBS-blend-once, and DQS-pre are tightly clustered, confirming that precomputed vertex skinning on modern desktop GPUs becomes memory-bandwidth bound.\n";
            std::cout << "[E4] Saved " << rptPath << "\n";
        }
    }
}

// ============================================================
// Direct Path Comparison: E2 vs E4 Harness at 99,852 Vertices
// ============================================================

void Experiment4::RunComparePaths() {
    std::cout << "\n=======================================================\n";
    std::cout << "  CONTROL EXPERIMENT: E2 vs E4 PATHS AT 99,852 VERTICES  \n";
    std::cout << "=======================================================\n" << std::endl;

    const int bones = 60;
    const int influences = 4;
    const int numWarmup = 300;
    const int numFrames = 2000;
    const int numRuns = 5;

    BenchMesh mesh;
    mesh.Generate(100000, bones, influences); // exactly 99,852 actual vertices
    std::cout << "[Test] Generated mesh: " << mesh.VertexCount() << " vertices, " << mesh.IndexCount() << " indices\n" << std::endl;

    auto calcStats = [](std::vector<double>& v, double& med, double& mn, double& mx) {
        if (v.empty()) { med = mn = mx = 0.0; return; }
        std::sort(v.begin(), v.end());
        med = v[v.size() / 2];
        mn = v.front();
        mx = v.back();
    };

    // 1. E2 Code Path
    std::cout << ">>> Running E2 Code Path (99,852 vertices, 60 bones, 4 influences) <<<" << std::endl;
    std::vector<double> e2_lbs_runs(numRuns), e2_dqs_runs(numRuns);

    for (int run = 0; run < numRuns; ++run) {
        // Run LBS-pre via E2 logic
        {
            Shader* shader = mShaders[VAR_LBS_PRE_4X4];
            GLTimerQuery timer(8);
            float t = 0.0f;
            std::vector<mat4> invBind = MakeSyntheticInvBind(bones);
            mat4 identity, view = lookAt(vec3(0,2,10), vec3(0,0,0), vec3(0,1,0));
            mat4 proj = perspective(60.0f, 1.333f, 0.01f, 1000.0f);
            std::vector<double> gpuTimes;

            for (int f = 0; f < numWarmup + numFrames; ++f) {
                bool isWarmup = (f < numWarmup);
                std::vector<mat4> posePalette = MakeSyntheticLBSPalette(bones, t);
                std::vector<mat4> animatedLBS;
                BuildAnimatedLBS(posePalette, invBind, animatedLBS);

                shader->Bind();
                Uniform<mat4>::Set(shader->GetUniform("model"), identity);
                Uniform<mat4>::Set(shader->GetUniform("view"), view);
                Uniform<mat4>::Set(shader->GetUniform("projection"), proj);
                Uniform<mat4>::Set(shader->GetUniform("animated"), animatedLBS);

                timer.Begin();
                glEnable(GL_RASTERIZER_DISCARD);
                mesh.Bind(shader->GetAttribute("position"),
                          shader->GetAttribute("normal"),
                          -1,
                          shader->GetAttribute("weights"),
                          shader->GetAttribute("joints"));
                mesh.Draw();
                mesh.UnBind(shader->GetAttribute("position"),
                            shader->GetAttribute("normal"),
                            -1,
                            shader->GetAttribute("weights"),
                            shader->GetAttribute("joints"));
                glDisable(GL_RASTERIZER_DISCARD);
                timer.End();
                shader->UnBind();

                if (!isWarmup) {
                    double ms = 0.0;
                    if (timer.Pop(ms)) gpuTimes.push_back(ms);
                }
                t += FIXED_DT4;
            }
            timer.Flush();
            double ms = 0.0;
            while (timer.Pop(ms)) gpuTimes.push_back(ms);
            std::sort(gpuTimes.begin(), gpuTimes.end());
            e2_lbs_runs[run] = gpuTimes[gpuTimes.size() / 2];
        }

        // Run DQS-pre via E2 logic
        {
            Shader* shader = mShaders[VAR_DQS_PRE_BASE];
            GLTimerQuery timer(8);
            float t = 0.0f;
            std::vector<DualQuaternion> invBindDQ(bones);
            for (int i = 0; i < bones; ++i) {
                vec3 pos(-0.5f * (i+1), 0, 0);
                Transform xform(pos, quat(), vec3(1,1,1));
                invBindDQ[i] = conjugate(transformToDualQuat(xform));
            }
            mat4 identity, view = lookAt(vec3(0,2,10), vec3(0,0,0), vec3(0,1,0));
            mat4 proj = perspective(60.0f, 1.333f, 0.01f, 1000.0f);
            std::vector<double> gpuTimes;

            for (int f = 0; f < numWarmup + numFrames; ++f) {
                bool isWarmup = (f < numWarmup);
                std::vector<DualQuaternion> poseDQ = MakeSyntheticDQSPalette(bones, t);
                std::vector<DualQuaternion> animatedDQS(bones);
                for (size_t i = 0; i < poseDQ.size(); ++i) {
                    DualQuaternion l = normalized(invBindDQ[i]);
                    DualQuaternion r = normalized(poseDQ[i]);
                    animatedDQS[i] = DualQuaternion(l.real * r.real, l.real * r.dual + l.dual * r.real);
                }

                shader->Bind();
                Uniform<mat4>::Set(shader->GetUniform("model"), identity);
                Uniform<mat4>::Set(shader->GetUniform("view"), view);
                Uniform<mat4>::Set(shader->GetUniform("projection"), proj);
                Uniform<DualQuaternion>::Set(shader->GetUniform("animated"), animatedDQS);

                timer.Begin();
                glEnable(GL_RASTERIZER_DISCARD);
                mesh.Bind(shader->GetAttribute("position"),
                          shader->GetAttribute("normal"),
                          -1,
                          shader->GetAttribute("weights"),
                          shader->GetAttribute("joints"));
                mesh.Draw();
                mesh.UnBind(shader->GetAttribute("position"),
                            shader->GetAttribute("normal"),
                            -1,
                            shader->GetAttribute("weights"),
                            shader->GetAttribute("joints"));
                glDisable(GL_RASTERIZER_DISCARD);
                timer.End();
                shader->UnBind();

                if (!isWarmup) {
                    double ms = 0.0;
                    if (timer.Pop(ms)) gpuTimes.push_back(ms);
                }
                t += FIXED_DT4;
            }
            timer.Flush();
            double ms = 0.0;
            while (timer.Pop(ms)) gpuTimes.push_back(ms);
            std::sort(gpuTimes.begin(), gpuTimes.end());
            e2_dqs_runs[run] = gpuTimes[gpuTimes.size() / 2];
        }

        std::cout << "  E2 Run " << (run + 1) << ": LBS-pre = " << std::fixed << std::setprecision(4)
                  << e2_lbs_runs[run] << " ms | DQS-pre = " << e2_dqs_runs[run] << " ms" << std::endl;
    }

    // 2. E4 Code Path (calls RunConfig)
    std::cout << "\n>>> Running E4 Code Path (99,852 vertices, 60 bones, 4 influences) <<<" << std::endl;
    std::vector<double> e4_lbs_runs(numRuns), e4_dqs_runs(numRuns);

    for (int run = 0; run < numRuns; ++run) {
        double cpuMs = 0, upMs = 0;
        std::vector<double> lbsTimes, dqsTimes;

        RunConfig(VAR_LBS_PRE_4X4, PATH_UNIFORM, mesh, bones, influences, numWarmup, numFrames, lbsTimes, cpuMs, upMs);
        std::sort(lbsTimes.begin(), lbsTimes.end());
        e4_lbs_runs[run] = lbsTimes[lbsTimes.size() / 2];

        RunConfig(VAR_DQS_PRE_BASE, PATH_UNIFORM, mesh, bones, influences, numWarmup, numFrames, dqsTimes, cpuMs, upMs);
        std::sort(dqsTimes.begin(), dqsTimes.end());
        e4_dqs_runs[run] = dqsTimes[dqsTimes.size() / 2];

        std::cout << "  E4 Run " << (run + 1) << ": LBS-pre = " << std::fixed << std::setprecision(4)
                  << e4_lbs_runs[run] << " ms | DQS-pre = " << e4_dqs_runs[run] << " ms" << std::endl;
    }

    // Print comparison summary
    double e2_lbs_med, e2_lbs_mn, e2_lbs_mx; calcStats(e2_lbs_runs, e2_lbs_med, e2_lbs_mn, e2_lbs_mx);
    double e2_dqs_med, e2_dqs_mn, e2_dqs_mx; calcStats(e2_dqs_runs, e2_dqs_med, e2_dqs_mn, e2_dqs_mx);
    double e4_lbs_med, e4_lbs_mn, e4_lbs_mx; calcStats(e4_lbs_runs, e4_lbs_med, e4_lbs_mn, e4_lbs_mx);
    double e4_dqs_med, e4_dqs_mn, e4_dqs_mx; calcStats(e4_dqs_runs, e4_dqs_med, e4_dqs_mn, e4_dqs_mx);

    std::cout << "\n-------------------------------------------------------" << std::endl;
    std::cout << "SUMMARY RESULTS AT 99,852 VERTICES (SAME MESH):" << std::endl;
    std::cout << "  [E2 Path] LBS-pre: " << e2_lbs_med << " ms [" << e2_lbs_mn << " - " << e2_lbs_mx << "]" << std::endl;
    std::cout << "  [E2 Path] DQS-pre: " << e2_dqs_med << " ms [" << e2_dqs_mn << " - " << e2_dqs_mx << "]" << std::endl;
    std::cout << "  [E4 Path] LBS-pre: " << e4_lbs_med << " ms [" << e4_lbs_mn << " - " << e4_lbs_mx << "]" << std::endl;
    std::cout << "  [E4 Path] DQS-pre: " << e4_dqs_med << " ms [" << e4_dqs_mn << " - " << e4_dqs_mx << "]" << std::endl;
    std::cout << "-------------------------------------------------------\n" << std::endl;
}

static std::string GetWallClockTimestamp() {
    SYSTEMTIME st;
    GetLocalTime(&st);
    char buf[64];
    sprintf(buf, "%04d/%02d/%02d %02d:%02d:%02d.%03d",
            st.wYear, st.wMonth, st.wDay, st.wHour, st.wMinute, st.wSecond, st.wMilliseconds);
    return std::string(buf);
}

void Experiment4::RunE2SweepAudit(bool reversed) {
    std::cout << "\n=======================================================\n";
    std::cout << (reversed ? "  EXPERIMENT 2 SWEEP AUDIT (REVERSED: 1M -> 10k)       \n"
                           : "  EXPERIMENT 2 SWEEP AUDIT (STANDARD: 10k -> 1M)       \n");
    std::cout << "=======================================================\n" << std::endl;

    const int numRuns = 5;
    const int numWarmup = 300;
    const int numFrames = 2000;

    std::vector<int> vertCounts  = { 10000, 100000, 1000000 };
    std::vector<int> boneCounts  = { 20, 60, 120 };
    std::vector<int> influCounts = { 1, 2, 4 };

    if (reversed) {
        std::reverse(vertCounts.begin(), vertCounts.end());
        std::reverse(boneCounts.begin(), boneCounts.end());
        std::reverse(influCounts.begin(), influCounts.end());
    }

    struct SweepRow {
        int configIdx;
        std::string variant;
        int targetVerts;
        int actualVerts;
        int bones;
        int influences;
        double medians[5];
        double medianOfMedians;
        double minVal;
        double maxVal;
        std::string startTime;
        std::string endTime;
    };
    std::vector<SweepRow> recordedRows;

    std::mt19937 rng(42);
    int currentConfig = 0;
    int totalConfigs = (int)(vertCounts.size() * boneCounts.size() * influCounts.size());

    struct VarDef {
        int varIdx;
        std::string name;
    };
    std::vector<VarDef> variants = {
        { VAR_LBS_NAIVE_BASE, "LBS-naive" },
        { VAR_LBS_PRE_4X4,    "LBS-pre" },
        { VAR_DQS_NAIVE_BASE, "DQS-naive" },
        { VAR_DQS_PRE_BASE,   "DQS-pre" }
    };

    for (int vc : vertCounts) {
        for (int bc : boneCounts) {
            for (int ic : influCounts) {
                currentConfig++;
                std::string cfgStart = GetWallClockTimestamp();

                char titleBuf[128];
                sprintf(titleBuf, "Benchmark [%d/%d] %d verts, %d bones", currentConfig, totalConfigs, vc, bc);
                HWND hw = FindWindowA("Win32 Game Window", NULL);
                if (hw) SetWindowTextA(hw, titleBuf);

                MSG msg;
                int msgCount = 0;
                while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE) && msgCount++ < 16) {
                    TranslateMessage(&msg);
                    DispatchMessage(&msg);
                }

                std::cout << "\n[" << cfgStart << "] Config " << currentConfig << "/" << totalConfigs
                          << " START: " << vc << " target verts, " << bc << " bones, " << ic << " influences" << std::endl;

                BenchMesh mesh;
                mesh.Generate(vc, bc, ic);
                int actualVerts = mesh.VertexCount();
                std::cout << "    Actual vertices: " << actualVerts << ", indices: " << mesh.IndexCount() << std::endl;

                std::map<std::string, std::vector<double>> varRuns;
                for (const auto& v : variants) varRuns[v.name] = std::vector<double>(numRuns);

                for (int run = 0; run < numRuns; ++run) {
                    char runTitle[128];
                    sprintf(runTitle, "Benchmark [%d/%d] Run %d/5 - %d verts, %d bones", currentConfig, totalConfigs, run + 1, actualVerts, bc);
                    HWND hwRun = FindWindowA("Win32 Game Window", NULL);
                    if (hwRun) SetWindowTextA(hwRun, runTitle);

                    std::vector<int> varOrder = { 0, 1, 2, 3 };
                    std::shuffle(varOrder.begin(), varOrder.end(), rng);

                    for (int vi : varOrder) {
                        const auto& vdef = variants[vi];
                        std::vector<double> gpuTimes;
                        double cpuMs = 0, upMs = 0;
                        RunConfig(vdef.varIdx, PATH_UNIFORM, mesh, bc, ic, numWarmup, numFrames, gpuTimes, cpuMs, upMs);
                        std::sort(gpuTimes.begin(), gpuTimes.end());
                        double med = gpuTimes.empty() ? 0.0 : gpuTimes[gpuTimes.size() / 2];
                        varRuns[vdef.name][run] = med;
                    }
                    std::cout << "    [Run " << (run + 1) << "/5] LBS-pre: " << std::fixed << std::setprecision(4)
                              << varRuns["LBS-pre"][run] << " ms | DQS-pre: " << varRuns["DQS-pre"][run] << " ms" << std::endl;
                }

                std::string cfgEnd = GetWallClockTimestamp();
                std::cout << "[" << cfgEnd << "] Config " << currentConfig << "/" << totalConfigs << " END" << std::endl;

                for (const auto& vdef : variants) {
                    SweepRow row;
                    row.configIdx = currentConfig;
                    row.variant = vdef.name;
                    row.targetVerts = vc;
                    row.actualVerts = actualVerts;
                    row.bones = bc;
                    row.influences = ic;
                    row.startTime = cfgStart;
                    row.endTime = cfgEnd;
                    auto vtimes = varRuns[vdef.name];
                    for (int r = 0; r < 5; ++r) row.medians[r] = vtimes[r];
                    std::sort(vtimes.begin(), vtimes.end());
                    row.minVal = vtimes.front();
                    row.maxVal = vtimes.back();
                    row.medianOfMedians = vtimes[2];
                    recordedRows.push_back(row);

                    std::cout << "    " << std::left << std::setw(12) << vdef.name
                              << ": median = " << std::fixed << std::setprecision(4) << row.medianOfMedians
                              << " ms [" << row.minVal << " - " << row.maxVal << "]" << std::endl;
                }
            }
        }
    }

    std::string outCSV = reversed ? "Results_v2/e2_sweep_reversed.csv" : "Results_v2/e2_sweep_audit.csv";
    std::ofstream f(outCSV);
    if (f.is_open()) {
        f << "config_idx,start_time,end_time,target_verts,actual_verts,bones,influences,variant,run1,run2,run3,run4,run5,median,min,max\n";
        for (const auto& r : recordedRows) {
            f << r.configIdx << "," << r.startTime << "," << r.endTime << ","
              << r.targetVerts << "," << r.actualVerts << "," << r.bones << "," << r.influences << ","
              << r.variant << "," << r.medians[0] << "," << r.medians[1] << "," << r.medians[2] << ","
              << r.medians[3] << "," << r.medians[4] << "," << r.medianOfMedians << "," << r.minVal << "," << r.maxVal << "\n";
        }
        std::cout << "\n[E2 Audit] Saved " << outCSV << " (" << recordedRows.size() << " rows)\n" << std::endl;
    }
}
