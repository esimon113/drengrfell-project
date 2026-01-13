#version 410 core

layout(location = 0) in vec2 frag_uv;

uniform vec3 highlightColor;
uniform float alpha;
uniform float time;

layout(location = 0) out vec4 color;

void main() {
    // convert uv from [0,1] to [-1,1]
    vec2 centered = (frag_uv - 0.5) * 2.0;
    float dist = length(centered);

    if (dist > 1.0) {
        discard;
    }

    // gradient: bright at center, fading to edge
    float gradient = 1.0 - smoothstep(0.7, 0.9, dist);
    // use quadratic gradient:
    gradient = gradient * gradient;

    float pulse = 0.85 + 0.15 * sin(time * 3.0);
    float finalAlpha = gradient * alpha * pulse;

    color = vec4(highlightColor, finalAlpha);
}
