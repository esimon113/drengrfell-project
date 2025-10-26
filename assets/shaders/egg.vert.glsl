#version 410 core

layout (location = 0) in vec2 position;
layout (location = 1) in float radius;
layout (location = 2) in vec3 color;

layout (location = 0) out vec3 vcolor;

// no model matrix, since position is already in world space
uniform mat4 view;
uniform mat4 projection;
uniform int width;

void main() {
	gl_PointSize = radius * float(width)/2;
	gl_Position = projection * view * vec4(position, 0, 1);
	vcolor = color;
}
