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

uniform mat4 animated[120];

void main() {
    mat4 skin = animated[joints.x] * weights.x;
#if INFLUENCES >= 2
    skin += animated[joints.y] * weights.y;
#endif
#if INFLUENCES >= 3
    skin += animated[joints.z] * weights.z;
#endif
#if INFLUENCES >= 4
    skin += animated[joints.w] * weights.w;
#endif

    vec4 skinnedPos = skin * vec4(position, 1.0);
    gl_Position = projection * view * model * skinnedPos;

    fragPos = vec3(model * skinnedPos);
    norm = vec3(model * skin * vec4(normal, 0.0));
    uv = texCoord;
}
