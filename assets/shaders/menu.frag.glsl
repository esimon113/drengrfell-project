#version 410 core

in vec2 frag_uv;

uniform sampler2D sprite;
uniform vec3 fcolor;

out vec4 color;

void main() {
    color = vec4(fcolor, 1.0) * texture(sprite, frag_uv);
}
