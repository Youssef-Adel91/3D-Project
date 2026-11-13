// passthrough.vert: Empirical memory bandwidth floor kernel.
// Touches all 56 bytes of vertex attributes and writes gl_Position.
#version 330 core

in vec3 position;
in vec3 normal;
in vec2 texCoord;
in vec4 weights;
in ivec4 joints;

out vec3 norm;
out vec3 fragPos;
out vec2 uv;

void main() {
    // Accumulate all attributes to prevent compiler dead-code elimination
    vec4 s = vec4(position, 1.0) + vec4(normal, 0.0) + weights + vec4(joints) * 0.0001;
    gl_Position = s;
    fragPos = s.xyz;
    norm = normal;
    uv = texCoord;
}

