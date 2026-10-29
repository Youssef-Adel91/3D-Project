#ifndef _H_SAMPLE_
#define _H_SAMPLE_

#include "Application.h"
#include <vector>
#include <string>
#include "Pose.h"
#include "Clip.h"
#include "Skeleton.h"
#include "Mesh.h"
#include "Texture.h"
#include "Shader.h"
#include "DualQuaternion.h"
#include "mat4.h"

enum VisualModel {
    VISUAL_BAR = 0,
    VISUAL_WOMAN = 1
};

enum VisualMotion {
    MOTION_TWIST = 0,
    MOTION_BEND = 1
};

class Sample : public Application {
private:
    // Assets: Bar
    Texture*          mTextureBar;
    std::vector<Mesh> mMeshesBar;
    Skeleton          mSkeletonBar;

    // Assets: Woman
    Texture*          mTextureWoman;
    std::vector<Mesh> mMeshesWoman;
    Skeleton          mSkeletonWoman;
    std::vector<Clip> mClipsWoman;

    // Shaders
    Shader* mLBSShader;
    Shader* mDQSShader;

    // Interactive State
    VisualModel  mModelMode;
    VisualMotion mMotionMode;
    bool         mIsPlaying;
    float        mAngleDeg;
    float        mAngleSpeed;
    float        mAngleDir;
    float        mWomanPlaybackTime;

    // Bounding Box for Woman
    vec3 mWomanCenter;
    vec3 mWomanSize;
    float mWomanCamDist;

public:
    Sample();
    virtual ~Sample();

    virtual void Initialize();
    virtual void Update(float deltaTime);
    virtual void Render(float inAspectRatio); // standard application interface
    virtual void Shutdown();

    // Split-viewport rendering (Left = LBS, Right = DQS)
    void RenderSplit(int width, int height);

    // Keyboard handling
    void OnKeyDown(int key);

    // Window title helper
    std::string GetWindowTitle(float fps) const;

    // Specific pose render helpers (for screenshots / video capture)
    void SetBarTwist(float deg);
    void SetBarBend(float deg);
    void SetWomanTime(float t);
    void SetModel(VisualModel model);
    void DiagnoseWomanDQS();

private:
    Pose BuildBarPose(float deg, VisualMotion motion);
    void ComputeWomanBounds();
};

#endif
