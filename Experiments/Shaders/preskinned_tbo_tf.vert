// preskinned_tbo_tf.vert: LBS-pre via TBO with transform feedback output for GPU/CPU validation.
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
out vec3 tf_position;  // transform feedback capture

uniform samplerBuffer skinMatrices;

mat4 FetchMatrix(int boneIdx) {
    int base = boneIdx * 4;
    vec4 c0 = texelFetch(skinMatrices, base + 0);
    vec4 c1 = texelFetch(skinMatrices, base + 1);
    vec4 c2 = texelFetch(skinMatrices, base + 2);
    vec4 c3 = texelFetch(skinMatrices, base + 3);
    return mat4(c0, c1, c2, c3);
}

void main() {
    mat4 skin = FetchMatrix(joints.x) * weights.x;
#if INFLUENCES >= 2
    skin += FetchMatrix(joints.y) * weights.y;
#endif
#if INFLUENCES >= 3
    skin += FetchMatrix(joints.z) * weights.z;
#endif
#if INFLUENCES >= 4
    skin += FetchMatrix(joints.w) * weights.w;
#endif

    vec4 skinnedPos = skin * vec4(position, 1.0);
    gl_Position = projection * view * model * skinnedPos;
    fragPos = vec3(model * skinnedPos);
    norm = vec3(model * skin * vec4(normal, 0.0));
    uv = texCoord;
    // With model=identity: tf_position = local skinned position
    tf_position = skinnedPos.xyz;
}

