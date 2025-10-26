#version 410 core

uniform sampler2D screen_texture;
uniform float screen_darkness;
uniform float time;

layout (location = 0) in vec2 frag_uv;
layout (location = 0) out vec4 color;

vec2 distort(vec2 uv, float t) {
	// TODO: (A2) Implement the wind distortion effect here. (You may want to use `sin`/`cos`)
	return uv;
}

vec3 colorShift(vec3 in_color) {
	// TODO: (A2) Implement the shift in color here. (You may want to make it blue-ish)
	return in_color;
}

void main() {
	vec2 uv = distort(frag_uv, time);
	color = texture(screen_texture, uv);
	color.xyz = colorShift(color.xyz);
	color.xyz = screen_darkness * color.xyz;
}
