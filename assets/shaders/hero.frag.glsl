#version 410 core

layout (location = 0) in vec2 frag_uv;

uniform sampler2D sprite;
uniform vec3 fcolor;

layout (location = 0) out vec4 color;

void main() {
    vec4 tex = texture(sprite, frag_uv);
    color = vec4(mix(tex.rgb, tex.rgb * fcolor, 0.85), tex.a);
}