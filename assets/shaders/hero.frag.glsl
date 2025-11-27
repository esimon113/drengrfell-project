#version 410 core

layout (location = 0) in vec2 frag_uv;

uniform sampler2D sprite;
uniform vec3 fcolor;

layout (location = 0) out vec4 color;

void main() {
	color = vec4(fcolor, 1) * texture(sprite, frag_uv);
}
