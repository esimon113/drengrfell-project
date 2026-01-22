#version 410 core

uniform float progress;
uniform float maxDarkness;

layout (location = 0) in vec2 frag_uv;
layout (location = 0) out vec4 color;

void main() {
	float dist = distance(frag_uv, vec2(0.5));
	float radius = mix(0.75, 0.0, progress);
	float alpha = smoothstep(radius - 0.1, radius, dist);
	alpha *= maxDarkness;
	color = vec4(0, 0, 0, alpha);
}
