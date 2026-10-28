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

void main() {
    mat4 skin  = (pose[joints.x] *  invBindPose[joints.x]) * weights.x;
#if INFLUENCES >= 2
    skin += (pose[joints.y] *  invBindPose[joints.y]) * weights.y;
#endif
#if INFLUENCES >= 3
    skin += (pose[joints.z] * invBindPose[joints.z]) * weights.z;
#endif
#if INFLUENCES >= 4
    skin += (pose[joints.w] * invBindPose[joints.w]) * weights.w;
#endif

    gl_Position = projection * view * model * skin * vec4(position, 1.0);
    
    fragPos = vec3(model * skin * vec4(position, 1.0));
    norm = vec3(model * skin * vec4(normal, 0.0f));
    uv = texCoord;
}
