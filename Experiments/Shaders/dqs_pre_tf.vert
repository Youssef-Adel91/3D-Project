// dqs_pre_tf.vert: DQS-pre with transform feedback output for GPU/CPU validation.
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

uniform mat2x4 animated[120];

vec4 mulQ(vec4 Q1, vec4 Q2) {
    return vec4(
         Q2.x * Q1.w + Q2.y * Q1.z - Q2.z * Q1.y + Q2.w * Q1.x,
        -Q2.x * Q1.z + Q2.y * Q1.w + Q2.z * Q1.x + Q2.w * Q1.y,
         Q2.x * Q1.y - Q2.y * Q1.x + Q2.z * Q1.w + Q2.w * Q1.z,
        -Q2.x * Q1.x - Q2.y * Q1.y - Q2.z * Q1.z + Q2.w * Q1.w
    );
}

mat2x4 normalizeDq(mat2x4 dq) {
    float invMag = 1.0 / length(dq[0]);
    dq[0] *= invMag;
    dq[1] *= invMag;
    return dq;
}

vec4 transformVector(mat2x4 dq, vec3 v) {
    vec4 real = dq[0];
    vec3 r_vector = real.xyz;
    float r_scalar = real.w;

    vec3 rotated = r_vector * 2.0 * dot(r_vector, v) +
            v * (r_scalar * r_scalar - dot(r_vector, r_vector)) +
            cross(r_vector, v) * 2.0 * r_scalar;

    return vec4(rotated, 0);
}

vec4 transformPoint(mat2x4 dq, vec3 v) {
    vec4 real = dq[0];
    vec4 dual = dq[1];

    vec3 rotated = transformVector(dq, v).xyz;
    vec4 conjugate = vec4(-real.xyz, real.w);
    vec3 t = mulQ(conjugate, dual * 2.0).xyz;

    return vec4(rotated + t, 1);
}

void main() {
    vec4 w = weights;
#if INFLUENCES >= 2
    if (dot(animated[joints.x][0], animated[joints.y][0]) < 0.0) { w.y *= -1.0; }
#endif
#if INFLUENCES >= 3
    if (dot(animated[joints.x][0], animated[joints.z][0]) < 0.0) { w.z *= -1.0; }
#endif
#if INFLUENCES >= 4
    if (dot(animated[joints.x][0], animated[joints.w][0]) < 0.0) { w.w *= -1.0; }
#endif

    mat2x4 skinDq = w.x * animated[joints.x];
#if INFLUENCES >= 2
    skinDq += w.y * animated[joints.y];
#endif
#if INFLUENCES >= 3
    skinDq += w.z * animated[joints.z];
#endif
#if INFLUENCES >= 4
    skinDq += w.w * animated[joints.w];
#endif

    skinDq = normalizeDq(skinDq);

    vec4 v = transformPoint(skinDq, position);
    gl_Position = projection * view * model * v;
    fragPos = vec3(model * v);
    vec4 n = transformVector(skinDq, normal);
    norm = vec3(model * n);
    uv = texCoord;
    // With model=identity: tf_position = local skinned position
    tf_position = v.xyz;
}

