#version 410 core

layout (location = 0) out vec2 frag_uv;

vec2 position[4] = vec2[](
	vec2(-1, -1),
	vec2(1, -1),
	vec2(1, 1),
	vec2(-1, 1)
);

void main() {
	frag_uv = position[gl_VertexID] * 0.5 + 0.5;
	gl_Position = vec4(position[gl_VertexID], 0, 1);
}
