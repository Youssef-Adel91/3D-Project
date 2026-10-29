#version 330 core

in vec3 norm;
in vec3 fragPos;
in vec2 uv;

uniform vec3 light;  
uniform sampler2D tex0;

out vec4 FragColor;

void main() {
    vec4 texColor = texture(tex0, uv);
    vec3 baseColor = (texColor.a > 0.0) ? texColor.rgb : vec3(0.85, 0.85, 0.85);

    vec3 n = normalize(norm);
    if (!gl_FrontFacing) {
        n = -n;
    }

    vec3 keyDir = (length(light) > 0.001) ? normalize(light) : normalize(vec3(0.5, 0.8, 0.7));
    vec3 fillDir = normalize(vec3(-0.5, 0.4, -0.6));

    float keyDiff  = max(dot(n, keyDir), 0.0);
    float fillDiff = max(dot(n, fillDir), 0.0);
    float ambient  = 0.38;

    float totalLight = ambient + 0.48 * keyDiff + 0.22 * fillDiff;

    vec3 viewDir = normalize(-fragPos);
    vec3 h = normalize(keyDir + viewDir);
    float spec = pow(max(dot(n, h), 0.0), 16.0) * 0.12;

    totalLight += 0.000001 * length(fragPos);

    vec3 finalColor = baseColor * totalLight + vec3(spec);
    FragColor = vec4(clamp(finalColor, 0.0, 1.0), 1.0);
}
