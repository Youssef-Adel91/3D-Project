// lbs_blend_once.vert: LBS accumulating weighted 3x4 matrix first, then applying once.
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

out vec3 norm;
out vec3 fragPos;
out vec2 uv;

// 3x4 matrix palette: 3 vec4 rows per bone (up to 120 bones = 360 vec4s)
uniform vec4 palette3x4[360];

void main() {
    int b0 = 3 * joints.x;
    vec4 r0 = palette3x4[b0]     * weights.x;
    vec4 r1 = palette3x4[b0 + 1] * weights.x;
    vec4 r2 = palette3x4[b0 + 2] * weights.x;

#if INFLUENCES >= 2
    int b1 = 3 * joints.y;
    r0 += palette3x4[b1]     * weights.y;
    r1 += palette3x4[b1 + 1] * weights.y;
    r2 += palette3x4[b1 + 2] * weights.y;
#endif

#if INFLUENCES >= 3
    int b2 = 3 * joints.z;
    r0 += palette3x4[b2]     * weights.z;
    r1 += palette3x4[b2 + 1] * weights.z;
    r2 += palette3x4[b2 + 2] * weights.z;
#endif

#if INFLUENCES >= 4
    int b3 = 3 * joints.w;
    r0 += palette3x4[b3]     * weights.w;
    r1 += palette3x4[b3 + 1] * weights.w;
    r2 += palette3x4[b3 + 2] * weights.w;
#endif

    // Apply blended 3x4 matrix once to position and normal
    vec4 p = vec4(position, 1.0);
    vec3 n = normal;
    vec3 skinnedPos  = vec3(dot(r0, p),     dot(r1, p),     dot(r2, p));
    vec3 skinnedNorm = vec3(dot(r0.xyz, n), dot(r1.xyz, n), dot(r2.xyz, n));

    vec4 worldPos = model * vec4(skinnedPos, 1.0);
    gl_Position = projection * view * worldPos;

    fragPos = worldPos.xyz;
    norm = vec3(model * vec4(skinnedNorm, 0.0));
    uv = texCoord;
}

