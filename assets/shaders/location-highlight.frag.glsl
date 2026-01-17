#version 410 core

layout(location = 0) in vec2 frag_uv;

uniform vec3 highlightColor;
uniform float alpha;
uniform float time;
uniform float pulseStrength;

layout(location = 0) out vec4 color;

void main() {
    // convert uv from [0,1] to [-1,1]
    vec2 centered = (frag_uv - 0.5) * 2.0;
    float dist = length(centered);

    if (dist > 1.0) {
        discard;
    }

    // gradient: bright at center, fading to edge
    float gradient = 1.0 - smoothstep(0.4, 0.95, dist);
    // use quadratic gradient:
    gradient = gradient * gradient;

    float basePulse = 0.85 + 0.15 * sin(time * 3.0);
    float pulse = mix(1.0, basePulse, clamp(pulseStrength, 0.0, 1.0));
    float finalAlpha = gradient * alpha * pulse;

    // Inner glow
    float innerGradient = exp(-6.0 * dist * dist);
    float innerAlpha = innerGradient * alpha * pulse * 0.25;

    vec3 innerColor = vec3(1.0);
    vec3 finalColor = mix(highlightColor, innerColor, innerGradient * 0.35);

    color = vec4(finalColor, clamp(finalAlpha + innerAlpha, 0.0, 1.0));
}
