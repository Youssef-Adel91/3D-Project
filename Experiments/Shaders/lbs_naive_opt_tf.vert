// lbs_naive_opt_tf.vert: LBS-naive-opt with transform feedback for GPU/CPU validation.
#version 330 core

#ifndef INFLUENCES
#define INFLUENCES 4
#endif

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

in vec3 position;
in vec3 normal;
in vec2 texCoord;
in vec4 weights;
in ivec4 joints;

uniform mat4 pose[120];
uniform mat4 invBindPose[120];

out vec3 norm;
out vec3 fragPos;
out vec2 uv;
out vec3 tf_position;

void main() {
    vec4 p = vec4(position, 1.0);
    vec4 n = vec4(normal, 0.0);

    vec4 v0 = pose[joints.x] * (invBindPose[joints.x] * p);
    vec4 n0 = pose[joints.x] * (invBindPose[joints.x] * n);
    vec3 skinnedPos = weights.x * v0.xyz;
    vec3 skinnedNorm = weights.x * n0.xyz;

#if INFLUENCES >= 2
    vec4 v1 = pose[joints.y] * (invBindPose[joints.y] * p);
    vec4 n1 = pose[joints.y] * (invBindPose[joints.y] * n);
    skinnedPos += weights.y * v1.xyz;
    skinnedNorm += weights.y * n1.xyz;
#endif

#if INFLUENCES >= 3
    vec4 v2 = pose[joints.z] * (invBindPose[joints.z] * p);
    vec4 n2 = pose[joints.z] * (invBindPose[joints.z] * n);
    skinnedPos += weights.z * v2.xyz;
    skinnedNorm += weights.z * n2.xyz;
#endif

#if INFLUENCES >= 4
    vec4 v3 = pose[joints.w] * (invBindPose[joints.w] * p);
    vec4 n3 = pose[joints.w] * (invBindPose[joints.w] * n);
    skinnedPos += weights.w * v3.xyz;
    skinnedNorm += weights.w * n3.xyz;
#endif

    vec4 worldPos = model * vec4(skinnedPos, 1.0);
    gl_Position = projection * view * worldPos;

    fragPos = worldPos.xyz;
    norm = vec3(model * vec4(skinnedNorm, 0.0));
    uv = texCoord;
    tf_position = skinnedPos;
}

