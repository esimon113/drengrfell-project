#version 410 core

layout(location = 0) in vec2 pos;   
layout(location = 0) out vec2 frag_uv;

uniform mat4 projection;
uniform mat4 model;

void main() {
    frag_uv = pos * 0.5 + 0.5;
    gl_Position = projection * model * vec4(pos, 0.0, 1.0);
}
