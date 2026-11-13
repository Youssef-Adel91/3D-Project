// lbs_pre_3x4_tbo_tf.vert: LBS-pre-3x4 via texture buffer with transform feedback for GPU/CPU validation.
#version 330 core

#ifndef INFLUENCES
#define INFLUENCES 4
#endif

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;
uniform int boneCount;

in vec3 position;
in vec3 normal;
in vec2 texCoord;
in vec4 weights;
in ivec4 joints;

out vec3 norm;
out vec3 fragPos;
out vec2 uv;
out vec3 tf_position;

uniform samplerBuffer skinMatrices;

void main() {
    vec4 p = vec4(position, 1.0);
    vec3 n = normal;

    int b0 = 3 * joints.x;
    vec4 r0_0 = texelFetch(skinMatrices, b0);
    vec4 r0_1 = texelFetch(skinMatrices, b0 + 1);
    vec4 r0_2 = texelFetch(skinMatrices, b0 + 2);
    vec3 skinnedPos = weights.x * vec3(dot(r0_0, p), dot(r0_1, p), dot(r0_2, p));
    vec3 skinnedNorm = weights.x * vec3(dot(r0_0.xyz, n), dot(r0_1.xyz, n), dot(r0_2.xyz, n));

#if INFLUENCES >= 2
    int b1 = 3 * joints.y;
    vec4 r1_0 = texelFetch(skinMatrices, b1);
    vec4 r1_1 = texelFetch(skinMatrices, b1 + 1);
    vec4 r1_2 = texelFetch(skinMatrices, b1 + 2);
    skinnedPos += weights.y * vec3(dot(r1_0, p), dot(r1_1, p), dot(r1_2, p));
    skinnedNorm += weights.y * vec3(dot(r1_0.xyz, n), dot(r1_1.xyz, n), dot(r1_2.xyz, n));
#endif

#if INFLUENCES >= 3
    int b2 = 3 * joints.z;
    vec4 r2_0 = texelFetch(skinMatrices, b2);
    vec4 r2_1 = texelFetch(skinMatrices, b2 + 1);
    vec4 r2_2 = texelFetch(skinMatrices, b2 + 2);
    skinnedPos += weights.z * vec3(dot(r2_0, p), dot(r2_1, p), dot(r2_2, p));
    skinnedNorm += weights.z * vec3(dot(r2_0.xyz, n), dot(r2_1.xyz, n), dot(r2_2.xyz, n));
#endif

#if INFLUENCES >= 4
    int b3 = 3 * joints.w;
    vec4 r3_0 = texelFetch(skinMatrices, b3);
    vec4 r3_1 = texelFetch(skinMatrices, b3 + 1);
    vec4 r3_2 = texelFetch(skinMatrices, b3 + 2);
    skinnedPos += weights.w * vec3(dot(r3_0, p), dot(r3_1, p), dot(r3_2, p));
    skinnedNorm += weights.w * vec3(dot(r3_0.xyz, n), dot(r3_1.xyz, n), dot(r3_2.xyz, n));
#endif

    vec4 worldPos = model * vec4(skinnedPos, 1.0);
    gl_Position = projection * view * worldPos;

    fragPos = worldPos.xyz;
    norm = vec3(model * vec4(skinnedNorm, 0.0));
    uv = texCoord;
    tf_position = skinnedPos;
}

