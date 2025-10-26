#version 410 core
layout (location = 0) in vec3 position;
layout (location = 1) in vec3 color;

layout (location = 0) out vec2 frag_position;
layout (location = 1) out vec3 frag_color;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

void main() {
	frag_color = color;
	frag_position = position.xy;
	gl_Position = projection * view * model * vec4(position, 1);
}
