// dqs_pre_tbo_tf.vert: DQS-pre via TBO with transform feedback output for GPU/CPU validation.
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
out vec3 tf_position;  // transform feedback capture

uniform samplerBuffer skinDQs;

vec4 mulQ(vec4 Q1, vec4 Q2) {
    return vec4(
         Q2.x * Q1.w + Q2.y * Q1.z - Q2.z * Q1.y + Q2.w * Q1.x,
        -Q2.x * Q1.z + Q2.y * Q1.w + Q2.z * Q1.x + Q2.w * Q1.y,
         Q2.x * Q1.y - Q2.y * Q1.x + Q2.z * Q1.w + Q2.w * Q1.z,
        -Q2.x * Q1.x - Q2.y * Q1.y - Q2.z * Q1.z + Q2.w * Q1.w
    );
}

vec4 transformVector_dq(vec4 real, vec3 v) {
    vec3 r_vector = real.xyz;
    float r_scalar = real.w;
    vec3 rotated = r_vector * 2.0 * dot(r_vector, v) +
            v * (r_scalar * r_scalar - dot(r_vector, r_vector)) +
            cross(r_vector, v) * 2.0 * r_scalar;
    return vec4(rotated, 0);
}

vec4 transformPoint_dq(vec4 real, vec4 dual, vec3 v) {
    vec3 rotated = transformVector_dq(real, v).xyz;
    vec4 conjugate = vec4(-real.xyz, real.w);
    vec3 t = mulQ(conjugate, dual * 2.0).xyz;
    return vec4(rotated + t, 1);
}

void main() {
    vec4 r0 = texelFetch(skinDQs, 2 * joints.x);
    vec4 d0 = texelFetch(skinDQs, 2 * joints.x + 1);

    vec4 w = weights;
#if INFLUENCES >= 2
    vec4 r1 = texelFetch(skinDQs, 2 * joints.y);
    vec4 d1 = texelFetch(skinDQs, 2 * joints.y + 1);
    if (dot(r0, r1) < 0.0) { w.y *= -1.0; }
#else
    vec4 r1 = r0; vec4 d1 = d0;
#endif
#if INFLUENCES >= 3
    vec4 r2 = texelFetch(skinDQs, 2 * joints.z);
    vec4 d2 = texelFetch(skinDQs, 2 * joints.z + 1);
    if (dot(r0, r2) < 0.0) { w.z *= -1.0; }
#else
    vec4 r2 = r0; vec4 d2 = d0;
#endif
#if INFLUENCES >= 4
    vec4 r3 = texelFetch(skinDQs, 2 * joints.w);
    vec4 d3 = texelFetch(skinDQs, 2 * joints.w + 1);
    if (dot(r0, r3) < 0.0) { w.w *= -1.0; }
#else
    vec4 r3 = r0; vec4 d3 = d0;
#endif

    vec4 blendReal = r0 * w.x + r1 * w.y + r2 * w.z + r3 * w.w;
    vec4 blendDual = d0 * w.x + d1 * w.y + d2 * w.z + d3 * w.w;

    float invMag = 1.0 / length(blendReal);
    blendReal *= invMag;
    blendDual *= invMag;

    vec4 v = transformPoint_dq(blendReal, blendDual, position);
    gl_Position = projection * view * model * v;
    fragPos = vec3(model * v);
    vec4 n = transformVector_dq(blendReal, normal);
    norm = vec3(model * n);
    uv = texCoord;
    // With model=identity: tf_position = local skinned position
    tf_position = v.xyz;
}

