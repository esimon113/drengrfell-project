#version 410 core
layout (location = 0) in vec2 frag_position;
layout (location = 1) in vec3 frag_color;

layout (location = 0) out vec4 color;

uniform vec3 fcolor;
uniform bool light_up;

void main() {
	color = vec4(fcolor * frag_color, 1);

	float radius = distance(vec2(0), frag_position);
	if (light_up && radius < 0.3) {
		color.xyz += (0.3  - radius) * 0.8 * vec3(1);
	}
}
