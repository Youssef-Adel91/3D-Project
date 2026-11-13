// lbs_pre_3x4_ubo_tf.vert: LBS-pre-3x4 via UBO with transform feedback for GPU/CPU validation.
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
out vec3 tf_position;

layout(std140) uniform SkinningData {
    vec4 palette3x4[1536];  // up to 512 bones * 3 rows
};

void main() {
    vec4 p = vec4(position, 1.0);
    vec3 n = normal;

    int b0 = 3 * joints.x;
    vec3 skinnedPos = weights.x * vec3(dot(palette3x4[b0],     p),
                                       dot(palette3x4[b0 + 1], p),
                                       dot(palette3x4[b0 + 2], p));
    vec3 skinnedNorm = weights.x * vec3(dot(palette3x4[b0].xyz,     n),
                                        dot(palette3x4[b0 + 1].xyz, n),
                                        dot(palette3x4[b0 + 2].xyz, n));

#if INFLUENCES >= 2
    int b1 = 3 * joints.y;
    skinnedPos += weights.y * vec3(dot(palette3x4[b1],     p),
                                   dot(palette3x4[b1 + 1], p),
                                   dot(palette3x4[b1 + 2], p));
    skinnedNorm += weights.y * vec3(dot(palette3x4[b1].xyz,     n),
                                    dot(palette3x4[b1 + 1].xyz, n),
                                    dot(palette3x4[b1 + 2].xyz, n));
#endif

#if INFLUENCES >= 3
    int b2 = 3 * joints.z;
    skinnedPos += weights.z * vec3(dot(palette3x4[b2],     p),
                                   dot(palette3x4[b2 + 1], p),
                                   dot(palette3x4[b2 + 2], p));
    skinnedNorm += weights.z * vec3(dot(palette3x4[b2].xyz,     n),
                                    dot(palette3x4[b2 + 1].xyz, n),
                                    dot(palette3x4[b2 + 2].xyz, n));
#endif

#if INFLUENCES >= 4
    int b3 = 3 * joints.w;
    skinnedPos += weights.w * vec3(dot(palette3x4[b3],     p),
                                   dot(palette3x4[b3 + 1], p),
                                   dot(palette3x4[b3 + 2], p));
    skinnedNorm += weights.w * vec3(dot(palette3x4[b3].xyz,     n),
                                    dot(palette3x4[b3 + 1].xyz, n),
                                    dot(palette3x4[b3 + 2].xyz, n));
#endif

    vec4 worldPos = model * vec4(skinnedPos, 1.0);
    gl_Position = projection * view * worldPos;

    fragPos = worldPos.xyz;
    norm = vec3(model * vec4(skinnedNorm, 0.0));
    uv = texCoord;
    tf_position = skinnedPos;
}

