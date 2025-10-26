#version 410 core

layout (location = 0) in vec3 vcolor;
layout (location = 0) out vec4 color;

void main() {
	vec2 circle_coordinate = 2.0 * gl_PointCoord - 1.0;
	if (dot(circle_coordinate, circle_coordinate) > 1.0) {
		discard;
	}
	color = vec4(vcolor, 1);
}
